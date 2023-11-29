#ifndef DYAD_UCX_PERFTEST_ABSTRACT_BACKEND_HPP
#define DYAD_UCX_PERFTEST_ABSTRACT_BACKEND_HPP

#include <ucp/api/ucp.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>

// Tag mask for UCX Tag send/recv
#define DYAD_UCX_TAG_MASK UINT64_MAX

// Macro function used to simplify checking the status
// of UCX operations
#define UCX_STATUS_FAIL(status) (status != UCS_OK)

extern "C" {
struct ucx_request {
    int completed;
};
typedef struct ucx_request ucx_request_t;

void ucx_request_init (void *request);

ucs_status_t ucx_request_wait (ucp_worker_h worker, ucx_request_t *request);
}

class UcxException : public std::exception
{
   public:
    UcxException (const char *msg) : m_msg (std::string (msg)), std::exception ()
    {
    }

    UcxException (const std::string &msg) : m_msg (msg), std::exception ()
    {
    }

    UcxException (std::string &&msg) : m_msg (msg), std::exception ()
    {
    }

    const char *what () const noexcept override
    {
        return m_msg.c_str ();
    };

   private:
    std::string m_msg;
};

class AbstractBackend
{
   public:
    enum CommMode {
        SEND,
        RECV,
        NONE,
    };

    AbstractBackend (CommMode mode);

    virtual ~AbstractBackend ();
    
    void init ();

    std::tuple<ucp_address_t *, size_t> get_address () const;

    std::optional<ucp_tag_t> get_tag () const;

    void set_remote_addr (ucp_address_t *remote_addr, size_t remote_addr_len);

    void set_tag (std::optional<ucp_tag_t> tag);

    virtual void establish_connection () = 0;

    virtual void send (void *buf, size_t buflen) = 0;

    virtual void recv (void **buf, size_t *buflen) = 0;

    virtual void close_connection () = 0;

    void shutdown ();

   protected:
    virtual void set_context_params (ucp_params_t *params) = 0;

    virtual void set_worker_params (ucp_worker_params_t *params) = 0;

    virtual void generate_tag (CommMode mode) = 0;

    bool m_initialized;

    ucp_context_h m_ctx;
    ucp_worker_h m_worker;
    ucp_ep_h m_remote_ep;
    CommMode m_mode;

    ucp_address_t *m_local_addr;
    size_t m_local_addr_size;
    ucp_address_t *m_remote_addr;
    size_t m_remote_addr_size;

    std::optional<ucp_tag_t> m_tag;
};

#endif /* DYAD_UCX_PERFTEST_ABSTRACT_BACKEND_HPP */