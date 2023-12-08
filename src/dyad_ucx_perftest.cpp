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

int server_main (const Mode& mode,
                 const std::string& tcp_addr,
                 const size_t data_size,
                 cali::ConfigManager& mgr)
{
    AbstractBackend* backend;
    switch (mode) {
        case Mode::TAG:
            backend = new TagBackend (AbstractBackend::SEND, data_size);
            break;
        default:
            throw std::runtime_error ("Invalid backend mode");
    }
    backend->init ();
    Server* serv = new Server (data_size, tcp_addr, backend, mgr);
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

int client_main (const Mode& mode,
                 const std::string& tcp_addr,
                 const size_t data_size,
                 unsigned long int num_iters,
                 cali::ConfigManager& mgr)
{
    AbstractBackend* backend;
    switch (mode) {
        case Mode::TAG:
            backend = new TagBackend (AbstractBackend::RECV, data_size);
            break;
        default:
            throw std::runtime_error ("Invalid backend mode");
    }
    backend->init ();
    Client* client = new Client (num_iters, data_size, tcp_addr, backend, mgr);
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
    return 0;
}

int main (int argc, char** argv)
{
    cali::ConfigManager mgr;
    CALI_CXX_MARK_FUNCTION;
    // std::string mode;
    std::string tcp_addr;
    size_t data_size;
    bool is_server = false;
    unsigned long num_iters = 1;
    Mode mode = Mode::TAG;
    std::map<std::string, Mode> mode_map{{"tag", Mode::TAG}, {"none", Mode::NONE}};
    CLI::App app;
    app.add_option ("mode", mode, "UCX backend mode to use")
        ->required ()
        ->transform (CLI::CheckedTransformer (mode_map, CLI::ignore_case));
    app.add_option ("tcp_address", tcp_addr, "TCP address and port for the server");
    app.add_option ("data_size", data_size, "Data size in bytes");
    app.add_flag ("--server,-s", is_server, "If provided, run as server");
    app.add_option ("--num_iters,-n", num_iters, "Set the number of iterations (default = 1)");
    CLI11_PARSE (app, argc, argv);
    if (is_server) {
        return server_main (mode, tcp_addr, data_size, mgr);
    }
    return client_main (mode, tcp_addr, data_size, num_iters, mgr);
}