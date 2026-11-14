# Prototype results (core, no plugin shell yet)

Run it yourself: `sh tests/run.sh` (needs only a C++17 compiler).

What the test does, for each demo bassline at 48 kHz:

1. Measures how well kick and bass line up in the low end (line-up, -1 to +1).
2. Lets the analyzer choose a fix.
3. Runs the real fixer on the bass: one causal filter with a fixed latency.
4. Measures the processed audio again. "planned" is what the maths predicted, "measured" is what the processed audio really gives.

Columns: low end dB = change in level of kick plus bass between 20 and 200 Hz; change above 400 Hz = how much the fixer altered the bass up there, relative to the whole bass (lower is better); bass shape = similarity of the bass's low end envelope before and after (1 = unchanged).

## Findings

- The simple fix (polarity, one phase rotation, one time shift) works in a real, causal filter. Measured matches planned on every demo, the low end gains 1.5 to 5.3 dB, the bass above 400 Hz is untouched (below -110 dB at 21 ms) and the bass keeps its shape (0.98 or better).
- 21 ms of latency is enough for the simple fix. 11 ms loses a little.
- The per band fix does not hold up. With 43 ms of latency it gives less low end than the simple fix and changes the bass shape more. With 171 ms it only matches the simple fix. The larger gains seen in the design mockup came from ideal maths on a perfectly repeating loop.
- Switched off, the fixer returns the input delayed by its latency, exact to 4e-12.

These are synthetic signals. Real kicks and basses still need testing.

## Raw output

```
bypass (fixer off): max error 3.83e-12  OK
fixer on, nothing set: max error 3.83e-12  OK

demo     mode      latency  | before | planned | measured | low end dB | change above 400 Hz (re whole bass) | bass shape
sub      simple     10.7 ms | -0.71  | +0.73   | +0.72    |  +4.7      |  -97.1 dB    | 0.999
sub      simple     21.3 ms | -0.71  | +0.73   | +0.73    |  +5.3      | -113.5 dB    | 0.999
sub      simple     42.7 ms | -0.71  | +0.73   | +0.72    |  +5.3      | -135.5 dB    | 0.998
sub      per band   42.7 ms | -0.71  | +0.81   | +0.63    |  +3.7      | -127.3 dB    | 0.985
sub      per band  170.7 ms | -0.71  | +0.81   | +0.79    |  +5.3      | -168.1 dB    | 0.963
oct80    simple     10.7 ms | -0.37  | +0.44   | +0.43    |  +1.5      |  -95.0 dB    | 0.994
oct80    simple     21.3 ms | -0.37  | +0.44   | +0.44    |  +1.9      | -111.7 dB    | 0.997
oct80    simple     42.7 ms | -0.37  | +0.44   | +0.44    |  +2.0      | -133.6 dB    | 0.997
oct80    per band   42.7 ms | -0.37  | +0.67   | +0.46    |  +0.9      | -126.5 dB    | 0.961
oct80    per band  170.7 ms | -0.37  | +0.67   | +0.63    |  +2.1      | -163.4 dB    | 0.948
off      simple     10.7 ms | -0.48  | +0.65   | +0.63    |  +2.5      |  -98.8 dB    | 0.995
off      simple     21.3 ms | -0.48  | +0.65   | +0.65    |  +3.1      | -114.8 dB    | 0.995
off      simple     42.7 ms | -0.48  | +0.65   | +0.65    |  +3.2      | -135.9 dB    | 0.995
off      per band   42.7 ms | -0.48  | +0.70   | +0.59    |  +1.8      | -120.6 dB    | 0.967
off      per band  170.7 ms | -0.48  | +0.70   | +0.67    |  +3.0      | -157.9 dB    | 0.939
offoct   simple     10.7 ms | -0.39  | +0.51   | +0.48    |  +1.2      |  -94.6 dB    | 0.994
offoct   simple     21.3 ms | -0.39  | +0.51   | +0.51    |  +1.5      | -111.9 dB    | 0.997
offoct   simple     42.7 ms | -0.39  | +0.51   | +0.51    |  +1.6      | -133.8 dB    | 0.997
offoct   per band   42.7 ms | -0.39  | +0.57   | +0.53    |  -0.0      | -116.1 dB    | 0.830
offoct   per band  170.7 ms | -0.39  | +0.57   | +0.54    |  +1.1      | -153.2 dB    | 0.881
rolloct  simple     10.7 ms | -0.54  | +0.61   | +0.59    |  +2.7      |  -95.5 dB    | 0.996
rolloct  simple     21.3 ms | -0.54  | +0.61   | +0.60    |  +3.2      | -113.2 dB    | 0.997
rolloct  simple     42.7 ms | -0.54  | +0.61   | +0.60    |  +3.2      | -133.0 dB    | 0.997
rolloct  per band   42.7 ms | -0.54  | +0.64   | +0.69    |  +1.9      | -127.9 dB    | 0.912
rolloct  per band  170.7 ms | -0.54  | +0.64   | +0.64    |  +3.1      | -164.5 dB    | 0.908
offmel   simple     10.7 ms | -0.38  | +0.46   | +0.44    |  +1.2      |  -95.2 dB    | 0.997
offmel   simple     21.3 ms | -0.38  | +0.46   | +0.45    |  +1.7      | -112.8 dB    | 0.997
offmel   simple     42.7 ms | -0.38  | +0.46   | +0.46    |  +1.7      | -134.0 dB    | 0.997
offmel   per band   42.7 ms | -0.38  | +0.52   | +0.47    |  +0.2      | -120.0 dB    | 0.910
offmel   per band  170.7 ms | -0.38  | +0.52   | +0.48    |  +1.5      | -160.5 dB    | 0.917
rollmel  simple     10.7 ms | -0.37  | +0.39   | +0.37    |  +1.5      |  -95.8 dB    | 0.998
rollmel  simple     21.3 ms | -0.37  | +0.39   | +0.38    |  +2.0      | -112.6 dB    | 0.998
rollmel  simple     42.7 ms | -0.37  | +0.39   | +0.39    |  +2.1      | -133.7 dB    | 0.998
rollmel  per band   42.7 ms | -0.37  | +0.43   | +0.38    |  +0.2      | -123.5 dB    | 0.948
rollmel  per band  170.7 ms | -0.37  | +0.43   | +0.45    |  +1.9      | -156.1 dB    | 0.889
onmel    simple     10.7 ms | -0.51  | +0.52   | +0.50    |  +2.1      |  -98.1 dB    | 0.999
onmel    simple     21.3 ms | -0.51  | +0.52   | +0.51    |  +2.6      | -113.3 dB    | 0.999
onmel    simple     42.7 ms | -0.51  | +0.52   | +0.51    |  +2.7      | -133.8 dB    | 0.999
onmel    per band   42.7 ms | -0.51  | +0.57   | +0.53    |  +0.7      | -122.0 dB    | 0.943
onmel    per band  170.7 ms | -0.51  | +0.57   | +0.57    |  +2.6      | -165.3 dB    | 0.960
house    simple     10.7 ms | -0.34  | +0.39   | +0.37    |  +0.9      |  -92.3 dB    | 0.986
house    simple     21.3 ms | -0.34  | +0.39   | +0.39    |  +1.7      | -115.7 dB    | 0.978
house    simple     42.7 ms | -0.34  | +0.39   | +0.39    |  +1.9      | -136.4 dB    | 0.977
house    per band   42.7 ms | -0.34  | +0.44   | +0.38    |  +0.1      | -122.2 dB    | 0.935
house    per band  170.7 ms | -0.34  | +0.44   | +0.43    |  +1.7      | -161.3 dB    | 0.900

all checks passed
```
