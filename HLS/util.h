#ifndef UTIL
#define UTIL

#include "core.h"

void merge_4_to_1_rrequest(rcache_stream* cache_result1, rcache_stream* cache_result2, rcache_stream* cache_result3,
		rcache_stream* cache_result4, rcache_stream* cache_result);

void distributer_rresponse(rresponse_stream* from1, rresponse_stream* to1, rresponse_stream* to2, rresponse_stream* to3, rresponse_stream* to4);

void find_eviction(lru_struct in1, lru_struct in2, lru_struct in3, lru_struct in4,
		lru_struct in5, lru_struct in6, lru_struct in7, lru_struct in8,
		lru_struct in9, lru_struct in10, lru_struct in11, lru_struct in12,
		lru_struct in13, lru_struct in14, lru_struct in15, lru_struct in16, id_t* minimum);

void merge_8_to_1_rcache(rresponse_stream* from1, rresponse_stream* from2, rresponse_stream* from3,
		rresponse_stream* from4, rresponse_stream* from5, rresponse_stream* from6, rresponse_stream* from7,
		rresponse_stream* from8, rresponse_stream* to);

void from_rcache_result_to_B_vcrequest(rcache_stream* from1, B_request_stream* to1, B_request_stream* to2, id_stream* B_len);

void merge_4_to_1_vcrequest(vcrequest_stream* cache_result1, vcrequest_stream* cache_result2, vcrequest_stream* cache_result3,
		vcrequest_stream* cache_result4, vcrequest_stream* cache_result);

void distributer_vcresult(vcresult_stream* from1, B_data_stream* val1, B_data_stream* val2, B_data_stream* val3, B_data_stream* val4,
		B_data_stream* cid1, B_data_stream* cid2, B_data_stream* cid3, B_data_stream* cid4);

void distributer_vcresult(vcresult_stream* from1, cacheline_stream* val1, cacheline_stream* val2, cacheline_stream* val3, cacheline_stream* val4);

void distributer_vcresult(vcresult_stream* from1, vcresult_stream* val1, vcresult_stream* val2, vcresult_stream* val3, vcresult_stream* val4);

void distributer_vcresponse(vcresponse_stream* from1, vcresponse_stream* to1, vcresponse_stream* to2, vcresponse_stream* to3, vcresponse_stream* to4);

void merge_2_to_1_vcresponse(vcresponse_stream* from1, vcresponse_stream* from2, vcresponse_stream* to);

void merge_8_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* from3,
		vcresult_stream* from4, vcresult_stream* from5, vcresult_stream* from6, vcresult_stream* from7,
		vcresult_stream* from8, vcresult_stream* to);

void merge_4_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* from3,
		vcresult_stream* from4, cacheline_stream* to);

void merge_2_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, cacheline_stream* to);

void merge_2_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* to);

void merge_4_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* from3,
		vcresult_stream* from4, vcresult_stream* to);

void merge_4_to_1_vcresponse(vcresponse_stream* from1, vcresponse_stream* from2, vcresponse_stream* from3,
		vcresponse_stream* from4, vcresponse_stream* to);

void merge_4_to_1_rresponse(rresponse_stream* from1, rresponse_stream* from2, rresponse_stream* from3,
		rresponse_stream* from4, rresponse_stream* to);

void convert_pblock_to_mergeitem(partial_stream* from, merge_stream* to);

void final_merger(merge_stream* from1, merge_stream* from2, merge_stream* result, id_stream* result_len, id_stream* result_len1);

void sub_final_merger(merge_stream* from1, merge_stream* from2, merge_stream* result);

void write_C_rid_rlen(id_stream* rlen, id_t* C_rcsr, id_t crum);

void write_C_val_cid(merge_stream* final_result, val_t* C_val, id_t* C_cid, id_t* counter_t, id_stream* result_len);

#endif
