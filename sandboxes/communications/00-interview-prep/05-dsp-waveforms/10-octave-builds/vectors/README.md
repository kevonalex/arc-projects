# vectors/

Generated files go here (`fft_*.txt`, `fir_*.txt`, `fsk4_*.txt`, `sync_*.txt`, `cfo_*.txt`, `q15_*.txt`, `ber_theory.txt`).

Create them with:

```
octave --no-gui octave/gen_vectors.m        # from the pack root (10-octave-builds)
```

The C++ tests compare against these files when they exist and **skip** the comparison (GTest "SKIPPED") when they do not, so the analytic tests still run on a machine without Octave. They are deliberately not committed: regenerate them so the reference is always produced by the model you can read in `octave/gen_vectors.m`.
