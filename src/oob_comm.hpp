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

    OOBComm (Mode mode, const std::string& tcp_addr, int data_port, int connection_port);

    ~OOBComm ();

    void send (const nlohmann::json& msg);
    
    void send_run_start ();

    nlohmann::json recv ();

    void recv_run_start ();

    void shutdown ();

   private:
    bool m_open;
    Mode m_mode;
    zsock_t* m_data_sock;
    zsock_t* m_connection_sock;
};

#endif /* DYAD_UCX_PERFTEST_OOB_COMM_HPP */