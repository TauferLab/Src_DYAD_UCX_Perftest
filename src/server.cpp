#include "server.hpp"

#include <caliper/Annotation.h>
#include <caliper/cali.h>
#include <fcntl.h>

#include "base64.h"
#include "utils.h"
#include <ctime>
#include <algorithm>

extern const base64_maps_t base64_maps_rfc4648;

Server::Server (size_t data_size,
                const std::string& tcp_addr,
                int port,
                size_t num_connections,
                AbstractBackend* backend,
                cali::ConfigManager& mgr)
    : m_num_expected_connections (num_connections),
      m_connections_active (num_connections, false),
      m_all_active (false),
      m_all_unactive (true),
      m_data_size (data_size),
      m_data_buf (nullptr),
      m_oob_comm (nullptr),
      m_backend (backend),
      m_mgr (mgr)
{
    if (m_num_expected_connections == 0) {
        throw std::runtime_error ("Server configured with 0 expected connections");
    }
    m_oob_comm = new OOBComm (OOBComm::SERVER, tcp_addr, port, port+1);
}

Server::~Server ()
{
    shutdown ();
    delete m_oob_comm;
}

void Server::gen_data ()
{
    cali::Function server_gen_data_region ("Server::gen_data");
    m_data_buf = m_backend->get_net_buf ();
    int fd = open ("/dev/urandom", O_RDONLY);
    read (fd, m_data_buf, m_data_size);
    close (fd);
}

void Server::start ()
{
    cali::Function server_start_region ("Server::start");
    do {
        nlohmann::json msg = m_oob_comm->recv ();
        int client_rank = msg.at ("rank").get<int> ();
        int msg_type = msg.at ("msg_type").get<int> ();
        if (msg_type != 0)
            throw std::runtime_error ("Received invalid starting message from client");
        m_connections_active.at(client_rank) = true;
        nlohmann::json resp;
        resp["ok"] = true;
        m_oob_comm->send (resp);
        check_active_connections ();
    } while (!m_all_active);
    m_oob_comm->send_run_start ();
}

void Server::check_active_connections ()
{
    m_all_active = std::all_of(m_connections_active.begin(), m_connections_active.end(), [](bool v){ return v; });
    m_all_unactive = std::none_of(m_connections_active.begin(), m_connections_active.end(), [](bool v){ return v; });
}

void Server::run ()
{
    cali::Function server_run_region ("Server::run");
    int loop_id = 0;
    CALI_CXX_MARK_LOOP_BEGIN (server_run_loop_id, "server_run_loop");
    do {
        CALI_CXX_MARK_LOOP_ITERATION (server_run_loop_id, loop_id);
        single_run ("Server::single_run");
        check_active_connections ();
        loop_id += 1;
    } while (!m_all_unactive);
    CALI_CXX_MARK_LOOP_END (server_run_loop_id);
    DYAD_PERFTEST_INFO ("Server is done running", "");
}

// Utility function to do manual timinig as sanity checks
static double ts_diff(struct timespec* t1, struct timespec* t0)
{
     return (t1->tv_sec - t0->tv_sec)
      + (t1->tv_nsec - t0->tv_nsec) / 1000000000.0;
}

void Server::single_run (const char* region_name)
{
    cali::ScopeAnnotation single_run_region (region_name);
    nlohmann::json msg = m_oob_comm->recv ();
    nlohmann::json resp;
    int client_rank = msg.at ("rank").get<int> ();
    if (!m_connections_active.at(client_rank)) {
        throw std::runtime_error ("Received an extra message from a closed client connection");
    }
    int msg_type = msg.at ("msg_type").get<int> ();
    DYAD_PERFTEST_INFO ("Message type is {}", msg_type);
    if (msg_type == 1) {
        std::optional<ucp_tag_t> tag = msg.at ("tag").get<ucp_tag_t> ();
        DYAD_PERFTEST_INFO ("Tag is {}", *tag);
        if (*tag == 0)
            tag = std::nullopt;
        auto addr_element = msg.at ("addr");
        // nlohmann::json::array_t serialized_addr =
        // addr_element.at("bytes").get<nlohmann::json::array_t> ();
        std::string serialized_addr = addr_element.get<std::string> ();
        void* enc_addr = (void*)serialized_addr.data ();
        size_t enc_addr_size = msg.at ("addr_size").get<size_t> ();
        size_t addr_size = base64_decoded_length (enc_addr_size);
        ucp_address_t* addr = (ucp_address_t*)malloc (addr_size);
        if (addr == nullptr)
            throw std::runtime_error ("Could not allocate memory for remote address");
        ssize_t decode_ret = base64_decode_using_maps (&base64_maps_rfc4648,
                                                       (char*)addr,
                                                       addr_size,
                                                       (const char*)enc_addr,
                                                       enc_addr_size);
        if (decode_ret < 0) {
            free (addr);
            throw std::runtime_error ("Could not decode remote address");
        }
        gen_data();
        m_backend->set_remote_addr (addr, addr_size);
        m_backend->set_tag (tag);
        m_backend->establish_connection ();
        CALI_MARK_BEGIN ("full_backend_send");
        ucs_status_ptr_t stat_ptr = m_backend->send (m_data_buf, m_data_size);
        m_backend->comm_wait (stat_ptr);
        CALI_MARK_END ("full_backend_send");
        m_backend->close_connection ();
        m_backend->return_net_buf (&m_data_buf);
        resp["iter_ok"] = true;
    } else if (msg_type == 2) {
        m_connections_active.at(client_rank) = false;
        resp["disconnect_ok"] = true;
    } else {
        throw std::runtime_error ("Invalid message type");
    }
    m_oob_comm->send (resp);
}

void Server::shutdown ()
{
    cali::Function server_shutdown_region ("Server::shutdown");
    m_oob_comm->shutdown ();
    m_backend->shutdown ();
}