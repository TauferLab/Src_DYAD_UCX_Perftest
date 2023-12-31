#include "oob_comm.hpp"

#include "utils.h"

#include <caliper/cali.h>
#include <caliper/Annotation.h>

#include <sstream>
#include <stdexcept>

static const char* connection_topic = "START";

OOBComm::OOBComm (Mode mode, const std::string& tcp_addr, int port)
    : m_open (false),
      m_mode (mode),
      m_sock (NULL)
{
    DYAD_PERFTEST_INFO ("Starting OOB communication on {}", tcp_addr);
    cali::Function oob_init_region ("OOBComm::init_zmq");
    std::string full_addr = fmt::format ("tcp://{}:{}", tcp_addr, port);
    if (m_mode == SERVER) {
        m_sock = zsock_new (ZMQ_REP);
        int rc = zsock_bind (m_sock, full_addr.c_str ());
        if (rc != port) {
            throw std::runtime_error ("ZMQ bound to incorrect TCP port");
        }
    } else if (m_mode == CLIENT) {
        m_sock = zsock_new (ZMQ_REQ);
        zsock_connect (m_sock, full_addr.c_str ());
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
    zstr_send (m_sock, serialized_msg.c_str ());
}

nlohmann::json OOBComm::recv ()
{
    DYAD_PERFTEST_INFO ("Receiving OOB message", "");
    cali::Function oob_recv_region ("OOBComm::recv");
    std::string serializecd_msg = zstr_recv (m_sock);
    std::istringstream ss (serializecd_msg);
    nlohmann::json msg;
    ss >> msg;
    return msg;
}

void OOBComm::shutdown ()
{
    cali::Function oob_shutdown_region ("OOBComm::shutdown");
    if (m_open) {
        zsock_destroy (&m_sock);
        m_sock = nullptr;
        m_open = false;
    }
}