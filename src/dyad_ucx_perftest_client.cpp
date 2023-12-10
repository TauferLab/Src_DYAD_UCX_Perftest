#include <caliper/cali.h>
#include <adiak.hpp>
#include <fmt/format.h>

#include <CLI/CLI.hpp>
#include <algorithm>
#include <cctype>

#include "client.hpp"
#include "server.hpp"
#include "tag_backend.hpp"

#include <mpi.h>

enum class Mode : int {
    TAG,
    NONE,
};

const std::map<std::string, Mode> mode_map{
    {"tag", Mode::TAG},
    {"none", Mode::NONE}
};

void set_metadata (Mode& m, size_t data_size, unsigned long num_iters)
{
    adiak::user ();
    adiak::executable ();
    adiak::executablepath ();
    adiak::hostname ();
    adiak::clustername ();
    adiak::walltime ();
    adiak::cputime ();
    adiak::systime ();
    adiak::jobsize ();
    adiak::numhosts ();
    adiak::hostlist ();
    auto key_it = std::find_if(mode_map.begin(), mode_map.end(), [&m](const auto& elem) { return elem.second == m; });
    if (key_it == mode_map.end())
        throw std::runtime_error("Invalid mode");
    adiak::value ("mode", key_it->first);
    adiak::value ("data_size", data_size);
    adiak::value ("num_iters", num_iters);
}

int main (int argc, char** argv)
{
    MPI_Init (&argc, &argv);
    int rank;
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm world_comm = MPI_COMM_WORLD;
    adiak::init (&world_comm);
    std::string tcp_addr;
    size_t data_size;
    unsigned long num_iters = 1;
    int port = 8888;
    Mode mode = Mode::TAG;
    CLI::App app;
    app.add_option ("mode", mode, "UCX backend mode to use")
        ->required ()
        ->transform (CLI::CheckedTransformer (mode_map, CLI::ignore_case));
    app.add_option ("tcp_address", tcp_addr, "TCP address for the server");
    app.add_option ("data_size", data_size, "Data size in bytes");
    app.add_option ("--num_iters,-n", num_iters, "Set the number of iterations (default = 1)")
        ->capture_default_str();
    app.add_option ("--port,-p", port, "TCP port for the server")
        ->capture_default_str();
    CLI11_PARSE (app, argc, argv);
    set_metadata (mode, data_size, num_iters);
    // Start Caliper annotations
    CALI_MARK_BEGIN ("main");
    AbstractBackend* backend;
    switch (mode) {
        case Mode::TAG:
            backend = new TagBackend (AbstractBackend::RECV, data_size, rank);
            break;
        default:
            throw std::runtime_error ("Invalid backend mode");
    }
    backend->init ();
    Client* client = new Client (rank, num_iters, data_size, tcp_addr, port, backend);
    try {
        client->start ();
        client->run ();
        client->shutdown ();
    } catch (UcxException& e) {
        fmt::print (stderr, "A UCX operation failed! Rethrowing error.\n");
        throw e;
    } catch (std::runtime_error& e) {
        fmt::print (stderr, "A non-UCX operation failed! Rethrowing error.\n");
        throw e;
    }
    delete client;
    delete backend;
    // End Caliper annotations
    CALI_MARK_END ("main");
    adiak::fini ();
    MPI_Finalize ();
    return 0;
}