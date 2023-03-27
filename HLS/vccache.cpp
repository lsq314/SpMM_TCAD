#include "vccache.h"
#include "util.h"

void vcrequest_splitter(B_request_stream* from, internal_control_stream* vrequest, internal_control_stream* crequest,
		internal_control_stream* vcollector, internal_control_stream* ccollector,
		internal_control_stream* scheduler1, internal_control_stream* scheduler2){

	B_row_request temp;

	internal_control tcontrol;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!from->empty()) {
			temp = from->read();
			if (!temp.end) {
				tcontrol.bridx = temp.bridx;
				tcontrol.brlen = temp.brlen;
				tcontrol.end = false;
				tcontrol.peID = temp.peID;

				vrequest->write(tcontrol);
				crequest->write(tcontrol);
				vcollector->write(tcontrol);
				ccollector->write(tcontrol);
				scheduler1->write(tcontrol);
				scheduler2->write(tcontrol);
			}
			enable = temp.end;
		}
	}

	tcontrol.end = true;

	vrequest->write(tcontrol);
	crequest->write(tcontrol);
	vcollector->write(tcontrol);
	ccollector->write(tcontrol);
	scheduler1->write(tcontrol);
	scheduler2->write(tcontrol);
}

void request_manager(internal_control_stream* from, vcrequest_stream* request, uint32_t deviceID, id_t type){

	internal_control ori_request;
	vccache_request curr_request;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!from->empty()) {
			ori_request = from->read();
			if (!ori_request.end) {

				id_t end_idx = (ori_request.bridx + ori_request.brlen - 1) / 4;

				curr_request.brlen = (end_idx - ori_request.bridx / 4) >= 8 ? 8 : (end_idx - ori_request.bridx / 4 + 1);
				curr_request.brid = ori_request.bridx / 4;
				curr_request.peID = ori_request.peID;
				curr_request.end = false;
				curr_request.dest = deviceID;
				curr_request.type = CACHE_FIRST_READ;
				curr_request.target = type;

				request->write(curr_request);
			}
			enable = ori_request.end;
		}
	}

	curr_request.end = true;
	request->write(curr_request);
}

void schedule_access(internal_control_stream* schedule, vcresponse_stream* response, B_data_stream* off, ap_uint<128>* off_pointer,
		vcrequest_stream* request, id_t type){

	vccache_response curr_response;
	vccache_request curr_request;

	internal_control ori_request;

	id_t begin_idx, begin_block_idx;
	id_t end_idx, end_block_idx;

	ap_uint<128> target_data;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!schedule->empty()) {
			ori_request = schedule->read();
			if (!ori_request.end) {
				for (id_t _t=0; _t<1; ) {
#pragma HLS PIPELINE off
					if (!response->empty()) {
						curr_response = response->read();
						begin_idx = ori_request.bridx;
						end_idx = ori_request.bridx + ori_request.brlen - 1;

						begin_block_idx = begin_idx / 4;
						end_block_idx = end_idx / 4;

						if (curr_response.result == CACHE_FIRST_HIT) {
							if (end_block_idx >= (begin_block_idx + 8)) {
								for (id_t _t=begin_block_idx+8; _t<=end_block_idx; _t++) {
									target_data = off_pointer[_t];
									off->write(target_data);
								}
							}
						} else {
							// issue streaming off-chip memory access
							for (id_t _t=begin_block_idx; _t<=end_block_idx; _t++) {
						#pragma HLS PIPELINE II=1
								target_data = off_pointer[_t];
								id_t min_idx = (end_block_idx >= (begin_block_idx + 8)) ? begin_block_idx + 7 : end_block_idx;

								if (_t <= min_idx) {
									curr_request.brid = begin_block_idx;
									curr_request.target = type;
									curr_request.peID = ori_request.peID;
									curr_request.end = false;
									curr_request.type = CACHE_WRITE;
									curr_request.data = target_data;
									curr_request.write_idx = _t-begin_block_idx;

									request->write(curr_request);
								}

								off->write(target_data);
							}
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

void request_merger(vcrequest_stream* from1, vcrequest_stream* from2, vcrequest_stream* to){

	bool from1_end = false;
	bool from2_end = false;

	vccache_request temp;

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

void collector(internal_control_stream* collect_from, cacheline_stream* result_from, B_data_stream* val_from, block_stream* to){

	internal_control temp_control;
	id_t begin_block_idx, end_block_idx;
	ap_uint<128> target_data;
	ap_uint<1024> cache_data;
	block result_block;
	ap_uint<3> begin, end;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE off
		if (!collect_from->empty()) {
			temp_control = collect_from->read();
			if (!temp_control.end) {
				begin_block_idx = temp_control.bridx / 4;
				end_block_idx = (temp_control.bridx + temp_control.brlen - 1) / 4;

				for (id_t _t=0; _t<1; ){
			#pragma HLS PIPELINE off
					if (!result_from->empty() || !val_from->empty()) {
						if (!result_from->empty()) {
							cache_data = result_from->read();
							id_t min_idx = (end_block_idx >= (begin_block_idx + 8)) ? begin_block_idx + 7 : end_block_idx;
							for (id_t _tt=begin_block_idx; _tt<=min_idx; _tt++) {
//								target_data = result_from->read();
								begin = (_tt == begin_block_idx) ? (temp_control.bridx % 4) : 0;
								end = (_tt == end_block_idx) ? (temp_control.bridx + temp_control.brlen - 1) % 4 : 3;

								result_block.begin = begin;
								result_block.end = end;
								result_block.data = cache_data((_tt-begin_block_idx)*128+127, (_tt-begin_block_idx)*128);

								to->write(result_block);
							}

							if (end_block_idx >= (begin_block_idx + 8)) {
								for (id_t _tt=begin_block_idx+8; _tt<=end_block_idx; _tt++) {
									target_data = val_from->read();

									begin = (_tt == begin_block_idx) ? (temp_control.bridx % 4) : 0;
									end = (_tt == end_block_idx) ? (temp_control.bridx + temp_control.brlen - 1) % 4 : 3;

									result_block.begin = begin;
									result_block.end = end;
									result_block.data = target_data;

									to->write(result_block);
								}
							}
						} else if (!val_from->empty()) {
							for (id_t _tt=begin_block_idx; _tt<=end_block_idx; _tt++) {
								target_data = val_from->read();

								begin = (_tt == begin_block_idx) ? (temp_control.bridx % 4) : 0;
								end = (_tt == end_block_idx) ? (temp_control.bridx + temp_control.brlen - 1) % 4 : 3;

								result_block.begin = begin;
								result_block.end = end;
								result_block.data = target_data;

								to->write(result_block);
							}
						}
						_t ++;
					}
				}
			}
			enable = temp_control.end;
		}
	}
}


void vcrequest_manager(B_request_stream* from, vcrequest_stream* vrequest_result, vcrequest_stream* crequest_result,
		vcresponse_stream* vresponse, vcresponse_stream* cresponse, cacheline_stream* vresult, cacheline_stream* cresult,
		block_stream* to1, block_stream* to2, uint32_t deviceID, ap_uint<128>* B_val, ap_uint<128>* B_cid){

#pragma HLS DATAFLOW disable_start_propagation

	internal_control_stream vcollector, ccollector, vrequest, crequest, vscheduler, cscheduler;
	vcrequest_stream v_first, c_first, v_req, c_req;
	B_data_stream voff, coff;
#pragma HLS STREAM variable=voff depth=8
#pragma HLS STREAM variable=coff depth=8

	// split the B access to B_val_access and B_cid_access
	vcrequest_splitter(from, &vrequest, &crequest, &vcollector, &ccollector, &vscheduler, &cscheduler);
	// send the request in parallel
	request_manager(&vrequest, &v_first, deviceID, B_VAL);
	schedule_access(&vscheduler, vresponse, &voff, B_val, &v_req, B_VAL);

	request_manager(&crequest, &c_first, deviceID, B_CID);
	schedule_access(&cscheduler, cresponse, &coff, B_cid, &c_req, B_CID);

	request_merger(&v_first, &v_req, vrequest_result);
	request_merger(&c_first, &c_req, crequest_result);

	// receive the data, each element foreknows the number to be received
	collector(&vcollector, vresult, &voff, to1);
	collector(&ccollector, cresult, &coff, to2);
}

void request_to_vcbanks(vcrequest_stream* request, id_t base_addr_val, id_t base_addr_cid,
        vcrequest_stream* bank1, vcrequest_stream* bank2){

	// vccache requests to different banks
	// vccache is **KB and each bank is **KB
	vccache_request temp_vcreq;
	temp_vcreq.end = false;

	id_t B_rlen_curr_addr;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!request->empty()) {
			temp_vcreq = request->read();

			if (!temp_vcreq.end) {
				// each block is 16 bytes, i.e., 16 addresses
				if (temp_vcreq.target == B_VAL)
					B_rlen_curr_addr = base_addr_val + temp_vcreq.brid * 16;
				else
					B_rlen_curr_addr = base_addr_cid + temp_vcreq.brid * 16;

				// addr / cacheline_size --> cacheline number
				// cacheline number % total set number --> set number
				// set_number % total bank number --> bank_number
				id_t curr_bank = ((B_rlen_curr_addr / VCCACHE_CACHELINE_BYTES) % VCCACHE_SET_NUM) % (VCCACHE_SET_NUM / VCCACHE_BANK_NUM);

				if (curr_bank == 0) {
					bank1->write(temp_vcreq);
				} else if (curr_bank == 1) {
					bank2->write(temp_vcreq);
				}
			}
			enable = temp_vcreq.end;
		}
	}

	temp_vcreq.end = true;

	bank1->write(temp_vcreq);
	bank2->write(temp_vcreq);
}

void vcache(vcrequest_stream* request, id_t base_addr_val, vcresponse_stream* vres, vcresult_stream* vcresult,
		id_t* access_number, id_t* hit_number){

	// each bank is a **KB 16-way cache with 16-byte cacheline
	ap_uint<1024> _data[VCCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_data type=ram_1p impl=uram

	ap_uint<32-VCCACHE_TAG_OFFSET> _tag[VCCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_tag type=ram_1p impl=lutram
#pragma HLS ARRAY_PARTITION variable=_tag complete dim=2

//	id_t _lru_count[VCCACHE_BANK_NUM][16];
//#pragma HLS BIND_STORAGE variable=_lru_count type=ram_1p impl=lutram
//#pragma HLS ARRAY_PARTITION variable=_lru_count complete dim=2
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
	id_t count = 0;

	// access_number and hit number
	id_t _access_number = 0;
	id_t _hit_number = 0;

	id_t B_rlen_curr_addr, curr_idx, curr_offset;

	ap_uint<32-VCCACHE_TAG_OFFSET> curr_tag;
	ap_uint<128> target_data;
	ap_uint<1024> cache_data;

	lru_struct temp_lru[16];
#pragma HLS ARRAY_PARTITION variable=temp_lru complete dim=1

	// ap_uint data is not initialized with 0
	for (id_t i=0; i<VCCACHE_BANK_NUM; i++) {
#pragma HLS PIPELINE II=1
		for (id_t j=0; j<16; j++) {
#pragma HLS UNROLL
			_tag[i][j] = 0;
		}
	}

	vccache_request temp_request;
	vccache_result temp_result;
	vccache_response temp_response;

	temp_response.end = false;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!request->empty()) {
			temp_request = request->read();

			if (!temp_request.end) {

				// calculate tag, offset (each cacheline contains 4 elements), index in the bank
				B_rlen_curr_addr = base_addr_val + temp_request.brid * 16;

				curr_idx = ((B_rlen_curr_addr / VCCACHE_CACHELINE_BYTES) % VCCACHE_SET_NUM) / (VCCACHE_SET_NUM / VCCACHE_BANK_NUM);
				curr_tag = B_rlen_curr_addr >> VCCACHE_TAG_OFFSET;

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
					// for write, if hits, the off-chip access is performed and thus the access time increases
					// for read, hits, the access time increases

					if (temp_request.type != CACHE_WRITE) {
						_access_number += 1;
						_hit_number += 1;

						if (temp_request.type == CACHE_FIRST_READ) {
							// response to the manager
							temp_response.result = CACHE_FIRST_HIT;
							temp_response.peID = temp_request.peID;
							vres->write(temp_response);
						}

						// write the result to collector
//						for (id_t i=0; i<temp_request.brlen; i++) {
//							temp_result.type = temp_request.target;
//							temp_result.data = _data[curr_idx][target_way*8+i];
//							temp_result.end = false;
//							temp_result.peID = temp_request.peID;
//
//							vcresult->write(temp_result);
//						}
						temp_result.type = temp_request.target;
						temp_result.data = _data[curr_idx][target_way];
						temp_result.end = false;
						temp_result.peID = temp_request.peID;

						vcresult->write(temp_result);

					} else {
						// only updates the content
//						_data[curr_idx][target_way*8+temp_request.write_idx] = temp_request.data;
						_data[curr_idx][target_way](temp_request.write_idx*128+127, temp_request.write_idx*128) = temp_request.data;
					}

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
						_access_number += 1;
						// response to the manager
						temp_response.peID = temp_request.peID;
						temp_response.result = CACHE_FIRST_MISS;
						vres->write(temp_response);
					} else {
						// if miss, find the victim using lru

						for (ap_uint<5> tl=0; tl<16; tl++) {
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

						if (temp_request.type == CACHE_READ) {
							_access_number += 1;
							// never happens

							// off-chip memory access
//							if (temp_request.target == B_VAL) {
//								target_data = source_ddr_val[temp_request.brid];
//							} else {
//								target_data = source_ddr_cid[temp_request.brid];
//							}

							_data[curr_idx][target_way] = target_data;
							_tag[curr_idx][target_way] = curr_tag;

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

							// write the result to collector
							temp_result.type = temp_request.target;
							temp_result.data = target_data;
							temp_result.end = false;
							temp_result.peID = temp_request.peID;

							vcresult->write(temp_result);

						} else {
							// write, assume all the writes are missed ones
							_data[curr_idx][target_way](127, 0) = temp_request.data;
							_tag[curr_idx][target_way] = curr_tag;

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
					}
				}
				count += 1;
			}
			enable = temp_request.end;
		}
	}

	*access_number = _access_number;
	*hit_number = _hit_number;

	temp_result.end = true;
	vcresult->write(temp_result);

	temp_response.end = true;
	vres->write(temp_response);
}

void ccache(vcrequest_stream* request, id_t base_addr_cid, vcresponse_stream* cres, vcresult_stream* vcresult,
		id_t* access_number, id_t* hit_number){

	// each bank is a **KB 16-way cache with 16-byte cacheline
	ap_uint<1024> _data[VCCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_data type=ram_1p impl=uram

	ap_uint<32-VCCACHE_TAG_OFFSET> _tag[VCCACHE_BANK_NUM][16];
#pragma HLS BIND_STORAGE variable=_tag type=ram_1p impl=lutram
#pragma HLS ARRAY_PARTITION variable=_tag complete dim=2

//	id_t _lru_count[VCCACHE_BANK_NUM][16];
//#pragma HLS BIND_STORAGE variable=_lru_count type=ram_1p impl=lutram
//#pragma HLS ARRAY_PARTITION variable=_lru_count complete dim=2
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
	id_t count = 0;

	// access_number and hit number
	id_t _access_number = 0;
	id_t _hit_number = 0;

	id_t B_rlen_curr_addr, curr_idx, curr_offset;

	ap_uint<32-VCCACHE_TAG_OFFSET> curr_tag;
	ap_uint<128> target_data;
	ap_uint<1024> cache_data;

	lru_struct temp_lru[16];
#pragma HLS ARRAY_PARTITION variable=temp_lru complete dim=1

	// ap_uint data is not initialized with 0
	for (id_t i=0; i<VCCACHE_BANK_NUM; i++) {
#pragma HLS PIPELINE II=1
		for (id_t j=0; j<16; j++) {
#pragma HLS UNROLL
			_tag[i][j] = 0;
		}
	}

	vccache_request temp_request;
	vccache_result temp_result;
	vccache_response temp_response;

	temp_response.end = false;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!request->empty()) {
			temp_request = request->read();

			if (!temp_request.end) {

				// calculate tag, offset (each cacheline contains 4 elements), index in the bank
				B_rlen_curr_addr = base_addr_cid + temp_request.brid * 16;

				curr_idx = ((B_rlen_curr_addr / VCCACHE_CACHELINE_BYTES) % VCCACHE_SET_NUM) / (VCCACHE_SET_NUM / VCCACHE_BANK_NUM);
				curr_tag = B_rlen_curr_addr >> VCCACHE_TAG_OFFSET;

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
					// for write, if hits, the off-chip access is performed and thus the access time increases
					// for read, hits, the access time increases

					if (temp_request.type != CACHE_WRITE) {
						_access_number += 1;
						_hit_number += 1;

						if (temp_request.type == CACHE_FIRST_READ) {
							// response to the manager
							temp_response.result = CACHE_FIRST_HIT;
							temp_response.peID = temp_request.peID;
							cres->write(temp_response);
						}

						// write the result to collector
//						for (id_t i=0; i<temp_request.brlen; i++) {
//							temp_result.type = temp_request.target;
//							temp_result.data = _data[curr_idx][target_way*8+i];
//							temp_result.end = false;
//							temp_result.peID = temp_request.peID;
//
//							vcresult->write(temp_result);
//						}
						temp_result.type = temp_request.target;
						temp_result.data = _data[curr_idx][target_way];
						temp_result.end = false;
						temp_result.peID = temp_request.peID;

						vcresult->write(temp_result);

					} else {
						// only updates the content
//						_data[curr_idx][target_way*8+temp_request.write_idx] = temp_request.data;
						_data[curr_idx][target_way](temp_request.write_idx*128+127, temp_request.write_idx*128) = temp_request.data;
					}

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
						_access_number += 1;
						// response to the manager
						temp_response.peID = temp_request.peID;
						temp_response.result = CACHE_FIRST_MISS;
						cres->write(temp_response);
					} else {
						// if miss, find the victim using lru

						for (ap_uint<5> tl=0; tl<16; tl++) {
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

						if (temp_request.type == CACHE_READ) {
							_access_number += 1;
							// never happens

							// off-chip memory access
//							if (temp_request.target == B_VAL) {
//								target_data = source_ddr_val[temp_request.brid];
//							} else {
//								target_data = source_ddr_cid[temp_request.brid];
//							}

							_data[curr_idx][target_way] = target_data;
							_tag[curr_idx][target_way] = curr_tag;

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

							// write the result to collector
							temp_result.type = temp_request.target;
							temp_result.data = target_data;
							temp_result.end = false;
							temp_result.peID = temp_request.peID;

							vcresult->write(temp_result);

						} else {
							// write, assume all the writes are missed ones
							_data[curr_idx][target_way](127, 0) = temp_request.data;
							_tag[curr_idx][target_way] = curr_tag;

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
					}
				}
				count += 1;
			}
			enable = temp_request.end;
		}
	}

	*access_number = _access_number;
	*hit_number = _hit_number;

	temp_result.end = true;
	vcresult->write(temp_result);

	temp_response.end = true;
	cres->write(temp_response);
}


