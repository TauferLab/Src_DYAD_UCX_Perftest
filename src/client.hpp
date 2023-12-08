#ifndef DYAD_UCX_PERFTEST_CLIENT_HPP
#define DYAD_UCX_PERFTEST_CLIENT_HPP

#include "abstract_backend.hpp"
#include "oob_comm.hpp"
#include <caliper/cali-manager.h>

class Client
{
   public:
    Client (unsigned long int num_iters,
            size_t data_size,
            const std::string& tcp_addr_and_port,
            AbstractBackend* backend,
            cali::ConfigManager& mgr);

    ~Client ();

    void start ();

    void run ();

    void shutdown ();

   private:
    void single_run(ucp_tag_t tag, const char* region_name);

    unsigned long int m_num_iters;
    size_t m_data_size;
    OOBComm* m_oob_comm;
    AbstractBackend* m_backend;
    cali::ConfigManager& m_mgr;
};

#endif /* DYAD_UCX_PERFTEST_CLIENT_HPP */