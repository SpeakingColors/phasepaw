// PhasePaw core: analysis and fixer, plain C++17, no dependencies.
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Analyzer: measures how well kick and bass line up in the low end (cross spectrum over
//           overlapping frames) and works out the best fix.
// Fixer:    one linear phase FIR that applies polarity, phase rotation, time shift and a
//           phase turn per third-octave band, only inside the chosen band. Fixed latency.
//
// This file is the prototype used by tests/prototype.cpp. The block based, real time
// versions for the plugin are built on the same maths.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace phasepaw {

using cd = std::complex<double>;
constexpr double kPi = 3.14159265358979323846;
constexpr int kBands = 15; // third-octave bands, centres 25 Hz * 2^(i/3)

inline void fft(std::vector<cd>& a, bool inverse) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = 2 * kPi / double(len) * (inverse ? 1 : -1);
        const cd wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            cd w(1, 0);
            for (size_t j = 0; j < len / 2; ++j) {
                const cd u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
    if (inverse) for (auto& x : a) x /= double(n);
}

struct Settings {
    bool on = true;
    int pol = 1;                          // +1 or -1
    double rotDeg = 0;                    // 0..360
    double timeMs = 0;                    // positive = bass later
    std::array<double, kBands> bandDeg{}; // extra phase turn per band
    double lo = 20, hi = 200;             // band the fixer works in
};

inline double smoothstep(double x) { x = std::clamp(x, 0.0, 1.0); return x * x * (3 - 2 * x); }

// 1 inside the band, soft upper edge (about 0.3 octave each side of hi), fades out below 12 Hz.
inline double mask(double f, const Settings& s) {
    if (f <= 0) return 0;
    return smoothstep(f / 12.0) * smoothstep((std::log2(s.hi / f) + 0.3) / 0.6);
}
inline int bandIndex(double f) {
    const int p = int(std::lround(std::log2(std::max(f, 1.0) / 25.0) * 3.0));
    return std::clamp(p, 0, kBands - 1);
}
// What the fixer does to the bass at frequency f.
inline cd gain(double f, const Settings& s) {
    if (!s.on) return cd(1, 0);
    const double m = mask(f, s);
    if (m <= 0) return cd(1, 0);
    const double ph = (s.rotDeg + s.bandDeg[size_t(bandIndex(f))]) * kPi / 180.0 - 2 * kPi * f * s.timeMs / 1000.0;
    return (1 - m) + m * double(s.pol) * std::polar(1.0, ph);
}

class Analyzer {
public:
    explicit Analyzer(double sampleRate, int frame = 16384, int hopSize = 4096)
        : sr(sampleRate), N(frame), hop(hopSize), win(size_t(frame)) {
        for (int i = 0; i < N; ++i) win[size_t(i)] = 0.5 - 0.5 * std::cos(2 * kPi * i / N);
        kmax = std::min(N / 2 - 1, int(std::ceil(1200.0 * N / sr)));
        reset();
    }
    void reset() { C.assign(size_t(kmax + 1), cd(0, 0)); W.assign(size_t(kmax + 1), 0.0); frames = 0; }

    // Offline use: add every frame of kick and bass (same length, mono).
    void add(const std::vector<double>& kick, const std::vector<double>& bass) {
        std::vector<cd> X(static_cast<size_t>(N)), B(static_cast<size_t>(N));
        for (size_t start = 0; start + size_t(N) <= kick.size(); start += size_t(hop)) {
            for (int i = 0; i < N; ++i) {
                X[size_t(i)] = kick[start + size_t(i)] * win[size_t(i)];
                B[size_t(i)] = bass[start + size_t(i)] * win[size_t(i)];
            }
            fft(X, false); fft(B, false);
            for (int k = 1; k <= kmax; ++k) {
                C[size_t(k)] += X[size_t(k)] * std::conj(B[size_t(k)]);
                W[size_t(k)] += std::abs(X[size_t(k)]) * std::abs(B[size_t(k)]);
            }
            ++frames;
        }
    }
    double freq(int k) const { return k * sr / N; }

    // Line-up (-1..+1) the bass would have against the kick with these settings.
    double lineup(const Settings& s) const {
        double num = 0, den = 0;
        for (int k = 1; k <= kmax; ++k) {
            const double f = freq(k), m = mask(f, s);
            if (m < 1e-3) continue;
            const cd g = gain(f, s);
            num += m * (C[size_t(k)] * std::conj(g)).real();
            den += m * W[size_t(k)] * std::abs(g);
        }
        return den > 0 ? num / den : 0;
    }

    // Best settings found from the measured cross spectrum.
    Settings autoAlign(Settings s, bool perBand) const {
        s.on = true; s.pol = 1; s.rotDeg = 0; s.timeMs = 0; s.bandDeg.fill(0);
        std::vector<int> ks; std::vector<cd> c; std::vector<int> bd; std::array<double, kBands> wb{}; double wtot = 0;
        for (int k = 1; k <= kmax; ++k) {
            const double m = mask(freq(k), s);
            if (m < 1e-3) continue;
            ks.push_back(k); c.push_back(m * C[size_t(k)]); bd.push_back(bandIndex(freq(k)));
            wb[size_t(bd.back())] += m * W[size_t(k)]; wtot += m * W[size_t(k)];
        }
        if (wtot <= 0) return s;
        double bestScore = -1e300, bestD = 0; cd bestG; std::array<cd, kBands> bestB{};
        for (int step = -100; step <= 100; ++step) {
            const double d = step * 0.1; cd g(0, 0); std::array<cd, kBands> sb{};
            for (size_t i = 0; i < ks.size(); ++i) {
                const cd e = c[i] * std::polar(1.0, 2 * kPi * freq(ks[i]) * d / 1000.0);
                g += e; sb[size_t(bd[i])] += e;
            }
            double v = 0;
            if (perBand) for (const auto& x : sb) v += std::abs(x); else v = std::abs(g);
            v -= 0.004 * std::abs(d) * wtot;
            if (v > bestScore) { bestScore = v; bestD = d; bestG = g; bestB = sb; }
        }
        double rot = std::arg(bestG) * 180.0 / kPi; // -180..180
        s.timeMs = bestD;
        if (rot > 90 || rot < -90) { s.pol = -1; rot += (rot > 0 ? -180 : 180); }
        s.rotDeg = std::fmod(rot + 360.0, 360.0);
        if (perBand) {
            const double base = std::arg(bestG);
            for (int b = 0; b < kBands; ++b) {
                const cd sb = bestB[size_t(b)];
                if (wb[size_t(b)] <= 0) continue;
                const double now = (sb * std::polar(1.0, -base)).real();
                if (std::abs(sb) - now > 0.05 * wb[size_t(b)]) {
                    double d = (std::arg(sb) - base) * 180.0 / kPi;
                    s.bandDeg[size_t(b)] = std::fmod(d + 720.0, 360.0);
                }
            }
        }
        return s;
    }

    double sr; int N, hop, kmax = 0, frames = 0;
    std::vector<double> win; std::vector<cd> C; std::vector<double> W;
};

class Fixer {
public:
    // half = latency in samples; the filter has 2 * half + 1 taps. half must be a power of two.
    Fixer(double sampleRate, int halfLength) : sr(sampleRate), half(halfLength), h(size_t(2 * halfLength + 1), 0.0) {
        h[size_t(half)] = 1.0;
    }
    int latency() const { return half; }

    void design(const Settings& s) {
        const size_t ng = size_t(8 * half);
        std::vector<cd> D(ng);
        for (size_t k = 0; k <= ng / 2; ++k) {
            cd g = gain(double(k) * sr / double(ng), s);
            if (k == 0 || k == ng / 2) g = cd(g.real(), 0);
            D[k] = g;
            if (k > 0 && k < ng / 2) D[ng - k] = std::conj(g);
        }
        fft(D, true);
        for (int i = -half; i <= half; ++i) {
            const double w = 0.5 + 0.5 * std::cos(kPi * i / double(half + 1));
            h[size_t(i + half)] = D[(size_t(i + int(ng))) % ng].real() * w;
        }
        // keep the part that is not touched exactly transparent
        if (!s.on) { std::fill(h.begin(), h.end(), 0.0); h[size_t(half)] = 1.0; }
    }

    // Offline convolution; output has the same length as the input and is late by latency().
    std::vector<double> run(const std::vector<double>& x) const {
        size_t n = 1; while (n < x.size() + h.size()) n <<= 1;
        std::vector<cd> a(n), b(n);
        for (size_t i = 0; i < x.size(); ++i) a[i] = x[i];
        for (size_t i = 0; i < h.size(); ++i) b[i] = h[i];
        fft(a, false); fft(b, false);
        for (size_t i = 0; i < n; ++i) a[i] *= b[i];
        fft(a, true);
        std::vector<double> y(x.size());
        for (size_t i = 0; i < x.size(); ++i) y[i] = a[i].real();
        return y;
    }

    double sr; int half; std::vector<double> h;
};

} // namespace phasepaw
