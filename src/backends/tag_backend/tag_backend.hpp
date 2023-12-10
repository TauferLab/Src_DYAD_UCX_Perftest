#ifndef DYAD_UCX_PERFTEST_TAG_BACKEND_HPP
#define DYAD_UCX_PERFTEST_TAG_BACKEND_HPP

#include "abstract_backend.hpp"

class TagBackend : public AbstractBackend
{
   public:
    TagBackend (AbstractBackend::CommMode mode, size_t data_size, int rank);

    virtual ~TagBackend () = default;

    virtual void establish_connection (bool warmup=false) final;

    virtual ucs_status_ptr_t send (void *buf, size_t buflen) final;

    virtual ucs_status_ptr_t recv (void **buf, size_t *buflen) final;
    
    virtual void comm_wait (ucs_status_ptr_t stat_ptr) final;

    virtual void close_connection (bool warmup=false) final;

   protected:
    virtual void set_context_params (ucp_params_t *params) final;

    virtual void set_worker_params (ucp_worker_params_t *params) final;
};

#endif /* DYAD_UCX_PERFTEST_TAG_BACKEND_HPP */