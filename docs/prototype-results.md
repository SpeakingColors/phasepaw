# Prototype results (core, no plugin shell yet)

Run it yourself: `sh tests/run.sh` (needs only a C++17 compiler).

## What the fixer does

- Polarity flip: the whole bass. Exact, no latency of its own.
- Time shift: the whole bass, in whole samples. Exact. The bass stays in one piece.
- Phase rotation: only inside the chosen band, as a pure phase change. The band tells the fixer where to work; it never acts as an EQ or a filter.

## What the test checks

1. Switched off, the fixer returns its input delayed by its latency (exact to 4e-12).
2. Phase only: the level of every frequency from 30 Hz to 20 kHz stays the same. Polarity and time shift: 0.000 dB change. Rotation with the standard 42.7 ms filter: within 0.25 dB. Shorter filters bend the level at the bottom of the band (21.3 ms: up to 1.2 dB, 10.7 ms: unusable for large rotations).
3. Signs and reach: a "bass" that is the kick itself, late or upside down, comes out at +1.00 line-up.
4. Nine demo basslines at 48 kHz: measure the line-up, let the analyzer choose a fix, run the real filter, measure the processed audio again. "planned" is what the maths predicted, "measured" is what the processed audio gives. Low end dB is the level change of kick plus bass between 20 and 300 Hz. "Change above 400 Hz" is what the fixer did to the bass up there beyond flipping and moving it (lower is better). Bass shape is the similarity of the bass's low end envelope before and after (1 = unchanged).

## Findings

- The simple fix (polarity, one rotation, one time shift) works in a real, causal filter. Measured matches planned on every demo, the low end gains 1.5 to 5.2 dB, nothing above the band is altered, and the bass keeps its shape (0.98 or better).
- Standard latency: 52.7 ms at 48 kHz (42.7 ms filter plus 10 ms of room for the time shift). The host compensates it.
- If no rotation is needed (polarity and time shift only), no filter is needed at all. That could become a mode with almost no latency.
- Per band fix (each third-octave band gets its own phase turn, 351 ms latency): it raises the line-up number (by up to 0.2) but adds no low end level compared with the simple fix (0.0 to -0.2 dB), and it changes the bass shape more (0.87 to 0.97). On this evidence it is not worth its latency. It stays an experiment, off by default.
- Lesson: the line-up number alone can be fooled by smearing the bass in time. The level of the low end and the bass shape have to be checked with it.

These are synthetic signals. Real kicks and basses still need testing, and nobody has listened to the output yet.

## Raw output

```
bypass (fixer off): max error 3.83e-12  OK
fixer on, nothing set: max error 3.83e-12  OK
phase only, polarity flip          filter  10.7 ms: worst level change 30 Hz to 20 kHz +0.000 dB at 43 Hz  OK
phase only, polarity flip          filter  21.3 ms: worst level change 30 Hz to 20 kHz +0.000 dB at 45 Hz  OK
phase only, polarity flip          filter  42.7 ms: worst level change 30 Hz to 20 kHz -0.000 dB at 34 Hz  OK
phase only, rotation 45            filter  10.7 ms: worst level change 30 Hz to 20 kHz -0.485 dB at 30 Hz  OK
phase only, rotation 45            filter  21.3 ms: worst level change 30 Hz to 20 kHz -0.145 dB at 30 Hz  OK
phase only, rotation 45            filter  42.7 ms: worst level change 30 Hz to 20 kHz +0.073 dB at 30 Hz  OK
phase only, rotation 90            filter  10.7 ms: worst level change 30 Hz to 20 kHz -7.355 dB at 49 Hz  OK
phase only, rotation 90            filter  21.3 ms: worst level change 30 Hz to 20 kHz -0.824 dB at 30 Hz  OK
phase only, rotation 90            filter  42.7 ms: worst level change 30 Hz to 20 kHz +0.229 dB at 30 Hz  OK
phase only, time shift 5 ms        filter  10.7 ms: worst level change 30 Hz to 20 kHz +0.000 dB at 43 Hz  OK
phase only, time shift 5 ms        filter  21.3 ms: worst level change 30 Hz to 20 kHz +0.000 dB at 45 Hz  OK
phase only, time shift 5 ms        filter  42.7 ms: worst level change 30 Hz to 20 kHz -0.000 dB at 34 Hz  OK
phase only, flip + 40 deg - 3 ms   filter  10.7 ms: worst level change 30 Hz to 20 kHz -0.450 dB at 30 Hz  OK
phase only, flip + 40 deg - 3 ms   filter  21.3 ms: worst level change 30 Hz to 20 kHz -0.107 dB at 30 Hz  OK
phase only, flip + 40 deg - 3 ms   filter  42.7 ms: worst level change 30 Hz to 20 kHz +0.058 dB at 30 Hz  OK
phase only, rot 90, band 60-150    filter  10.7 ms: worst level change 30 Hz to 20 kHz -3.449 dB at 59 Hz  OK
phase only, rot 90, band 60-150    filter  21.3 ms: worst level change 30 Hz to 20 kHz -1.226 dB at 59 Hz  OK
phase only, rot 90, band 60-150    filter  42.7 ms: worst level change 30 Hz to 20 kHz -0.169 dB at 56 Hz  OK
copy, 3 ms late                  before +0.43, auto: polarity +1, rotation   2.1 deg, shift -2.9 ms, measured after +1.000  OK
copy, upside down                before -1.00, auto: polarity -1, rotation   0.0 deg, shift +0.0 ms, measured after +1.000  OK
copy, 2 ms late and upside down  before -0.70, auto: polarity -1, rotation   2.1 deg, shift -1.9 ms, measured after +1.000  OK

demo     mode      latency  | before | planned | measured | low end dB (20-300 Hz) | change above 400 Hz | bass shape
sub      simple     20.7 ms | -0.71  | +0.73   | +0.73    |  +5.2      | -110.0 dB    | 0.999
sub      simple     31.3 ms | -0.71  | +0.73   | +0.73    |  +5.2      | -143.8 dB    | 0.999
sub      simple     52.7 ms | -0.71  | +0.73   | +0.72    |  +5.2      | -180.3 dB    | 0.999
sub      per band   52.7 ms | -0.71  | +0.81   | +0.75    |  +3.2      | -114.6 dB    | 0.968
sub      per band  180.7 ms | -0.71  | +0.81   | +0.83    |  +4.6      | -152.4 dB    | 0.968
sub      per band  351.3 ms | -0.71  | +0.81   | +0.83    |  +5.2      | -165.0 dB    | 0.965
oct80    simple     20.7 ms | -0.37  | +0.44   | +0.44    |  +2.0      |  -88.7 dB    | 0.997
oct80    simple     31.3 ms | -0.37  | +0.44   | +0.44    |  +2.0      | -121.4 dB    | 0.997
oct80    simple     52.7 ms | -0.37  | +0.44   | +0.44    |  +2.0      | -150.2 dB    | 0.997
oct80    per band   52.7 ms | -0.37  | +0.67   | +0.47    |  +2.8      | -111.8 dB    | 0.877
oct80    per band  180.7 ms | -0.37  | +0.67   | +0.42    |  +1.0      | -148.8 dB    | 0.874
oct80    per band  351.3 ms | -0.37  | +0.67   | +0.65    |  +2.0      | -165.9 dB    | 0.880
off      simple     20.7 ms | -0.48  | +0.64   | +0.65    |  +3.1      |  -89.2 dB    | 0.994
off      simple     31.3 ms | -0.48  | +0.64   | +0.64    |  +3.1      | -121.8 dB    | 0.994
off      simple     52.7 ms | -0.48  | +0.64   | +0.65    |  +3.1      | -148.6 dB    | 0.994
off      per band   52.7 ms | -0.48  | +0.70   | +0.58    |  +4.2      | -107.8 dB    | 0.964
off      per band  180.7 ms | -0.48  | +0.70   | +0.61    |  +2.5      | -139.4 dB    | 0.918
off      per band  351.3 ms | -0.48  | +0.70   | +0.69    |  +3.1      | -159.7 dB    | 0.918
offoct   simple     20.7 ms | -0.39  | +0.51   | +0.51    |  +1.6      |  -85.4 dB    | 0.996
offoct   simple     31.3 ms | -0.39  | +0.51   | +0.51    |  +1.5      | -117.2 dB    | 0.996
offoct   simple     52.7 ms | -0.39  | +0.51   | +0.51    |  +1.5      | -143.4 dB    | 0.996
offoct   per band   52.7 ms | -0.39  | +0.57   | +0.45    |  +1.7      | -103.5 dB    | 0.929
offoct   per band  180.7 ms | -0.39  | +0.57   | +0.48    |  +1.0      | -140.4 dB    | 0.944
offoct   per band  351.3 ms | -0.39  | +0.57   | +0.52    |  +1.4      | -159.0 dB    | 0.902
rolloct  simple     20.7 ms | -0.54  | +0.60   | +0.60    |  +3.2      |  -89.0 dB    | 0.997
rolloct  simple     31.3 ms | -0.54  | +0.60   | +0.60    |  +3.2      | -122.8 dB    | 0.997
rolloct  simple     52.7 ms | -0.54  | +0.60   | +0.60    |  +3.2      | -149.3 dB    | 0.997
rolloct  per band   52.7 ms | -0.54  | +0.64   | +0.62    |  +1.5      | -116.0 dB    | 0.919
rolloct  per band  180.7 ms | -0.54  | +0.64   | +0.66    |  +2.9      | -152.4 dB    | 0.890
rolloct  per band  351.3 ms | -0.54  | +0.64   | +0.64    |  +3.1      | -165.2 dB    | 0.885
offmel   simple     20.7 ms | -0.38  | +0.46   | +0.45    |  +1.7      |  -84.3 dB    | 0.997
offmel   simple     31.3 ms | -0.38  | +0.46   | +0.45    |  +1.7      | -118.6 dB    | 0.997
offmel   simple     52.7 ms | -0.38  | +0.46   | +0.46    |  +1.7      | -145.8 dB    | 0.997
offmel   per band   52.7 ms | -0.38  | +0.52   | +0.44    |  +3.1      |  -98.5 dB    | 0.951
offmel   per band  180.7 ms | -0.38  | +0.52   | +0.42    |  +1.6      | -132.6 dB    | 0.914
offmel   per band  351.3 ms | -0.38  | +0.52   | +0.51    |  +1.7      | -149.5 dB    | 0.902
rollmel  simple     20.7 ms | -0.37  | +0.39   | +0.38    |  +2.1      |  -91.3 dB    | 0.999
rollmel  simple     31.3 ms | -0.37  | +0.39   | +0.38    |  +2.1      | -125.8 dB    | 0.999
rollmel  simple     52.7 ms | -0.37  | +0.39   | +0.39    |  +2.1      | -158.2 dB    | 0.999
rollmel  per band   52.7 ms | -0.37  | +0.43   | +0.46    |  +0.9      | -101.6 dB    | 0.877
rollmel  per band  180.7 ms | -0.37  | +0.43   | +0.43    |  +1.7      | -133.6 dB    | 0.886
rollmel  per band  351.3 ms | -0.37  | +0.43   | +0.44    |  +1.9      | -156.1 dB    | 0.873
onmel    simple     20.7 ms | -0.51  | +0.51   | +0.51    |  +2.7      | -109.8 dB    | 1.000
onmel    simple     31.3 ms | -0.51  | +0.51   | +0.51    |  +2.7      | -144.7 dB    | 1.000
onmel    simple     52.7 ms | -0.51  | +0.51   | +0.51    |  +2.7      | -183.1 dB    | 1.000
onmel    per band   52.7 ms | -0.51  | +0.57   | +0.24    |  +5.7      |  -97.1 dB    | 0.968
onmel    per band  180.7 ms | -0.51  | +0.57   | +0.58    |  +2.6      | -138.6 dB    | 0.954
onmel    per band  351.3 ms | -0.51  | +0.57   | +0.55    |  +2.7      | -155.0 dB    | 0.962
house    simple     20.7 ms | -0.34  | +0.39   | +0.37    |  +1.4      |  -76.1 dB    | 0.987
house    simple     31.3 ms | -0.34  | +0.39   | +0.38    |  +1.8      | -108.5 dB    | 0.978
house    simple     52.7 ms | -0.34  | +0.39   | +0.39    |  +1.8      | -131.5 dB    | 0.979
house    per band   52.7 ms | -0.34  | +0.43   | +0.21    |  +3.9      | -106.6 dB    | 0.927
house    per band  180.7 ms | -0.34  | +0.43   | +0.43    |  +1.4      | -143.7 dB    | 0.881
house    per band  351.3 ms | -0.34  | +0.43   | +0.43    |  +1.7      | -159.3 dB    | 0.879

all checks passed
```
