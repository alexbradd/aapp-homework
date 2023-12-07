# Advanced Algorithms and Parallel Programming

This repository contains the material required to solve the AAPP challenge.
The following sections provide details about the repository structure, how to build the application, and how to execute it.

## Repository structure

We follow the streamlined approach for applications based on the CMake building system.
At the root level, we have the main `CMakeLists.txt` file that drives the compilation phases.
The actual source files are stored in the folder `src`.
For convenience, we also include a sample file with a few molecules `data/molecules.smi`.
To define a common way of executing the application, we provide a script in `scripts/launch.sh` that wraps the execution to set a fixed level of parallelism.


## How to compile the application

It follows the common CMake approach.
Assuming that the working directory is the repository root, it is enough to issue the following commands:

```bash
$ cmake -S . -B build
$ cmake --build build
```

## How to run the application

You can find the executable in the building directory.
For example, if the build directory is `build`, the executable will be `./build/main`.
The executable reads the input molecules from the standard input, writes intermediate results on the standard error, and prints the final table on the standard output.
For developing purposes, you can execute the application directly.
However, we will use the launcher script `./scripts/launch.sh` to run it. Make sure that it works properly.

For example, assuming that the working directory is in the repository root, and the building directory is `./build`, then, you can execute the application with the script:

```bash
$ ./scripts/launch.sh ./build/main ./data/molecules.smi output.csv 1
```

You will see the intermediate results on the terminal, while the final output is stored in the output.csv file.

> **NOTE**: the input file is very small and for developing purposes. You can find more datasets with a larger number of molecules here: [https://github.com/GLambard/Molecules_Dataset_Collection/tree/master](https://github.com/GLambard/Molecules_Dataset_Collection/tree/master)
