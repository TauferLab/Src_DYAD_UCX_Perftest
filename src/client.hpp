#ifndef DYAD_UCX_PERFTEST_CLIENT_HPP
#define DYAD_UCX_PERFTEST_CLIENT_HPP

#include "abstract_backend.hpp"
#include "oob_comm.hpp"

class Client
{
   public:
    Client (int rank,
            unsigned long int num_iters,
            size_t data_size,
            const std::string& tcp_addr,
            int port,
            AbstractBackend* backend);

    ~Client ();

    void start ();

    void run ();

    void shutdown ();

   private:
    void single_run(ucp_tag_t tag, const char* region_name);

    int m_rank;
    unsigned long int m_num_iters;
    size_t m_data_size;
    OOBComm* m_oob_comm;
    AbstractBackend* m_backend;
};

#endif /* DYAD_UCX_PERFTEST_CLIENT_HPP */