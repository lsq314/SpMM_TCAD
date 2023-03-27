#ifndef RCACHE
#define RCACHE

#include "core.h"

void request_to_rbanks(rcache_stream* request, id_t base_addr_rcsr,
        rcache_stream* bank1, rcache_stream* bank2, rcache_stream* bank3, rcache_stream* bank4
		);

void rcrequest_manager(rcache_stream* from, rcache_stream* rrequest_result,
		rresponse_stream* response, rcache_stream* to1, ap_uint<128>* B_rcsr);

void rcache(rcache_stream* request, id_t base_addr_rcsr, rresponse_stream* rres, id_t* access_number, id_t* hit_number,
		mi_stream* ms);

#endif
