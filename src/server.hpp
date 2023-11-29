#ifndef DYAD_UCX_PERFTEST_SERVER_HPP
#define DYAD_UCX_PERFTEST_SERVER_HPP

#include "abstract_backend.hpp"
#include "oob_comm.hpp"

class Server
{
   public:
    Server (size_t data_size, const std::string& tcp_addr_and_port, AbstractBackend* backend);

    ~Server ();

    void gen_data ();

    void start ();

    void run ();

    void shutdown ();

   private:
    size_t m_data_size;
    void* m_data_buf;
    OOBComm* m_oob_comm;
    AbstractBackend* m_backend;
};

#endif /* DYAD_UCX_PERFTEST_SERVER_HPP */