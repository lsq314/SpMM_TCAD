#include "core.h"
#include "util.h"

void merge_4_to_1_rrequest(rcache_stream* cache_result1, rcache_stream* cache_result2, rcache_stream* cache_result3,
		rcache_stream* cache_result4, rcache_stream* cache_result){

	bool whether_read1 = false;
	bool whether_read2 = false;
	bool whether_read3 = false;
	bool whether_read4 = false;

	bool have_read1 = false;
	bool have_read2 = false;
	bool have_read3 = false;
	bool have_read4 = false;

	bool end1 = false;
	bool end2 = false;
	bool end3 = false;
	bool end4 = false;

	rcache_request temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || cache_result1->empty()) && (have_read2 || cache_result2->empty()) &&
					(have_read3 || cache_result3->empty()) && (have_read4 || cache_result4->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
				have_read3 = false;
				have_read4 = false;
			}

			whether_read1 = !have_read1 && !cache_result1->empty();
			whether_read2 = !have_read2 && !cache_result2->empty();
			whether_read3 = !have_read3 && !cache_result3->empty();
			whether_read4 = !have_read4 && !cache_result4->empty();

			if (whether_read1) {
				temp = cache_result1->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = cache_result2->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			} else if (whether_read3) {
				temp = cache_result3->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read3 = false;
				have_read3 = true;
				end3 = temp.end;
			} else if (whether_read4) {
				temp = cache_result4->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read4 = false;
				have_read4 = true;
				end4 = temp.end;
			}

			enable = end1 && end2 && end3 && end4;
		}
	}

	temp.end = true;
	cache_result->write(temp);
}

void distributer_rresponse(rresponse_stream* from1, rresponse_stream* to1, rresponse_stream* to2, rresponse_stream* to3, rresponse_stream* to4){

	rcache_response temp;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!from1->empty()) {
			temp = from1->read();
			if (!temp.end){
				if (temp.dest == 1) {
					to1->write(temp);
				} else if (temp.dest == 2) {
					to2->write(temp);
				} else if (temp.dest == 3) {
					to3->write(temp);
				} else if (temp.dest == 4) {
					to4->write(temp);
				}
			}
			enable = temp.end;
		}
	}

	temp.end = true;
	to1->write(temp);
	to2->write(temp);
	to3->write(temp);
	to4->write(temp);
}

void find_eviction(lru_struct in1, lru_struct in2, lru_struct in3, lru_struct in4,
		lru_struct in5, lru_struct in6, lru_struct in7, lru_struct in8,
		lru_struct in9, lru_struct in10, lru_struct in11, lru_struct in12,
		lru_struct in13, lru_struct in14, lru_struct in15, lru_struct in16, id_t* minimum){

#pragma HLS INLINE

	lru_struct 	temp_11, temp_12, temp_13, temp_14, temp_15, temp_16, temp_17, temp_18,
				temp_21, temp_22, temp_23, temp_24,
				temp_41, temp_42,
				temp_81;

	temp_11 = (in1.counter < in2.counter) ? in1 : in2;
	temp_12 = (in3.counter < in4.counter) ? in3 : in4;
	temp_13 = (in5.counter < in6.counter) ? in5 : in6;
	temp_14 = (in7.counter < in8.counter) ? in7 : in8;
	temp_15 = (in9.counter < in10.counter) ? in9 : in10;
	temp_16 = (in11.counter < in12.counter) ? in11 : in12;
	temp_17 = (in13.counter < in14.counter) ? in13 : in14;
	temp_18 = (in15.counter < in16.counter) ? in15 : in16;

	temp_21 = (temp_11.counter < temp_12.counter) ? temp_11 : temp_12;
	temp_22 = (temp_13.counter < temp_14.counter) ? temp_13 : temp_14;
	temp_23 = (temp_15.counter < temp_16.counter) ? temp_15 : temp_16;
	temp_24 = (temp_17.counter < temp_18.counter) ? temp_17 : temp_18;

	temp_41 = (temp_21.counter < temp_22.counter) ? temp_21 : temp_22;
	temp_42 = (temp_23.counter < temp_24.counter) ? temp_23 : temp_24;

	temp_81 = (temp_41.counter < temp_42.counter) ? temp_41 : temp_42;

	*minimum = temp_81.way_n;
}

void from_rcache_result_to_B_vcrequest(rcache_stream* from1, B_request_stream* to1, B_request_stream* to2, id_stream* B_len){

	rcache_request temp, temp1;
	id_t first_idx, second_idx;
	B_row_request temp_result;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!from1->empty() && !to2->full()) {
			temp = from1->read();

			if (temp.dest != 7) {
//				temp1 = from1->read();

//				first_idx = (temp.Acid < temp1.Acid) ? temp.Acid : temp1.Acid;
//				second_idx = (temp.Acid < temp1.Acid) ? temp1.Acid : temp.Acid;
				first_idx = temp.Acid;
				second_idx = temp.Acid1;

				temp_result.peID = temp.dest;
				temp_result.bridx = first_idx;
				temp_result.brlen = second_idx - first_idx;
				temp_result.end = false;

				if ((second_idx - first_idx) > 16)
					to1->write(temp_result);
				else
					to2->write(temp_result);

				B_len->write(second_idx - first_idx);
			}

			enable = temp.dest == 7;
		}
	}

	temp_result.end = true;
	to1->write(temp_result);
	to2->write(temp_result);
}

void merge_4_to_1_vcrequest(vcrequest_stream* cache_result1, vcrequest_stream* cache_result2, vcrequest_stream* cache_result3,
		vcrequest_stream* cache_result4, vcrequest_stream* cache_result){

	// merge requests from the 8 banks to one single stream in round-robin fashion
	bool whether_read1 = false;
	bool whether_read2 = false;
	bool whether_read3 = false;
	bool whether_read4 = false;

	bool have_read1 = false;
	bool have_read2 = false;
	bool have_read3 = false;
	bool have_read4 = false;

	bool end1 = false;
	bool end2 = false;
	bool end3 = false;
	bool end4 = false;

	vccache_request temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || cache_result1->empty()) && (have_read2 || cache_result2->empty()) &&
					(have_read3 || cache_result3->empty()) && (have_read4 || cache_result4->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
				have_read3 = false;
				have_read4 = false;
			}

			whether_read1 = !have_read1 && !cache_result1->empty();
			whether_read2 = !have_read2 && !cache_result2->empty();
			whether_read3 = !have_read3 && !cache_result3->empty();
			whether_read4 = !have_read4 && !cache_result4->empty();

			if (whether_read1) {
				temp = cache_result1->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = cache_result2->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			} else if (whether_read3) {
				temp = cache_result3->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read3 = false;
				have_read3 = true;
				end3 = temp.end;
			} else if (whether_read4) {
				temp = cache_result4->read();

				if (!temp.end)
					cache_result->write(temp);

				whether_read4 = false;
				have_read4 = true;
				end4 = temp.end;
			}

			enable = end1 && end2 && end3 && end4;
		}
	}

	temp.end = true;
	cache_result->write(temp);
}

void distributer_vcresponse(vcresponse_stream* from1, vcresponse_stream* to1, vcresponse_stream* to2, vcresponse_stream* to3, vcresponse_stream* to4){

	vccache_response temp;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!from1->empty()) {
			temp = from1->read();
			if (!temp.end){
				if (temp.peID == 1) {
					to1->write(temp);
				} else if (temp.peID == 2) {
					to2->write(temp);
				} else if (temp.peID == 3) {
					to3->write(temp);
				} else if (temp.peID == 4) {
					to4->write(temp);
				}
			}
			enable = temp.end;
		}
	}

	temp.end = true;
	to1->write(temp);
	to2->write(temp);
	to3->write(temp);
	to4->write(temp);
}

void distributer_vcresult(vcresult_stream* from1, B_data_stream* val1, B_data_stream* val2, B_data_stream* val3, B_data_stream* val4,
		B_data_stream* cid1, B_data_stream* cid2, B_data_stream* cid3, B_data_stream* cid4){

	vccache_result temp;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!from1->empty()) {
			temp = from1->read();
			if (!temp.end){
				if (temp.type == B_VAL) {
					if (temp.peID == 1) {
						val1->write(temp.data);
					} else if (temp.peID == 2) {
						val2->write(temp.data);
					} else if (temp.peID == 3) {
						val3->write(temp.data);
					} else if (temp.peID == 4) {
						val4->write(temp.data);
					}
				} else {
					if (temp.peID == 1) {
						cid1->write(temp.data);
					} else if (temp.peID == 2) {
						cid2->write(temp.data);
					} else if (temp.peID == 3) {
						cid3->write(temp.data);
					} else if (temp.peID == 4) {
						cid4->write(temp.data);
					}
				}
			}
			enable = temp.end;
		}
	}
}

void distributer_vcresult(vcresult_stream* from1, cacheline_stream* val1, cacheline_stream* val2, cacheline_stream* val3, cacheline_stream* val4){

	vccache_result temp;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!from1->empty()) {
			temp = from1->read();
			if (!temp.end){
				if (temp.peID == 1) {
					val1->write(temp.data);
				} else if (temp.peID == 2) {
					val2->write(temp.data);
				} else if (temp.peID == 3) {
					val3->write(temp.data);
				} else if (temp.peID == 4) {
					val4->write(temp.data);
				}
			}
			enable = temp.end;
		}
	}
}

void distributer_vcresult(vcresult_stream* from1, vcresult_stream* val1, vcresult_stream* val2, vcresult_stream* val3, vcresult_stream* val4){

	vccache_result temp;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE II=1
		if (!from1->empty()) {
			temp = from1->read();
			if (!temp.end){
				if (temp.peID == 1) {
					val1->write(temp);
				} else if (temp.peID == 2) {
					val2->write(temp);
				} else if (temp.peID == 3) {
					val3->write(temp);
				} else if (temp.peID == 4) {
					val4->write(temp);
				}
			}
			enable = temp.end;
		}
	}

	temp.end = true;
	val1->write(temp);
	val2->write(temp);
	val3->write(temp);
	val4->write(temp);
}

void merge_8_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* from3,
		vcresult_stream* from4, vcresult_stream* from5, vcresult_stream* from6, vcresult_stream* from7,
		vcresult_stream* from8, vcresult_stream* to){

	bool from1_end = false;
	bool from2_end = false;
	bool from3_end = false;
	bool from4_end = false;
	bool from5_end = false;
	bool from6_end = false;
	bool from7_end = false;
	bool from8_end = false;

	vccache_result temp;

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
			} else if (!from3->empty()) {
				temp = from3->read();
				from3_end = temp.end;
				if (!from3_end)
					to->write(temp);
			} else if (!from4->empty()) {
				temp = from4->read();
				from4_end = temp.end;
				if (!from4_end)
					to->write(temp);
			} else if (!from5->empty()) {
				temp = from5->read();
				from5_end = temp.end;
				if (!from5_end)
					to->write(temp);
			} else if (!from6->empty()) {
				temp = from6->read();
				from6_end = temp.end;
				if (!from6_end)
					to->write(temp);
			} else if (!from7->empty()) {
				temp = from7->read();
				from7_end = temp.end;
				if (!from7_end)
					to->write(temp);
			} else if (!from8->empty()) {
				temp = from8->read();
				from8_end = temp.end;
				if (!from8_end)
					to->write(temp);
			}
		}
		enable = from1_end && from2_end && from3_end && from4_end &&
				from5_end && from6_end && from7_end && from8_end;
	}

	temp.end = true;
	to->write(temp);
}

void merge_2_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, cacheline_stream* to){

	// merge requests from the 8 banks to one single stream in round-robin fashion
	bool whether_read1 = false;
	bool whether_read2 = false;

	bool have_read1 = false;
	bool have_read2 = false;

	bool end1 = false;
	bool end2 = false;

	vccache_result temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || from1->empty()) && (have_read2 || from2->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
			}

			whether_read1 = !have_read1 && !from1->empty();
			whether_read2 = !have_read2 && !from2->empty();

			if (whether_read1) {
				temp = from1->read();

				if (!temp.end)
					to->write(temp.data);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = from2->read();

				if (!temp.end)
					to->write(temp.data);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			}

			enable = end1 && end2;
		}
	}
}

void merge_2_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* to){

	// merge requests from the 8 banks to one single stream in round-robin fashion
	bool whether_read1 = false;
	bool whether_read2 = false;

	bool have_read1 = false;
	bool have_read2 = false;

	bool end1 = false;
	bool end2 = false;

	vccache_result temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || from1->empty()) && (have_read2 || from2->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
			}

			whether_read1 = !have_read1 && !from1->empty();
			whether_read2 = !have_read2 && !from2->empty();

			if (whether_read1) {
				temp = from1->read();

				if (!temp.end)
					to->write(temp);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = from2->read();

				if (!temp.end)
					to->write(temp);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			}

			enable = end1 && end2;
		}
	}

	temp.end = true;
	to->write(temp);
}

void merge_4_to_1_vcresult(vcresult_stream* from1, vcresult_stream* from2, vcresult_stream* from3,
		vcresult_stream* from4, vcresult_stream* to){

	// merge requests from the 8 banks to one single stream in round-robin fashion
	bool whether_read1 = false;
	bool whether_read2 = false;
	bool whether_read3 = false;
	bool whether_read4 = false;

	bool have_read1 = false;
	bool have_read2 = false;
	bool have_read3 = false;
	bool have_read4 = false;

	bool end1 = false;
	bool end2 = false;
	bool end3 = false;
	bool end4 = false;

	vccache_result temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || from1->empty()) && (have_read2 || from2->empty()) &&
					(have_read3 || from3->empty()) && (have_read4 || from4->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
				have_read3 = false;
				have_read4 = false;
			}

			whether_read1 = !have_read1 && !from1->empty();
			whether_read2 = !have_read2 && !from2->empty();
			whether_read3 = !have_read3 && !from3->empty();
			whether_read4 = !have_read4 && !from4->empty();

			if (whether_read1) {
				temp = from1->read();

				if (!temp.end)
					to->write(temp);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = from2->read();

				if (!temp.end)
					to->write(temp);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			} else if (whether_read3) {
				temp = from3->read();

				if (!temp.end)
					to->write(temp);

				whether_read3 = false;
				have_read3 = true;
				end3 = temp.end;
			} else if (whether_read4) {
				temp = from4->read();

				if (!temp.end)
					to->write(temp);

				whether_read4 = false;
				have_read4 = true;
				end4 = temp.end;
			}

			enable = end1 && end2 && end3 && end4;
		}
	}

	temp.end = true;
	to->write(temp);
}

void merge_4_to_1_rresponse(rresponse_stream* from1, rresponse_stream* from2, rresponse_stream* from3,
		rresponse_stream* from4, rresponse_stream* to){

	bool from1_end = false;
	bool from2_end = false;
	bool from3_end = false;
	bool from4_end = false;

	rcache_response temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if (!from1->empty()) {
				temp = from1->read();
				from1_end = temp.end;
				if (!from1_end && !temp.no_use)
					to->write(temp);
			} else if (!from2->empty()) {
				temp = from2->read();
				from2_end = temp.end;
				if (!from2_end && !temp.no_use)
					to->write(temp);
			} else if (!from3->empty()) {
				temp = from3->read();
				from3_end = temp.end;
				if (!from3_end && !temp.no_use)
					to->write(temp);
			} else if (!from4->empty()) {
				temp = from4->read();
				from4_end = temp.end;
				if (!from4_end && !temp.no_use)
					to->write(temp);
			}
		}
		enable = from1_end && from2_end && from3_end && from4_end;
	}

//	temp.end = true;
//	to->write(temp);
}

void merge_2_to_1_vcresponse(vcresponse_stream* from1, vcresponse_stream* from2, vcresponse_stream* to){

	// merge requests from the 8 banks to one single stream in round-robin fashion
	bool whether_read1 = false;
	bool whether_read2 = false;

	bool have_read1 = false;
	bool have_read2 = false;

	bool end1 = false;
	bool end2 = false;

	vccache_response temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if ((have_read1 || from1->empty()) && (have_read2 || from2->empty())){
				// if all the sub_streams have been read or empty, then we update the have_read flag
				// have_read --> false means that the sub_stream can be read
				have_read1 = false;
				have_read2 = false;
			}

			whether_read1 = !have_read1 && !from1->empty();
			whether_read2 = !have_read2 && !from2->empty();

			if (whether_read1) {
				temp = from1->read();

				if (!temp.end)
					to->write(temp);

				whether_read1 = false;
				have_read1 = true;
				end1 = temp.end;
			} else if (whether_read2) {
				temp = from2->read();

				if (!temp.end)
					to->write(temp);

				whether_read2 = false;
				have_read2 = true;
				end2 = temp.end;
			}

			enable = end1 && end2;
		}
	}

	temp.end = true;
	to->write(temp);
}

void final_merger(merge_stream* from1, merge_stream* from2, merge_stream* result, id_stream* result_len, id_stream* result_len1){

	merge_item temp, temp1;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		// if the two sub-streams are not empty
		if (!from1->empty() && !from2->empty()) {
			temp = from1->read();
			temp1 = from2->read();
			if (!temp.global_end && !temp1.global_end) {
				// shall we read from the streams
				bool hold_s1 = true;
				bool hold_s2 = true;

				id_t counter = 0;

				for (bool enable_i=false, enable_j=false; !enable_i || !enable_j; ){
			#pragma HLS PIPELINE II=1
					if (!hold_s1 && !enable_i){
						temp = from1->read();
						enable_i = temp.end;
					}
					if (!hold_s2 && !enable_j) {
						temp1 = from2->read();
						enable_j = temp1.end;
					}

					if (enable_i)
						temp.cid = 0xFFFFFFFF;

					if (enable_j)
						temp1.cid = 0xFFFFFFFF;

					if (!(enable_i && enable_j)) {
						if (temp.cid < temp1.cid) {
							result->write(temp);
							hold_s2 = true;
							hold_s1 = false;
							counter++;
						} else if (temp.cid > temp1.cid) {
							result->write(temp1);
							hold_s1 = true;
							hold_s2 = false;
							counter++;
						} else {
							val_t m_result = temp.val + temp1.val;
						#pragma HLS BIND_OP variable=m_result op=fadd impl=fulldsp
							merge_item temp2;
							temp2.cid = temp.cid;
							temp2.val = m_result;
							result->write(temp2);

							hold_s1 = false;
							hold_s2 = false;
							counter++;
						}
					}
				}
				result_len->write(counter);
				result_len1->write(counter);
			}
			enable = temp.global_end && temp1.global_end;
		}
	}

	merge_item temp_mi;
	temp_mi.global_end = true;
	result->write(temp_mi);

	result_len->write(0);
	result_len1->write(TERMINATE);
}

void sub_final_merger(merge_stream* from1, merge_stream* from2, merge_stream* result){

	merge_item temp, temp1;

	// shall we read from the streams

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		// if the two sub-streams are not empty
		if (!from1->empty() && !from2->empty()) {
			temp = from1->read();
			temp1 = from2->read();

			if (!temp.global_end && !temp1.global_end) {

				bool hold_s1 = true;
				bool hold_s2 = true;
				for (bool enable_i=false, enable_j=false; !enable_i || !enable_j; ){
			#pragma HLS PIPELINE II=1
					if (!hold_s1 && !enable_i){
						temp = from1->read();
						enable_i = temp.end;
					}
					if (!hold_s2 && !enable_j) {
						temp1 = from2->read();
						enable_j = temp1.end;
					}

					if (enable_i)
						temp.cid = 0xFFFFFFFF;

					if (enable_j)
						temp1.cid = 0xFFFFFFFF;

					if (!(enable_i && enable_j)) {
						if (temp.cid < temp1.cid) {
							result->write(temp);
							hold_s2 = true;
							hold_s1 = false;
						} else if (temp.cid > temp1.cid) {
							result->write(temp1);
							hold_s1 = true;
							hold_s2 = false;
						} else {
							val_t m_result = temp.val + temp1.val;
						#pragma HLS BIND_OP variable=m_result op=fadd impl=fulldsp
							merge_item temp2;
							temp2.cid = temp.cid;
							temp2.val = m_result;
							result->write(temp2);

							hold_s1 = false;
							hold_s2 = false;
						}
					}
				}

				merge_item tempiii;
				tempiii.end = true;
				tempiii.global_end = false;
				result->write(tempiii);
			}
			enable = temp.global_end && temp1.global_end;
		}
	}

	merge_item temp_mi;
	temp_mi.global_end = true;
	result->write(temp_mi);
}

void convert_pblock_to_mergeitem(partial_stream* from, merge_stream* to){

	partial_block temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!from->empty()) {
			temp = from->read();

			if (!temp.finish) {
				for (id_t i=temp.begin; i<=temp.end; i++){
		#pragma HLS PIPELINE II=1
					merge_item temp_m;
					temp_m.cid = temp.cid(i*32+31, i*32);
					temp_m.val = Reinterpret<val_t>(static_cast<ap_uint<32> >(temp.data(i*32+31, i*32)));
					temp_m.end = false;
					temp_m.global_end = false;

					to->write(temp_m);
				}
			}

			enable = temp.finish;
		}
	}

	merge_item temp_m;
	temp_m.global_end = true;
	to->write(temp_m);
}

void write_C_rid_rlen(id_stream* rlen, id_t* C_rcsr, id_t crum){

	id_t curr_idx = 0;

	for (id_t i=0; i<crum+1; i++){
#pragma HLS PIPELINE II=1
		if (i>=1) {
			curr_idx += rlen->read();
		}
		C_rcsr[i] = curr_idx;

#ifndef __SYNTHESIS__
		break;
#endif
	}
}

void write_C_val_cid(merge_stream* final_result, val_t* C_val, id_t* C_cid, id_t* counter_t, id_stream* result_len){

	bool enable = false;
	id_t counter = 0;

	for (; !enable; ){
#pragma HLS PIPELINE off
		if (!result_len->empty()) {
			id_t temp_len = result_len->read();
			if (temp_len != TERMINATE) {
				for (id_t ttt=counter; ttt<temp_len+counter; ttt++) {
#pragma HLS PIPELINE II=1
					merge_item t_mi = final_result->read();
					C_val[ttt] = t_mi.val;
					C_cid[ttt] = t_mi.cid;
				}
				counter += temp_len;
			}
			enable = temp_len == TERMINATE;
		}
	}

	*counter_t = counter;
}
