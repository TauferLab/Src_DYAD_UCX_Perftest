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

## Running the Performance Tester

The performance tester executable (`dyad_ucx_perftest`) mimics the client-server design of DYAD.

To run the tester, first, use the scheduler on your system to get a two node allocation. One node will
be used to run the server, while the other node is used to run the client.

To launch the server on the first node, run:

```bash
$ ./dyad_ucx_perftest <mode> <ip_addr> <data_size> -s
```

In the above command, `mode` can be one of the following:
* `tag`: use tag-matching send/recv from UCX

Additionally, `ip_addr` is the IP address and port to be used by the server. Finally, `data_size` is the size
of data to be transferred in bytes.

After launching the server, launch the client on the other node using:

```bash
$ ./dyad_ucx_perftest <mode> <ip_addr> <data_size> -n <num_iters>
```

The three positional arguments passed to the client invocation of `dyad_ucx_perftest` should be the same
as the ones passed to the server. The additional `num_iters` argumennt specifies the number of iterations
the client should run for. This is equivalent to the number of data transfers that will occur.

## Profiling/Tracing the Run with Caliper

To evaluate the performance of the UCX integration, this tester is annotated with Caliper. To enable the Caliper
annotations at runtime, a Caliper configuration must be provided via the `CALI_CONFIG` environment variable.
The recommended minimum configuration is:
```bash
CALI_CONFIG="event-trace,output=<client | server>.cali
```

This configuration will generate a Caliper trace of the client or server and dump the results into `client.cali`
or `server.cali`, depending on the value passed to `output`. Additional configuration options can be found
in the [Caliper docs](https://software.llnl.gov/Caliper/BuiltinConfigurations.html).

To analyze this profile/trace data, a playground Jupyter notebook is provided in the `analysis` directory.
This notebook leverages LLNL's [Thicket tool](https://thicket.readthedocs.io/en/latest/) for analysis of the profiles/traces.