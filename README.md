# Direction_recoils
Code to be applied to reco_files to obtain direction and head-tail

# Directionality and Analysis Features

This repository provides an `Analyzer` class and related scripts for directionality studies and general analysis of CYGNO reconstructed tracks.

## Main Components

- **Core Implementation:**  
  - `Analyzer.cxx` (main class implementation)  
  - `Analyzer.h` (class header)
  - `TrackPlotter.cpp` (simple utility for plotting tracks)

- **Example Scripts:**  
  - `Base_script.cxx`: Basic usage example, suitable for getting started with the analysis workflow.
  - `INAF_dir_MP.cxx`: Multiprocessing version compatible with `INAF_dir.cxx`, configurable via a config file for batch processing.

### Mulitiprocessing Usage

The `INAF_dir_MP.cxx` script allows for multiprocessing, it is now the standard for ER directionality analysis.

- **Pile-up Tagging:**  
  The ability to tag pile-up events exists but is currently hardcoded. This feature may be removed or made more flexible in future versions.

- **Event Cuts:**  
  Event cuts are managed through config files; however, the set of variables available for cuts is hardcoded. To introduce new variables, you must update both the config file and the `event_vars` map in the code.

## Compilation Instructions

To compile the main analysis program:

```sh
g++ Analyzer.cxx Base_script.cxx -o nameprog `root-config --libs --cflags` -lSpectrum
```

For optimized builds:

```sh
g++ Analyzer.cxx Base_script.cxx -O3 -o nameprog `root-config --libs --cflags` -lSpectrum
```

## Additional Features

- **Track Quality Cuts:**  
  - Scripts include configurable cuts for track length, energy, and cluster size.
- **Output:**  
  - Results are saved in ROOT files for further analysis and plotting.
- **Configurable Parameters:**  
  - Most scripts accept command-line or config file parameters for reproducible workflows.

For further details, refer to comments in each script and the inline documentation in `Analyzer.h`.
