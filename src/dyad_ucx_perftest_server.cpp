#include <caliper/cali.h>
#include <adiak.hpp>
#include <fmt/format.h>

#include <CLI/CLI.hpp>
#include <algorithm>
#include <cctype>

#include "client.hpp"
#include "server.hpp"
#include "tag_backend.hpp"

enum class Mode : int {
    TAG,
    NONE,
};

const std::map<std::string, Mode> mode_map{
    {"tag", Mode::TAG},
    {"none", Mode::NONE}
};

void set_metadata (Mode& m, size_t data_size, size_t num_connections)
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
    adiak::value ("num_connections", num_connections);
}

int main (int argc, char** argv)
{
    adiak::init (nullptr);
    std::string tcp_addr;
    size_t data_size;
    int port = 8888;
    size_t num_connections = 0;
    Mode mode = Mode::TAG;
    CLI::App app;
    app.add_option ("mode", mode, "UCX backend mode to use")
        ->required ()
        ->transform (CLI::CheckedTransformer (mode_map, CLI::ignore_case));
    app.add_option ("tcp_address", tcp_addr, "TCP address for the server");
    app.add_option ("data_size", data_size, "Data size in bytes");
    app.add_option ("num_connections", num_connections, "Number of client connections");
    app.add_option ("--port,-p", port, "TCP port for the server")
        ->capture_default_str();
    CLI11_PARSE (app, argc, argv);
    set_metadata (mode, data_size, num_connections);
    // Start Caliper annotations
    CALI_MARK_BEGIN ("main");
    AbstractBackend* backend;
    switch (mode) {
        case Mode::TAG:
            backend = new TagBackend (AbstractBackend::SEND, data_size, 0);
            break;
        default:
            throw std::runtime_error ("Invalid backend mode");
    }
    backend->init ();
    Server* serv = new Server (data_size, tcp_addr, port, num_connections, backend);
    try {
        serv->start ();
        serv->run ();
        serv->shutdown ();
    } catch (UcxException& e) {
        fmt::print (stderr, "A UCX operation failed! Rethrowing error.\n");
        throw e;
    } catch (std::runtime_error& e) {
        fmt::print (stderr, "A non-UCX operation failed! Rethrowing error.\n");
        throw e;
    }
    delete serv;
    delete backend;
    // End Caliper annotations
    CALI_MARK_END ("main");
    adiak::fini ();
    return 0;
}