#include "rcache.h"
#include "util.h"

void rrequest_splitter(rcache_stream* from, rcache_stream* rrequest, rcache_stream* rcollector, rcache_stream* scheduler1){

	rcache_request temp;
	rcache_request temp_r;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!from->empty()) {
			temp = from->read();
			if (!temp.end) {
				temp_r.end = false;
				temp_r.Acid = temp.Acid;
				temp_r.dest = temp.dest;
				temp_r.data = temp.data;
				temp_r.Acid1 = temp.Acid1;
				temp_r.type = temp.type;

				rrequest->write(temp_r);
				rcollector->write(temp_r);
				scheduler1->write(temp_r);
			}
			enable = temp.end;
		}
	}

	temp_r.end = true;
	rrequest->write(temp_r);
	rcollector->write(temp_r);
	scheduler1->write(temp_r);
}

void rrequest_manager(rcache_stream* from, rcache_stream* request){

	rcache_request ori_request;
	rcache_request curr_request;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!from->empty()) {
			ori_request = from->read();
			if (!ori_request.end) {
				curr_request.Acid = ori_request.Acid;
				curr_request.Acid1 = ori_request.Acid1;
				curr_request.end = false;
				curr_request.dest = ori_request.dest;
				curr_request.data = ori_request.data;
				curr_request.type = CACHE_FIRST_READ;

				request->write(curr_request);
			}
			enable = ori_request.end;
		}
	}

	curr_request.end = true;
	request->write(curr_request);
}

void rschedule_access(rcache_stream* schedule, rresponse_stream* response, rcacheline_stream* off, ap_uint<128>* off_pointer,
		rcache_stream* request){

	rcache_response curr_response;
	rcache_request curr_request;

	rcache_request ori_request;

	ap_uint<128> Bridx[2];
	ap_uint<160> target_data;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!schedule->empty()) {
			ori_request = schedule->read();
			if (!ori_request.end) {

				for (id_t _t=0; _t<1; ){
			#pragma HLS PIPELINE off
					if (!response->empty()) {
						curr_response = response->read();

						if (curr_response.result == CACHE_FIRST_MISS) {
							id_t access_idx = ori_request.Acid / 4;
							// issue streaming off-chip memory access
							for (id_t i=access_idx; i<access_idx+2; i++){
		#pragma HLS PIPELINE II=1
								Bridx[i-access_idx] = off_pointer[i];
							}

							target_data(127, 0) = Bridx[0];
							target_data(159, 128) = Bridx[1](31, 0);

							curr_request.Acid = ori_request.Acid;
							curr_request.Acid1 = ori_request.Acid1;
							curr_request.end = false;
							curr_request.data = target_data;
							curr_request.dest = ori_request.dest;
							curr_request.type = CACHE_WRITE;
							request->write(curr_request);
							off->write(target_data);
						} else {
							off->write(curr_response.data);
						}
						_t ++;
					}
				}
			}
			enable = ori_request.end;
		}
#ifndef __SYNTHESIS__
		enable = true;
#endif
	}

	curr_request.end = true;
	request->write(curr_request);
}

void rrequest_merger(rcache_stream* from1, rcache_stream* from2, rcache_stream* to){

	bool from1_end = false;
	bool from2_end = false;

	rcache_request temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if (!from1->empty()) {
				temp = from1->read();
				from1_end = temp.end;
				if (!from1_end)
					to->write(temp);
			} else if (!from2->empty()) {
				temp = from2->read();
				from2_end = temp.end;
				if (!from2_end)
					to->write(temp);
			}
		}
		enable = from1_end && from2_end;
	}

	temp.end = true;
	to->write(temp);
}

void rcollector(rcache_stream* collect_from, rcacheline_stream* val_from, rcache_stream* to){

	rcache_request temp_control;
	ap_uint<160> target_data;
	rcache_response curr_response;
	rcache_request result_request;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE off
		if (!collect_from->empty()) {
			temp_control = collect_from->read();
			if (!temp_control.end) {

				for (id_t _t=0; _t<1; ){
			#pragma HLS PIPELINE off
					if (!val_from->empty()) {
						target_data = val_from->read();

						id_t curr_offset = temp_control.Acid % 4;
						id_t Acid = target_data(curr_offset*32+31, curr_offset*32);
						id_t Acid1 = target_data((curr_offset+1)*32+31, (curr_offset+1)*32);

						result_request.Acid = Acid;
						result_request.Acid1 = Acid1;
						result_request.end = false;
						result_request.data = 0;
						result_request.dest = temp_control.dest;
						result_request.type = temp_control.type;

						to->write(result_request);
						_t ++;
					}
				}
			}
			enable = temp_control.end;
		}
	}

	result_request.dest = 7;
	to->write(result_request);
}


void rcrequest_manager(rcache_stream* from, rcache_stream* rrequest_result, rresponse_stream* response, rcache_stream* to1, ap_uint<128>* B_rcsr){

#pragma HLS DATAFLOW disable_start_propagation

	rcache_stream rcollec, rrequest, rscheduler;
#pragma HLS STREAM variable=rcollec depth=16
#pragma HLS STREAM variable=rscheduler depth=16
	rcache_stream r_first, r_req;
//#pragma HLS STREAM variable=r_req depth=100
	rcacheline_stream roff;
#pragma HLS STREAM variable=roff depth=8

	rrequest_splitter(from, &rrequest, &rcollec, &rscheduler);
	// send the request in parallel
	rrequest_manager(&rrequest, &r_first);
	rschedule_access(&rscheduler, response, &roff, B_rcsr, &r_req);
	rrequest_merger(&r_first, &r_req, rrequest_result);

	// receive the data, each element foreknows the number to be received
	rcollector(&rcollec, &roff, to1);
}

void request_to_rbanks(rcache_stream* request, id_t base_addr_rcsr,
        rcache_stream* bank1, rcache_stream* bank2, rcache_stream* bank3, rcache_stream* bank4
//		rcache_stream* bank5, rcache_stream* bank6, rcache_stream* bank7, rcache_stream* bank8
		){

	rcache_request temp_req;
	temp_req.end = false;

	id_t B_rlen_curr_addr;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!request->empty()) {
			temp_req = request->read();

			if (!temp_req.end) {
				B_rlen_curr_addr = base_addr_rcsr + temp_req.Acid * 4;

				// addr / cacheline_size --> cacheline number
				// cacheline number % total set number --> set number
				// set_number % total bank number --> bank_number
				id_t curr_bank = ((B_rlen_curr_addr / RCACHE_CACHELINE_BYTES) % RCACHE_SET_NUM) % (RCACHE_SET_NUM / RCACHE_BANK_NUM);

				if (curr_bank == 0) {
					bank1->write(temp_req);
				} else if (curr_bank == 1) {
					bank2->write(temp_req);
				} else if (curr_bank == 2) {
					bank3->write(temp_req);
				} else if (curr_bank == 3) {
					bank4->write(temp_req);
				}
			}
			enable = temp_req.end;
		}
	}

	temp_req.end = true;

	bank1->write(temp_req);
	bank2->write(temp_req);
	bank3->write(temp_req);
	bank4->write(temp_req);
}

void rcache(rcache_stream* request, id_t base_addr_rcsr, rresponse_stream* rres, id_t* access_number, id_t* hit_number,
		mi_stream* ms){

	ap_uint<160> _data[RCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_data type=ram_1p impl=bram

	ap_uint<32-RCACHE_TAG_OFFSET> _tag[RCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_tag type=ram_1p impl=lutram
#pragma HLS ARRAY_PARTITION variable=_tag complete dim=2

	id_t _lru_count_1[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_1 type=ram_t2p impl=bram
	id_t _lru_count_2[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_2 type=ram_t2p impl=bram
	id_t _lru_count_3[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_3 type=ram_t2p impl=bram
	id_t _lru_count_4[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_4 type=ram_t2p impl=bram
	id_t _lru_count_5[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_5 type=ram_t2p impl=bram
	id_t _lru_count_6[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_6 type=ram_t2p impl=bram
	id_t _lru_count_7[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_7 type=ram_t2p impl=bram
	id_t _lru_count_8[VCCACHE_BANK_NUM][2];
#pragma HLS BIND_STORAGE variable=_lru_count_8 type=ram_t2p impl=bram

	// lru counter, higher counter refers to newly accessed
	id_t count = 1;

	// access_number and hit number
	id_t _access_number = 0;
	id_t _hit_number = 0;

	id_t curr_idx, curr_offset;
	ap_uint<32> B_rlen_curr_addr;

	ap_uint<32-RCACHE_TAG_OFFSET> curr_tag;
	ap_uint<160> target_data;

	lru_struct temp_lru[16];
#pragma HLS ARRAY_PARTITION variable=temp_lru complete dim=1

	// ap_uint data is not initialized with 0
	for (id_t i=0; i<RCACHE_BANK_NUM; i++) {
#pragma HLS PIPELINE II=1
		for (id_t j=0; j<16; j++) {
#pragma HLS UNROLL
			_tag[i][j] = 0;
		}
	}

	rcache_request temp_request;
	rcache_response temp_response;
	manager_idle temp_mi;

	temp_response.end = false;
	temp_mi.end = false;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!request->empty()) {
			temp_request = request->read();

			if (!temp_request.end) {

				B_rlen_curr_addr = base_addr_rcsr + temp_request.Acid * 4;

				curr_idx = ((B_rlen_curr_addr / RCACHE_CACHELINE_BYTES) % RCACHE_SET_NUM) / (RCACHE_SET_NUM / RCACHE_BANK_NUM);
				curr_tag = B_rlen_curr_addr >> RCACHE_TAG_OFFSET;

				// check whether cache hits
				id_t target_way = 0xFFFFFFFF;
				for (id_t j=0; j<16; j++) {
			#pragma HLS UNROLL
					if (_tag[curr_idx][j] == curr_tag) {
						target_way = j;
					}
				}

				if (target_way != 0xFFFFFFFF) {
					// if cache hits
					// for read, hits, the access time increases
					_access_number += 1;
					_hit_number += 1;

					if (temp_request.type == CACHE_FIRST_READ) {
						// response to the manager
						temp_response.result = CACHE_FIRST_HIT;
						temp_response.dest = temp_request.dest;
						temp_response.data = _data[curr_idx][target_way];
						temp_response.no_use = false;
					} else {
						// for concurrent writes
						temp_response.no_use = true;
						temp_response.result = CACHE_IDLE;
					}

					temp_mi.dest = temp_request.dest;
					temp_mi.idle = true;

					// update the lru counter
//					_lru_count[curr_idx][target_way] = count;
					if (target_way / 2 == 0) {
						_lru_count_1[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 1) {
						_lru_count_2[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 2) {
						_lru_count_3[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 3) {
						_lru_count_4[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 4) {
						_lru_count_5[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 5) {
						_lru_count_6[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 6) {
						_lru_count_7[curr_idx][target_way%2] = count;
					} else if (target_way / 2 == 7) {
						_lru_count_8[curr_idx][target_way%2] = count;
					}
				}
				else {
					if (temp_request.type == CACHE_FIRST_READ) {
						// response to the manager
						temp_response.dest = temp_request.dest;
						temp_response.result = CACHE_FIRST_MISS;
						temp_response.no_use = false;

						temp_mi.idle = false;
						temp_mi.dest = temp_request.dest;
					} else {
						// if miss, find the victim using lru
						for (id_t tl=0; tl<16; tl++) {
					#pragma HLS UNROLL
							if (tl / 2 == 0) {
								temp_lru[tl].counter = _lru_count_1[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 1) {
								temp_lru[tl].counter = _lru_count_2[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 2) {
								temp_lru[tl].counter = _lru_count_3[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 3) {
								temp_lru[tl].counter = _lru_count_4[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 4) {
								temp_lru[tl].counter = _lru_count_5[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 5) {
								temp_lru[tl].counter = _lru_count_6[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 6) {
								temp_lru[tl].counter = _lru_count_7[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							} else if (tl / 2 == 7) {
								temp_lru[tl].counter = _lru_count_8[curr_idx][tl%2];
								temp_lru[tl].way_n = tl;
							}
						}

						find_eviction(temp_lru[0], temp_lru[1], temp_lru[2], temp_lru[3],
								temp_lru[4], temp_lru[5], temp_lru[6], temp_lru[7],
								temp_lru[8], temp_lru[9], temp_lru[10], temp_lru[11],
								temp_lru[12], temp_lru[13], temp_lru[14], temp_lru[15], &target_way);

						_access_number += 1;
						// write, assume all the writes are missed ones
						_data[curr_idx][target_way] = temp_request.data;
						_tag[curr_idx][target_way] = curr_tag;

						// update the lru counter
						if (target_way / 2 == 0) {
							_lru_count_1[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 1) {
							_lru_count_2[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 2) {
							_lru_count_3[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 3) {
							_lru_count_4[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 4) {
							_lru_count_5[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 5) {
							_lru_count_6[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 6) {
							_lru_count_7[curr_idx][target_way%2] = count;
						} else if (target_way / 2 == 7) {
							_lru_count_8[curr_idx][target_way%2] = count;
						}

						temp_response.no_use = true;
						temp_response.result = CACHE_IDLE;

						temp_mi.idle = true;
						temp_mi.dest = temp_request.dest;
					}
				}

				rres->write(temp_response);
				ms->write(temp_mi);

				count += 1;
			}
			enable = temp_request.end;
		}
	}

	*access_number = _access_number;
	*hit_number = _hit_number;

	temp_response.end = true;
	rres->write(temp_response);

	temp_mi.end = true;
	ms->write(temp_mi);
}
