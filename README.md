# DYAD UCX Perftest

This repo contains a testbed performance tester for the integration of UCX into DYAD.
It provides us an easy-to-edit codebase for testing implementations and configurations of the integration.

## Installing Dependencies

Most of the performance tester's dependencies are provided as git submodules. So, the first step to installing
the tester's dependencies is to run:

```bash
$ git submodule update --init --recursive
```

However, there are a few dependencies that are not submodules. For these, a `spack.yaml` file has been provided
to allow the rest of the dependencies to be easily built.

Finally, there are a couple of optional Python dependencies needed to run the code in the `analysis` directory.
These dependencies can be installed using the Conda environment file (`environment.yml`) provided in the root of the repo.

## Building the Performance Tester

The performance tester uses a standard CMake build and install process. So, the tester can be built and installed using:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<install prefix> ..
$ make [-j]
$ make install
```