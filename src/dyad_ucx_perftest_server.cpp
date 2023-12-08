#include <caliper/cali-manager.h>
#include <caliper/cali.h>
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

int main (int argc, char** argv)
{
    cali::ConfigManager mgr;
    CALI_CXX_MARK_FUNCTION;
    std::string tcp_addr;
    size_t data_size;
    int port = 8888;
    size_t num_connections = 0;
    Mode mode = Mode::TAG;
    std::map<std::string, Mode> mode_map{{"tag", Mode::TAG}, {"none", Mode::NONE}};
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
    AbstractBackend* backend;
    switch (mode) {
        case Mode::TAG:
            backend = new TagBackend (AbstractBackend::SEND, data_size);
            break;
        default:
            throw std::runtime_error ("Invalid backend mode");
    }
    backend->init ();
    Server* serv = new Server (data_size, tcp_addr, port, num_connections, backend, mgr);
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
    return 0;
}