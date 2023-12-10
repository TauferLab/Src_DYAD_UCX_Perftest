#include "client.hpp"

#include <caliper/Annotation.h>
#include <caliper/cali.h>

#include "base64.h"
#include "utils.h"
#include <ctime>
#include <mpi.h>

extern const base64_maps_t base64_maps_rfc4648;

Client::Client (int rank,
                unsigned long int num_iters,
                size_t data_size,
                const std::string& tcp_addr,
                int port,
                AbstractBackend* backend)
    : m_rank (rank),
      m_num_iters (num_iters),
      m_data_size (data_size),
      m_oob_comm (nullptr),
      m_backend (backend)
{
    m_oob_comm = new OOBComm (OOBComm::CLIENT, tcp_addr, port);
}

Client::~Client ()
{
    shutdown ();
    delete m_oob_comm;
}

void Client::start ()
{
    cali::Function start_region ("Client::start");
    nlohmann::json msg;
    msg["rank"] = m_rank;
    msg["msg_type"] = 0;
    m_oob_comm->send (msg);
    nlohmann::json response = m_oob_comm->recv ();
    if (!response.at ("ok").get<bool> ())
        throw std::runtime_error ("Did not get a valid response from server");
    MPI_Barrier (MPI_COMM_WORLD);
}

void Client::run ()
{
    cali::Function run_region ("Client::run");
    ucp_tag_t tag = m_backend->get_tag ();
    CALI_CXX_MARK_LOOP_BEGIN (client_run_loop_id, "client_run_loop");
    for (unsigned long int i = 0; i < m_num_iters; i++) {
        CALI_CXX_MARK_LOOP_ITERATION (client_run_loop_id, i);
        DYAD_PERFTEST_INFO ("Iteration {}", i);
        single_run (tag, "Client::single_run");
    }
    CALI_CXX_MARK_LOOP_END (client_run_loop_id);
    nlohmann::json shutdown_msg;
    shutdown_msg["rank"] = m_rank;
    shutdown_msg["msg_type"] = 2;
    m_oob_comm->send (shutdown_msg);
}

// Utility function to do manual timing as sanity checks
static double ts_diff(struct timespec* t1, struct timespec* t0)
{
     return (t1->tv_sec - t0->tv_sec)
      + (t1->tv_nsec - t0->tv_nsec) / 1000000000.0;
}

void Client::single_run (ucp_tag_t tag, const char* region_name)
{
    cali::ScopeAnnotation single_run_region (region_name);
    nlohmann::json msg;
    msg["rank"] = m_rank;
    msg["msg_type"] = 1;
    msg["tag"] = tag;
    DYAD_PERFTEST_INFO ("Message type is {}", msg.at ("msg_type").get<int> ());
    DYAD_PERFTEST_INFO ("Tag is {}", msg.at ("tag").get<ucp_tag_t> ());
    std::tuple<ucp_address_t*, size_t> addr_info = m_backend->get_address ();
    size_t enc_size = base64_encoded_length (std::get<1> (addr_info));
    void* enc_buf = malloc (enc_size + 1);
    if (enc_buf == nullptr)
        throw std::runtime_error ("Cannot allocate memory for local address to send to server");
    ssize_t enc_ret = base64_encode_using_maps (&base64_maps_rfc4648,
                                                (char*)enc_buf,
                                                enc_size + 1,
                                                (const char*)std::get<0> (addr_info),
                                                std::get<1> (addr_info));
    if (enc_ret < 0) {
        free (enc_buf);
        throw std::runtime_error ("Could not serialize the local address for server");
    }
    std::string serialized_buf ((const char*)enc_buf, ((const char*)enc_buf) + (enc_size + 1));
    // std::vector<uint8_t> std_buf ((uint8_t*)enc_buf, ((uint8_t*)enc_buf) + (enc_size + 1));
    // nlohmann::json::binary_t serialized_buf (std_buf);
    msg["addr"] = serialized_buf;
    msg["addr_size"] = enc_size;
    m_oob_comm->send (msg);
    void* data_buf = m_backend->get_net_buf ();
    size_t data_size;
    m_backend->establish_connection ();
    CALI_MARK_BEGIN ("full_backend_recv");
    ucs_status_ptr_t stat_ptr = m_backend->recv (&data_buf, &data_size);
    m_backend->comm_wait (stat_ptr);
    CALI_MARK_END ("full_backend_recv");
    if (data_size != m_data_size)
        throw std::runtime_error ("Got an incorrect data size on recv");
    m_backend->close_connection ();
    m_backend->return_net_buf (&data_buf);
    nlohmann::json response = m_oob_comm->recv ();
    if (!response.at ("iter_ok").get<bool> ())
        throw std::runtime_error ("Did not get a valid response from server for iteration");
}

void Client::shutdown ()
{
    cali::Function shutdown_region ("Client::shutdown");
    m_oob_comm->shutdown ();
    m_backend->shutdown ();
}