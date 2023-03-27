#ifndef SPMM_VCCACHE
#define SPMM_VCCACHE

#include "core.h"

void vcrequest_manager(B_request_stream* from, vcrequest_stream* vrequest_result, vcrequest_stream* crequest_result,
		vcresponse_stream* vresponse, vcresponse_stream* cresponse, cacheline_stream* vresult, cacheline_stream* cresult,
		block_stream* to1, block_stream* to2, uint32_t deviceID, ap_uint<128>* B_val, ap_uint<128>* B_cid);

void request_to_vcbanks(vcrequest_stream* request, id_t base_addr_val, id_t base_addr_cid,
        vcrequest_stream* bank1, vcrequest_stream* bank2);

void vcache(vcrequest_stream* request, id_t base_addr_val, vcresponse_stream* vres,
		vcresult_stream* vcresult, id_t* access_number, id_t* hit_number);

void ccache(vcrequest_stream* request, id_t base_addr_cid,
		vcresponse_stream* cres, vcresult_stream* vcresult, id_t* access_number, id_t* hit_number);

#endif
