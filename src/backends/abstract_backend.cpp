#include "abstract_backend.hpp"

#include "utils.h"

#include <fmt/format.h>

#include <caliper/cali.h>
#include <caliper/Annotation.h>

#include <cassert>

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

AbstractBackend::AbstractBackend (AbstractBackend::CommMode mode, size_t data_size, int rank)
    : m_initialized (false),
      m_ctx (nullptr),
      m_worker (nullptr),
      m_remote_ep (nullptr),
      m_mode (mode),
      m_local_addr (nullptr),
      m_local_addr_size (0),
      m_remote_addr (nullptr),
      m_remote_addr_size (0),
      m_tag (rank),
      m_data_size(data_size)
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
    
    // OPTIMIZATION 1:
    // Allocate a single buffer of max transfer size, and reuse it for communications
#if OPT_1
    DYAD_PERFTEST_INFO ("Allocating memory buffer for optimization 1", "");
    ucp_mem_map_params_t mmap_params;
    mmap_params.field_mask = UCP_MEM_MAP_PARAM_FIELD_ADDRESS |
                            UCP_MEM_MAP_PARAM_FIELD_LENGTH |
                            UCP_MEM_MAP_PARAM_FIELD_FLAGS |
                            UCP_MEM_MAP_PARAM_FIELD_MEMORY_TYPE |
                            UCP_MEM_MAP_PARAM_FIELD_PROT;
    mmap_params.address = NULL;
    mmap_params.memory_type = UCS_MEMORY_TYPE_HOST;
    mmap_params.length = MAX_BUFFER_SIZE;
    mmap_params.flags = UCP_MEM_MAP_ALLOCATE;
    if (m_mode == SEND) {
        mmap_params.prot = UCP_MEM_MAP_PROT_LOCAL_READ;
    } else {
        mmap_params.prot = UCP_MEM_MAP_PROT_REMOTE_WRITE;
    }
    status = ucp_mem_map (m_ctx, &mmap_params, &m_map);
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException ("Failed to allocate transfer buffer with ucp_mem_map");
    }
    ucp_mem_attr_t attr;
    attr.field_mask = UCP_MEM_ATTR_FIELD_ADDRESS;
    status = ucp_mem_query(m_map, &attr);
    if (UCX_STATUS_FAIL (status)) {
        ucp_mem_unmap(m_ctx, m_map);
        throw UcxException ("Failed to get memory address allocated by ucp_mem_map");
    }
    m_net_buf = attr.address;
#endif
    
    // OPTIMIZATION 2:
    // Perform a tiny warmup communication with ourself to prevent extra hidden initialization costs from
    // UCX from impacting our data transfers
#if OPT_2
    DYAD_PERFTEST_INFO ("Running warmup communication", "");
    warmup ("Backend::warmup");
#endif
}

AbstractBackend::~AbstractBackend ()
{
}

std::tuple<ucp_address_t *, size_t> AbstractBackend::get_address () const
{
    return std::make_tuple (m_local_addr, m_local_addr_size);
}

ucp_tag_t AbstractBackend::get_tag () const
{
    return m_tag;
}

void* AbstractBackend::get_net_buf ()
{
    // OPTIMIZATION 1:
    // Use the globally allocated buffer from init
    cali::Function get_buf_region ("Backend::get_net_buf");
#if OPT_1
    return m_net_buf;
#else
    void* net_buf = malloc(m_data_size);
    return net_buf;
#endif
}

void AbstractBackend::return_net_buf(void** net_buf)
{
    // OPTIMIZATION 1:
    // Use the globally allocated buffer from init
    cali::Function ret_buf_region ("Backend::return_net_buf");
#if OPT_1
    return;
#else
    if (net_buf == NULL || *net_buf == NULL)
        return;
    free (*net_buf);
#endif
}

void AbstractBackend::set_remote_addr (ucp_address_t *remote_addr, size_t remote_addr_len)
{
    if (remote_addr != nullptr) {
        m_remote_addr = remote_addr;
        m_remote_addr_size = remote_addr_len;
    }
}

void AbstractBackend::set_tag (ucp_tag_t tag)
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
        // OPTIMIZATION 1:
        // Persistant buffer needs to be deallocated using ucp_mem_unmap
#if OPT_1
        if (m_net_buf != nullptr) {
            ucp_mem_unmap (m_ctx, m_map);
            m_net_buf = nullptr;
            m_map = nullptr;
        }
#endif
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
        m_initialized = false;
    }
}

// OPTIMIZATION 2:
// Perform a tiny warmup communication with ourself to prevent extra hidden initialization costs from
// UCX from impacting our data transfers
#if OPT_2
void AbstractBackend::warmup (const char* region_name)
{
    cali::Function warmup_function (region_name);
    void* data_buf = get_net_buf ();
    void* recv_buf = malloc (1);
    size_t recv_size = 0;
    std::tuple<ucp_address_t*, size_t> addr_info = get_address ();
    set_remote_addr (std::get<0>(addr_info), std::get<1>(addr_info));
    establish_connection (true);
    ucs_status_ptr_t send_stat_ptr = send (data_buf, 1);
    ucs_status_ptr_t recv_stat_ptr = recv (&recv_buf, &recv_size);
    comm_wait (recv_stat_ptr);
    comm_wait (send_stat_ptr);
    close_connection (true);
    assert (recv_size == 1);
    free (recv_buf);
    return_net_buf (&data_buf);
}
#endif