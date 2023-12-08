#ifndef DYAD_UCX_PERFTEST_SERVER_HPP
#define DYAD_UCX_PERFTEST_SERVER_HPP

#include "abstract_backend.hpp"
#include "oob_comm.hpp"
#include <caliper/cali-manager.h>

class Server
{
   public:
    Server (size_t data_size,
            const std::string& tcp_addr_and_port,
            AbstractBackend* backend,
            cali::ConfigManager& mgr);

    ~Server ();

    void start ();

    void run ();

    void shutdown ();

   private:
    void single_run(int& msg_type, const char* region_name);

    void gen_data ();

    size_t m_data_size;
    void* m_data_buf;
    OOBComm* m_oob_comm;
    AbstractBackend* m_backend;
    cali::ConfigManager& m_mgr;
};

#endif /* DYAD_UCX_PERFTEST_SERVER_HPP */