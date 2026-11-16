# Direction_recoils

Impact point and direction of CYGNO reconstructed clusters (`Events` tree of the reconstruction).

## Layout

- `direction.cxx`: single process.
- `direction_mp.cxx`: the same with `n_workers` processes; the output is identical.
- `config/config.txt`: generic config. Basic keys are set; optional ones are commented. `config/config_INAF.txt`: config of `example/INAF_dir_MP.cxx`.
- `src/Analyzer.{h,cxx}`: track analysis class. `src/DirectionRun.h` is the shared driver (config, cuts, loop).
- `example/`: older scripts. `INAF_dir_MP.cxx` adds pile-up tagging and polarimetry outputs. Also `Base_script.cxx` and `Track_plotter.cpp`.

## Build and run

```sh
g++ -O3 -Isrc src/Analyzer.cxx direction_mp.cxx -o direction_mp `root-config --cflags --libs` -lSpectrum
./direction_mp config/config.txt
```

Build `direction.cxx` the same way.

## Output

`<output_dir>/Direction_<input file name>` contains:

- tree `track_info`, one row per accepted cluster: `run`, `event`, `cluster` (index in the `sc_` arrays), `Integral`, `Xmean/Ymean`, the barycentre `XBar/YBar`, the impact point `X/Y_ImpactPoint`, the direction `AngleDIR` (rad, from the impact point) and the axis `AnglePCA` (rad).
- `config`: the text of the config file used.

Pixel coordinates are those of `redpix_ix/iy`.

## Method

For each cluster:

1. Optional `threshold` and `remove_noise_value` pixel cleaning.
2. The impact point: barycentre of the last ≤ `NPIP` pixels on the skewed side of the main axis.
3. The direction: main axis of the pixels weighted by exp(−d/`wfactor`) around the impact point. The sense points from the impact point into the track.

## Notes

- The head–tail convention takes the fainter end as the start.
- Pictures (`save_png_every`, `Analyzer::SavePicDir`) are written into `./Tracks/`.
- `example/INAF_dir_MP.cxx` skips clusters tagged as pile-up. Its profile covers only x < 2304 px, so on wider cameras it tags and skips every cluster beyond that.
- `example/Base_script.cxx` and `example/Track_plotter.cpp`:
  - They read `redpix_iy` as X, so their axes are swapped.
  - Their cuts and parameters are hard-coded for LIME/MANGO.
