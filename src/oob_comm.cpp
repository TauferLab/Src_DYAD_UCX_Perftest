#include "oob_comm.hpp"

#include "utils.h"

#include <caliper/cali.h>
#include <caliper/Annotation.h>

#include <sstream>
#include <stdexcept>

static const char* connection_topic = "START";

OOBComm::OOBComm (Mode mode, const std::string& tcp_addr, int data_port, int connection_port)
    : m_open (false),
      m_mode (mode),
      m_data_sock (NULL),
      m_connection_sock (NULL)
{
    DYAD_PERFTEST_INFO ("Starting OOB communication on {}", tcp_addr_and_port);
    cali::Function oob_init_region ("OOBComm::init_zmq");
    std::string full_addr_connection = fmt::format ("tcp://{}:{}", tcp_addr, connection_port);
    std::string full_addr_data = fmt::format ("tcp://{}:{}", tcp_addr, data_port);
    if (m_mode == SERVER) {
        m_data_sock = zsock_new (ZMQ_REP);
        int rc = zsock_bind (m_data_sock, full_addr_data.c_str ());
        if (rc != data_port) {
            throw std::runtime_error ("ZMQ bound to incorrect TCP port for data control plane communication");
        }
        m_connection_sock = zsock_new_pub (full_addr_connection.c_str ());
        if (m_connection_sock == NULL) {
            throw std::runtime_error ("Failed to create ZMQ Pub socket for notifying clients of start");
        }
    } else if (m_mode == CLIENT) {
        m_data_sock = zsock_new (ZMQ_REQ);
        zsock_connect (m_data_sock, full_addr_data.c_str ());
        m_connection_sock = zsock_new_sub (full_addr_connection.c_str(), connection_topic);
        if (m_connection_sock == NULL) {
            throw std::runtime_error ("Failed to create ZMQ Sub socket for receiving notification of start");
        }
    } else {
        throw std::runtime_error ("Invalid OOB mode");
    }
    m_open = true;
}

OOBComm::~OOBComm ()
{
    shutdown ();
}

void OOBComm::send (const nlohmann::json& msg)
{
    DYAD_PERFTEST_INFO ("Sending OOB message", "");
    cali::Function oob_send_region ("OOBComm::send");
    std::ostringstream ss;
    ss << msg;
    std::string serialized_msg = ss.str ();
    zstr_send (m_data_sock, serialized_msg.c_str ());
}

void OOBComm::send_run_start ()
{
    zstr_send (m_connection_sock, connection_topic);
}

nlohmann::json OOBComm::recv ()
{
    DYAD_PERFTEST_INFO ("Receiving OOB message", "");
    cali::Function oob_recv_region ("OOBComm::recv");
    std::string serializecd_msg = zstr_recv (m_data_sock);
    std::istringstream ss (serializecd_msg);
    nlohmann::json msg;
    ss >> msg;
    return msg;
}

void OOBComm::recv_run_start ()
{
    std::string topic = zstr_recv (m_connection_sock);
    if (topic != std::string(connection_topic)) {
        throw std::runtime_error ("Received invalid start message from server");
    }
}

void OOBComm::shutdown ()
{
    cali::Function oob_shutdown_region ("OOBComm::shutdown");
    if (m_open) {
        zsock_destroy (&m_data_sock);
        zsock_destroy (&m_connection_sock);
        m_data_sock = nullptr;
        m_connection_sock = nullptr;
        m_open = false;
    }
}