// End to end check of the PhasePaw core on synthetic kick and bass.
// For each demo: measure the line-up, let the analyzer choose a fix, run the real
// (causal, fixed latency) filter on the bass, then measure the processed audio again.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../core/phasepaw_core.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace phasepaw;
using Vec = std::vector<double>;

static const double SR = 48000.0;
static const double TB = 8192.0 / 22050.0; // one beat, about 161.5 BPM (same as the mockup)

static Vec genKick(double f1, bool shortKick, int n) {
    Vec k(static_cast<size_t>(n)); double ph = 0, mx = 0, prev = 0; unsigned seed = 12345;
    for (int i = 0; i < n; ++i) {
        const double t = i / SR;
        const double f = f1 + 3.35 * f1 * std::exp(-t / 0.024) + 1100 * std::exp(-t / 0.004);
        ph += 2 * kPi * f / SR;
        const double env = (shortKick ? std::exp(-t / 0.055) : (0.5 + 0.5 * std::exp(-t / 0.09)) * std::exp(-t / 0.3)) * std::min(1.0, t / 0.0008);
        const double tail = std::min(1.0, (TB - t) / 0.02);
        double v = std::tanh(2.4 * std::sin(ph) * env) / std::tanh(2.4);
        seed = (seed * 1103515245u + 12345u) & 0x7fffffffu;
        const double nz = seed / double(0x3fffffff) - 1, hp = nz - prev; prev = nz;
        v += 0.22 * hp * std::exp(-t / 0.004) + 0.10 * std::sin(2 * kPi * 3200 * t) * std::exp(-t / 0.0025);
        v *= std::max(0.0, tail);
        k[size_t(i)] = v; mx = std::max(mx, std::abs(v));
    }
    for (auto& v : k) v *= 0.9 / mx;
    return k;
}
struct Note { double start, len, semi, rel; };
static double noteEnv(double t, double len, double att, double rel) {
    if (t < 0 || t > len + rel) return 0;
    return std::min(1.0, t / att) * (t < len ? 1.0 : std::max(0.0, 1 - (t - len) / rel));
}
static Vec genNotes(double f0, double phDeg, const std::vector<Note>& notes, int n) {
    Vec b(size_t(n), 0.0); const double phi = phDeg * kPi / 180;
    for (const auto& nt : notes) {
        const double f = f0 * std::pow(2.0, nt.semi / 12.0);
        const int nh = std::min(110, int(4800 / f));
        std::vector<double> amps;
        for (int h = 1; h <= nh; ++h) amps.push_back((h == 1 ? 1.7 : 1 / std::pow(h, 0.92)) / (1 + std::pow(h * f / 2600, 2)));
        const int n0 = std::max(0, int(nt.start * SR)), n1 = std::min(n, int(std::ceil((nt.start + nt.len + nt.rel) * SR)));
        for (int i = n0; i < n1; ++i) {
            const double lt = i / SR - nt.start; if (lt < 0) continue;
            const double e = noteEnv(lt, nt.len, 0.003, nt.rel); if (e == 0) continue;
            const double w = 2 * kPi * f * lt + phi, c1 = std::cos(w); double sp = 0, sc = std::sin(w), sm = 0;
            for (int h = 0; h < nh; ++h) { sm += amps[size_t(h)] * sc; const double ns = 2 * c1 * sc - sp; sp = sc; sc = ns; }
            b[size_t(i)] += e * sm;
        }
    }
    return b;
}
using Pattern = std::vector<std::vector<Note>>; // notes per beat
static Pattern pattern(const std::string& name) {
    const double e8 = 0.5 * TB, s = 0.25 * TB, r = 0.015;
    auto off = [&](double n) { return std::vector<Note>{{e8, 0.13, n, 0.04}}; };
    auto roll = [&](double a, double b, double c) { return std::vector<Note>{{s, 0.07, a, r}, {2 * s, 0.07, b, r}, {3 * s, 0.07, c, r}}; };
    if (name == "sub") return {{{0, 0.26, 0, 0.04}}};
    if (name == "oct80") return {{{0, 0.12, 0, 0.04}, {e8, 0.12, 12, 0.04}}};
    if (name == "off") return {off(0)};
    if (name == "offoct") return {off(0), off(12), off(0), off(12)};
    if (name == "rolloct") return {roll(0, 12, 0)};
    if (name == "offmel") return {off(0), off(0), off(3), off(5)};
    if (name == "rollmel") return {roll(0, 0, 0), roll(0, 0, 3), roll(5, 5, 5), roll(3, 3, -2)};
    if (name == "onmel") return {{{0, 0.3, 0, 0.04}}, {{0, 0.3, 0, 0.04}}, {{0, 0.3, -2, 0.04}}, {{0, 0.3, 3, 0.04}}};
    if (name == "house") return {{{0, 0.1, 0, 0.04}, {3 * s, 0.06, 0, r}}, {{e8, 0.1, 0, 0.04}}, {{0, 0.1, 3, 0.04}, {3 * s, 0.06, 3, r}}, {{e8, 0.1, 5, 0.04}}};
    return {{{0, 0.26, 0, 0.04}}};
}
static void makeDemo(const std::string& name, int beats, Vec& kick, Vec& bass) {
    const int nb = int(std::lround(TB * SR)); const double f = 43.65;
    const Vec k1 = genKick(f, false, nb); const Pattern p = pattern(name);
    std::vector<Vec> bb; double mx = 1e-9;
    for (const auto& notes : p) { bb.push_back(genNotes(f, 150, notes, nb)); for (double v : bb.back()) mx = std::max(mx, std::abs(v)); }
    kick.assign(size_t(nb * beats), 0.0); bass.assign(size_t(nb * beats), 0.0);
    for (int b = 0; b < beats; ++b) for (int i = 0; i < nb; ++i) {
        kick[size_t(b * nb + i)] = k1[size_t(i)];
        bass[size_t(b * nb + i)] = bb[size_t(b) % bb.size()][size_t(i)] * 0.75 / mx;
    }
}
static Vec delayBy(const Vec& x, int d) { Vec y(x.size(), 0.0); for (size_t i = size_t(d); i < x.size(); ++i) y[i] = x[i - size_t(d)]; return y; }
// energy of x between f0 and f1 (whole signal)
static double bandEnergy(const Vec& x, double f0, double f1) {
    size_t n = 1; while (n < x.size()) n <<= 1; std::vector<cd> a(n);
    for (size_t i = 0; i < x.size(); ++i) a[i] = x[i] * (0.5 - 0.5 * std::cos(2 * kPi * double(i) / double(x.size()))); // fade the ends, or the cut leaks into every band
    fft(a, false); double e = 0;
    for (size_t k = 1; k < n / 2; ++k) { const double f = double(k) * SR / double(n); if (f >= f0 && f < f1) e += std::norm(a[k]); }
    return e;
}
static Vec lowpass(const Vec& x, double fc) {
    size_t n = 1; while (n < x.size()) n <<= 1; std::vector<cd> a(n);
    for (size_t i = 0; i < x.size(); ++i) a[i] = x[i];
    fft(a, false);
    for (size_t k = 0; k < n; ++k) { const double f = double(std::min(k, n - k)) * SR / double(n); if (f > fc) a[k] = 0; }
    fft(a, true); Vec y(x.size()); for (size_t i = 0; i < x.size(); ++i) y[i] = a[i].real(); return y;
}
static double envSimilarity(const Vec& a, const Vec& b) { // low end envelopes, 10 ms steps
    const Vec la = lowpass(a, 250), lb = lowpass(b, 250); const size_t w = 480; std::vector<double> ea, eb;
    for (size_t i = 0; i + w <= la.size(); i += w) { double x = 0, y = 0; for (size_t j = 0; j < w; ++j) { x += la[i + j] * la[i + j]; y += lb[i + j] * lb[i + j]; } ea.push_back(std::sqrt(x)); eb.push_back(std::sqrt(y)); }
    double best = 0;
    for (int sh = -3; sh <= 3; ++sh) { double sa = 0, sb = 0, sab = 0; for (size_t i = 0; i < ea.size(); ++i) { const long j = long(i) + sh; if (j < 0 || j >= long(eb.size())) continue; sa += ea[i] * ea[i]; sb += eb[size_t(j)] * eb[size_t(j)]; sab += ea[i] * eb[size_t(j)]; } best = std::max(best, sab / std::sqrt(sa * sb)); }
    return best;
}

int main() {
    int failures = 0;
    const int beats = 24;
    // 1. transparency
    {
        Vec kick, bass; makeDemo("sub", 8, kick, bass);
        Fixer fx(SR, 2048); Settings s; s.on = false; fx.design(s);
        Vec y = fx.run(bass), d = delayBy(bass, fx.latency()); double err = 0;
        for (size_t i = 0; i < y.size(); ++i) err = std::max(err, std::abs(y[i] - d[i]));
        std::printf("bypass (fixer off): max error %.2e  %s\n", err, err < 1e-9 ? "OK" : "FAIL"); if (err >= 1e-9) ++failures;
        Settings n; fx.design(n); y = fx.run(bass); err = 0;
        for (size_t i = 0; i < y.size(); ++i) err = std::max(err, std::abs(y[i] - d[i]));
        std::printf("fixer on, nothing set: max error %.2e  %s\n", err, err < 1e-6 ? "OK" : "FAIL"); if (err >= 1e-6) ++failures;
    }
    // 2. every demo, two latencies, simple and per band
    const char* demos[] = {"sub", "oct80", "off", "offoct", "rolloct", "offmel", "rollmel", "onmel", "house"};
    std::printf("\n%-8s %-9s %-8s | before | planned | measured | low end dB | change above 400 Hz (re whole bass) | bass shape\n", "demo", "mode", "latency");
    for (const char* name : demos) {
        Vec kick, bass; makeDemo(name, beats, kick, bass);
        Analyzer an(SR); an.add(kick, bass); Settings neutral;
        const double before = an.lineup(neutral);
        const double eBefore = bandEnergy(lowpass(Vec(kick), 24000), 20, 200); (void)eBefore;
        Vec mix0(kick.size()); for (size_t i = 0; i < kick.size(); ++i) mix0[i] = kick[i] + bass[i];
        const double low0 = bandEnergy(mix0, 20, 200);
        for (int perBand = 0; perBand <= 1; ++perBand) for (int half : {512, 1024, 2048, 8192}) {
            if (!perBand && half == 8192) continue; // the simple fix does not need the long filter
            if (perBand && half < 2048) continue;   // the per band fix needs a long filter to tell the bands apart
            const Settings s = an.autoAlign(neutral, perBand != 0);
            const double planned = an.lineup(s);
            Fixer fx(SR, half); fx.design(s);
            const Vec bf = fx.run(bass), kd = delayBy(kick, fx.latency()), bd = delayBy(bass, fx.latency());
            // skip the start (filter fill) when measuring
            const size_t skip = size_t(fx.latency()) + 4096;
            Vec k2(kd.begin() + long(skip), kd.end()), b2(bf.begin() + long(skip), bf.end()), b0(bd.begin() + long(skip), bd.end());
            Analyzer an2(SR); an2.add(k2, b2);
            const double measured = an2.lineup(neutral);
            Vec mix1(k2.size()), mixRef(k2.size()); for (size_t i = 0; i < k2.size(); ++i) { mix1[i] = k2[i] + b2[i]; mixRef[i] = k2[i] + b0[i]; }
            const double lowDb = 10 * std::log10(bandEnergy(mix1, 20, 200) / bandEnergy(mixRef, 20, 200));
            Vec diff(b2.size()); for (size_t i = 0; i < b2.size(); ++i) diff[i] = b2[i] - b0[i];
            const double hiDb = 10 * std::log10(bandEnergy(diff, 400, 20000) / bandEnergy(b0, 20, 20000) + 1e-30);
            const double shape = envSimilarity(b0, b2);
            std::printf("%-8s %-9s %5.1f ms | %+5.2f  | %+5.2f   | %+5.2f    | %+5.1f      | %6.1f dB    | %.3f\n", name, perBand ? "per band" : "simple",
                        1000.0 * fx.latency() / SR, before, planned, measured, lowDb, hiDb, shape);
            if (measured < before + 0.15 && planned > before + 0.3) { std::printf("  FAIL: the processed audio did not improve as planned\n"); ++failures; }
            if (hiDb > -60) { std::printf("  FAIL: the fixer changed the bass above 400 Hz\n"); ++failures; }
        }
        (void)low0;
    }
    std::printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
