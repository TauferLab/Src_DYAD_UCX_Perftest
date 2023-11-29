#include "abstract_backend.hpp"

#include <fmt/format.h>

#include <caliper/cali.h>
#include <caliper/Annotation.h>

extern "C" {
    void ucx_request_init (void *request)
    {
        ucx_request_t *real_request = NULL;
        real_request = (ucx_request_t *)request;
        real_request->completed = 0;
    }
    
    ucs_status_t ucx_request_wait (ucp_worker_h worker, ucx_request_t *request)
    {
        CALI_MARK_BEGIN ("ucx_request_wait");
        ucs_status_t final_request_status = UCS_OK;
        if (UCS_PTR_IS_PTR (request)) {
            do {
                ucp_worker_progress (worker);
                // usleep(100);
                // Get the final status of the communication operation
                final_request_status = ucp_request_check_status (request);
            } while (final_request_status == UCS_INPROGRESS);
            // Free and deallocate the request object
            ucp_request_free (request);
            goto dtl_ucx_request_wait_region_finish;
        } else if (UCS_PTR_IS_ERR (request)) {
            final_request_status = UCS_PTR_STATUS (request);
            goto dtl_ucx_request_wait_region_finish;
        }
        final_request_status = UCS_OK;
dtl_ucx_request_wait_region_finish:
        CALI_MARK_END ("ucx_request_wait");
        return final_request_status;
    }
}

AbstractBackend::AbstractBackend (AbstractBackend::CommMode mode)
    : m_initialized (false),
      m_ctx (nullptr),
      m_worker (nullptr),
      m_remote_ep (nullptr),
      m_mode (mode),
      m_local_addr (nullptr),
      m_local_addr_size (0),
      m_remote_addr (nullptr),
      m_remote_addr_size (0),
      m_tag (std::nullopt)
{
}

void AbstractBackend::init ()
{
    cali::Function ucx_init ("Backend::init");
    ucp_params_t ucx_params;
    ucp_worker_params_t worker_params;
    ucp_config_t *config;
    ucs_status_t status;
    ucp_worker_attr_t worker_attrs;

    // Read the UCX configuration
    status = ucp_config_read (NULL, NULL, &config);
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException ("Could not read the UCX config");
    }

    // Define the settings, parameters, features, etc.
    // for the UCX context. UCX will use this info internally
    // when creating workers, endpoints, etc.
    set_context_params (&ucx_params);

    // Initialize UCX
    status = ucp_init (&ucx_params, config, &m_ctx);

    // If in debug mode, print the configuration of UCX to stderr
    // if (debug) {
    //     ucp_config_print (config, stderr, "UCX Configuration", UCS_CONFIG_PRINT_CONFIG);
    // }
    // Release the config
    ucp_config_release (config);
    // Log an error if UCX initialization failed
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException (fmt::format ("ucp_init failed (status = {})", (int)status));
    }

    // Define the settings for the UCX worker (i.e., progress engine)
    set_worker_params (&worker_params);

    // Create the worker and log an error if that fails
    status = ucp_worker_create (m_ctx, &worker_params, &m_worker);
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException (fmt::format ("ucp_worker_create failed (status = {})", (int)status));
    }

    // Query the worker for its address
    worker_attrs.field_mask = UCP_WORKER_ATTR_FIELD_ADDRESS;
    status = ucp_worker_query (m_worker, &worker_attrs);
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException (fmt::format ("Cannot get UCX worker address (status = {})", (int)status));
    }
    m_local_addr = worker_attrs.address;
    m_local_addr_size = worker_attrs.address_length;
    generate_tag (m_mode);
}

AbstractBackend::~AbstractBackend ()
{
}

std::tuple<ucp_address_t *, size_t> AbstractBackend::get_address () const
{
    return std::make_tuple (m_local_addr, m_local_addr_size);
}

std::optional<ucp_tag_t> AbstractBackend::get_tag () const
{
    return m_tag;
}

void AbstractBackend::set_remote_addr (ucp_address_t *remote_addr, size_t remote_addr_len)
{
    if (remote_addr != nullptr) {
        m_remote_addr = remote_addr;
        m_remote_addr_size = remote_addr_len;
    }
}

void AbstractBackend::set_tag (std::optional<ucp_tag_t> tag)
{
    m_tag = tag;
}

void AbstractBackend::shutdown ()
{
    cali::Function shutdown_ucx_region ("Backend::shutdown");
    if (m_initialized) {
        if (m_remote_ep != nullptr) {
            close_connection ();
            m_remote_ep = nullptr;
        }
        // Release local address if not already released
        if (m_local_addr != nullptr) {
            ucp_worker_release_address (m_worker, m_local_addr);
            m_local_addr = nullptr;
            m_local_addr_size = 0;
        }
        // Release worker if not already released
        if (m_worker != nullptr) {
            ucp_worker_destroy (m_worker);
            m_worker = nullptr;
        }
        // Release context if not already released
        if (m_ctx != nullptr) {
            ucp_cleanup (m_ctx);
            m_ctx = nullptr;
        }
        m_tag = std::nullopt;
        m_initialized = false;
    }
}