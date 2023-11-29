#ifndef DYAD_UCX_PERFTEST_TAG_BACKEND_HPP
#define DYAD_UCX_PERFTEST_TAG_BACKEND_HPP

#include "abstract_backend.hpp"

class TagBackend : public AbstractBackend
{
   public:
    TagBackend (AbstractBackend::CommMode mode);

    virtual ~TagBackend () = default;

    virtual void establish_connection () final;

    virtual void send (void *buf, size_t buflen) final;

    virtual void recv (void **buf, size_t *buflen) final;

    virtual void close_connection () final;

   protected:
    virtual void set_context_params (ucp_params_t *params) final;

    virtual void set_worker_params (ucp_worker_params_t *params) final;

    virtual void generate_tag (CommMode mode) final;
};

#endif /* DYAD_UCX_PERFTEST_TAG_BACKEND_HPP */