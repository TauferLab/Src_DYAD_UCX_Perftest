#ifndef DYAD_UCX_PERFTEST_OOB_COMM_HPP
#define DYAD_UCX_PERFTEST_OOB_COMM_HPP

#include <czmq.h>

#include <nlohmann/json.hpp>

class OOBComm
{
   public:
    enum Mode {
        CLIENT,
        SERVER,
        NONE,
    };

    OOBComm (Mode mode, const std::string& tcp_addr_and_port);

    ~OOBComm ();

    void send (const nlohmann::json& msg);

    nlohmann::json recv ();

    void shutdown ();

   private:
    bool m_open;
    Mode m_mode;
    zsock_t* m_sock;
};

#endif /* DYAD_UCX_PERFTEST_OOB_COMM_HPP */