This repository can be used to recreate the simulation results described in the paper [The Lasso Method for Multi-Robot Foraging](https://ieeexplore.ieee.org/document/9867081).

# cwaggle: C++ Simulator
A version of the [cwaggle simulator](https://github.com/davechurchill/cwaggle).

## Compilation
Execute `make` in the cwaggle directory to build the executable `cwaggle_lasso` in `cwaggle/bin`.  This Makefile works on Mac OS and should also work in Linux:

    cd cwaggle
    make

## Execution

    cd cwaggle/bin
    ./cwaggle_lasso

Executing in `cwaggle/bin` is necessary as the executable will require `lasso_config.txt` which exists there and will also rely on the presence of the `images`, `data`, and potentially the `screenshots` folders.

The configuration file `lasso_config.txt` contains most of the parameters necessary for the simulation.  Provide 0 or 1 for Boolean values such as `gui` which controls whether the GUI is displayed or not.  The simulation will run at full speed if the GUI is not displayed.  Modify `renderSteps` to adjust the frequency of visual updates.

# Plots
Execute `plots.py` in `analysis_scripts` to generate plots of the simulation results stored in `data`.