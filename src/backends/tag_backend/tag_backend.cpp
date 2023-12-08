#include "tag_backend.hpp"

#include <fmt/format.h>

#include <random>
#include <unistd.h>

#include <caliper/cali.h>
#include <caliper/Annotation.h>

#include "utils.h"

void ucx_ep_err_handler (void *arg, ucp_ep_h ep, ucs_status_t status)
{
    DYAD_PERFTEST_ERROR ("An error occured on the UCP endpoint (status = {})\n", (int)status);
}

void send_callback (void *req, ucs_status_t status, void* user_data)
{
    DYAD_PERFTEST_INFO ("Invoking the UCX send callback for tag backend (status = {})\n", (int)status);
    ucx_request_t *real_req = (ucx_request_t *)req;
    real_req->completed = 1;
}

void recv_callback (void *request, ucs_status_t status, const ucp_tag_recv_info_t *tag_info, void* user_data)
{
    DYAD_PERFTEST_INFO ("Invoking the UCX recv callback for tag backend (status = {})\n", (int)status);
    ucx_request_t *real_request = NULL;
    real_request = (ucx_request_t *)request;
    real_request->completed = 1;
}

TagBackend::TagBackend (AbstractBackend::CommMode mode, size_t data_size) : AbstractBackend (mode, data_size)
{
}

void TagBackend::establish_connection (bool warmup)
{
    DYAD_PERFTEST_INFO ("Establishing connection with other UCX endpoint\n", "");
    cali::Function establish_connection_region("TagBackend::establish_connection");
    ucp_ep_params_t params;
    ucs_status_t status = UCS_OK;
#if OPTIMIZATION_2
    if (warmup || m_mode == SEND) {
#else
    if (m_mode == SEND) {
#endif
        params.field_mask = UCP_EP_PARAM_FIELD_REMOTE_ADDRESS | UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE
                            | UCP_EP_PARAM_FIELD_ERR_HANDLER;
        params.address = m_remote_addr;
        params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
        params.err_handler.cb = ucx_ep_err_handler;
        params.err_handler.arg = NULL;
        status = ucp_ep_create (m_worker, &params, &m_remote_ep);
        if (UCX_STATUS_FAIL (status)) {
            throw UcxException (fmt::format ("ucp_ep_create failed with status {}", (int)status));
        }
        // if (dtl_handle->debug) {
        //     ucp_ep_print_info (dtl_handle->ep, stderr);
        // }
    } else if (m_mode == RECV) {
        // FLUX_DYAD_PERFTEST_INFO (dtl_handle->h,
        //                "No explicit connection establishment needed for UCX "
        //                "receiver\n");
    } else {
        throw std::runtime_error (fmt::format ("Invalid communication mode: {}", (int)m_mode));
    }
}

ucs_status_ptr_t TagBackend::send (void *buf, size_t buflen)
{
    DYAD_PERFTEST_INFO ("Sending {} bytes of data using UCX Tag Send\n", buflen);
    cali::Function send_region("TagBackend::send");
    ucs_status_ptr_t stat_ptr;
    ucs_status_t status = UCS_OK;
    ucx_request_t *req = nullptr;
    if (m_remote_ep == nullptr) {
        throw UcxException ("UCP endpoint was not created prior to invoking send!");
    }
    if (!m_tag) {
        throw std::runtime_error ("Tag not set prior to invoking send!");
    }
    ucp_request_param_t params;
    params.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                          UCP_OP_ATTR_FIELD_DATATYPE;
    params.cb.send = send_callback;
    params.datatype = ucp_dt_make_contig(1);
    stat_ptr = ucp_tag_send_nbx (m_remote_ep, buf, buflen, *m_tag, &params);
    return stat_ptr;
}

ucs_status_ptr_t TagBackend::recv (void **buf, size_t *buflen)
{
    DYAD_PERFTEST_INFO ("Receiving data using UCX Tag Recv", "");
    cali::Function send_region("TagBackend::recv");
    ucs_status_t status = UCS_OK;
    ucp_tag_message_h msg = NULL;
    ucp_tag_recv_info_t msg_info;
    ucs_status_ptr_t stat_ptr = NULL;
    if (!m_tag) {
        throw std::runtime_error ("Tag not set prior to invoking recv!");
    }
    CALI_MARK_BEGIN ("ucp_tag_probe");
    do {
        ucp_worker_progress (m_worker);
        msg = ucp_tag_probe_nb (m_worker,
                                *m_tag,
                                DYAD_UCX_TAG_MASK,
                                1,  // Remove the message from UCP tracking
                                // Requires calling ucp_tag_msg_recv_nb
                                // with the ucp_tag_message_h to retrieve message
                                &msg_info);
        usleep (10);
    } while (msg == nullptr);
    CALI_MARK_END ("ucp_tag_probe");
    // CALI_MARK_BEGIN ("recv_buffer_alloc");
    *buflen = msg_info.length;
    // *buf = malloc (*buflen);
    // CALI_MARK_END ("recv_buffer_alloc");
    if (*buf == nullptr) {
        throw std::runtime_error ("Could not allocate memory for buffer");
    }
    CALI_MARK_BEGIN ("ucp_tag_msg_recv");
    ucp_request_param_t recv_params;
    recv_params.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                               UCP_OP_ATTR_FIELD_DATATYPE;
    recv_params.cb.recv = recv_callback;
    recv_params.datatype = ucp_dt_make_contig(1);
    stat_ptr = ucp_tag_msg_recv_nbx (m_worker, *buf, *buflen, msg, &recv_params);
    CALI_MARK_END ("ucp_tag_msg_recv");
    return stat_ptr;
}

void TagBackend::comm_wait (ucs_status_ptr_t stat_ptr)
{
    ucs_status_t status = ucx_request_wait (m_worker, (ucx_request_t*)stat_ptr);
    if (UCX_STATUS_FAIL (status)) {
        throw UcxException (fmt::format ("UCP tag communication failed (status = {})", (int)status));
    }
}

void TagBackend::close_connection (bool warmup)
{
    DYAD_PERFTEST_INFO ("Closing UCX connection", "");
    cali::Function close_connection_region("TagBackend::close_connection");
    ucs_status_t status = UCS_OK;
    ucs_status_ptr_t stat_ptr;
#if OPTIMIZATION_2
    if (warmup || m_mode == SEND) {
#else
    if (m_mode == SEND) {
#endif
        if (m_remote_ep != NULL) {
            ucp_request_param_t close_params;
            close_params.op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS;
            close_params.flags = UCP_EP_CLOSE_FLAG_FORCE;
            stat_ptr = ucp_ep_close_nbx (m_remote_ep, &close_params);
            if (stat_ptr != NULL) {
                if (UCS_PTR_IS_PTR (stat_ptr)) {
                    do {
                        ucp_worker_progress (m_worker);
                        status = ucp_request_check_status (stat_ptr);
                    } while (status == UCS_INPROGRESS);
                    ucp_request_free (stat_ptr);
                } else {
                    status = UCS_PTR_STATUS (stat_ptr);
                }
                if (UCX_STATUS_FAIL (status)) {
                    throw UcxException (
                        fmt::format ("Could not successfully close endpoint (status = {}", (int)status));
                }
            }
            m_remote_ep = nullptr;
        }
#if OPTIMIZATION_2
        if ((!warmup || m_mode != RECV) && m_remote_addr != nullptr) {
#else
        if (m_remote_addr != nullptr) {
#endif
            free (m_remote_addr);
            m_remote_addr = nullptr;
            m_remote_addr_size = 0;
        }
    } else if (m_mode == RECV) {
    } else {
        throw std::runtime_error ("Somehow, an invalid comm mode reached 'close_connection'");
    }
}

void TagBackend::set_context_params (ucp_params_t *params)
{
    params->field_mask =
        UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_REQUEST_SIZE | UCP_PARAM_FIELD_REQUEST_INIT;
    params->features = UCP_FEATURE_TAG | UCP_FEATURE_WAKEUP;
    params->request_size = sizeof (struct ucx_request);
    params->request_init = ucx_request_init;
}

void TagBackend::set_worker_params (ucp_worker_params_t *params)
{
    params->field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE | UCP_WORKER_PARAM_FIELD_EVENTS;
    params->thread_mode = UCS_THREAD_MODE_SINGLE;
    params->events = UCP_WAKEUP_TAG_RECV;
}

void TagBackend::generate_tag (CommMode mode)
{
    if (mode == RECV) {
        m_tag = 123241;
    } else {
        m_tag = std::nullopt;
    }
}