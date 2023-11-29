#include "server.hpp"

#include "base64.h"

#include <fcntl.h>

extern const base64_maps_t base64_maps_rfc4648;

Server::Server (size_t data_size, const std::string& tcp_addr_and_port, AbstractBackend* backend)
    : m_data_size (data_size), m_data_buf (nullptr), m_oob_comm (nullptr), m_backend (backend)
{
    m_oob_comm = new OOBComm (OOBComm::SERVER, tcp_addr_and_port);
}

Server::~Server ()
{
    if (m_data_buf != nullptr) {
        free (m_data_buf);
        m_data_buf = nullptr;
    }
    shutdown ();
    delete m_oob_comm;
}

void Server::gen_data ()
{
    m_data_buf = malloc (m_data_size);
    int fd = open ("/dev/urandom", O_RDONLY);
    read (fd, m_data_buf, m_data_size);
    close (fd);
}

void Server::start ()
{
    nlohmann::json msg = m_oob_comm->recv ();
    int msg_type = msg.at ("msg_type").get<int> ();
    if (msg_type != 0)
        throw std::runtime_error ("Received invalid starting message from client");
    nlohmann::json resp;
    resp["ok"] = true;
    m_oob_comm->send (resp);
}

void Server::run ()
{
    int msg_type = 0;
    do {
        nlohmann::json msg = m_oob_comm->recv ();
        int msg_type = msg.at ("msg_type").get<int> ();
        if (msg_type == 1) {
            std::optional<ucp_tag_t> tag = msg.at ("tag").get<ucp_tag_t> ();
            if (*tag == 0)
                tag = std::nullopt;
            auto addr_element = msg.at ("addr");
            nlohmann::json::binary_t serialized_addr = addr_element.get<nlohmann::json::binary_t> ();
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
            m_backend->set_remote_addr (addr, addr_size);
            m_backend->set_tag (tag);
            m_backend->establish_connection ();
            m_backend->send (m_data_buf, m_data_size);
            m_backend->close_connection ();
            nlohmann::json resp;
            resp["iter_ok"] = true;
            m_oob_comm->send (resp);
        }
    } while (msg_type != 2);
}

void Server::shutdown ()
{
    m_oob_comm->shutdown ();
    m_backend->shutdown ();
}