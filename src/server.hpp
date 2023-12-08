#ifndef DYAD_UCX_PERFTEST_SERVER_HPP
#define DYAD_UCX_PERFTEST_SERVER_HPP

#include "abstract_backend.hpp"
#include "oob_comm.hpp"
#include <caliper/cali-manager.h>

class Server
{
   public:
    Server (size_t data_size,
            const std::string& tcp_addr,
            int port,
            size_t num_connections,
            AbstractBackend* backend,
            cali::ConfigManager& mgr);

    ~Server ();

    void start ();

    void run ();

    void shutdown ();

   private:
    void single_run(const char* region_name);

    void gen_data ();
    
    void check_active_connections ();
    
    size_t m_num_expected_connections;
    std::vector<bool> m_connections_active;
    bool m_all_active;
    bool m_all_unactive;

    size_t m_data_size;
    void* m_data_buf;
    OOBComm* m_oob_comm;
    AbstractBackend* m_backend;
    cali::ConfigManager& m_mgr;
};

#endif /* DYAD_UCX_PERFTEST_SERVER_HPP */