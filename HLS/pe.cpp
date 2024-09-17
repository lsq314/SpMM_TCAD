#include "pe.h"

void pe(block_stream* B_cids, block_stream* B_vals, block_stream* B_cidsd, block_stream* B_valsd, pe_stream* task, partial_stream* partial,
        id_stream* B_rlen, mcontrol_stream* mcontrol, bool_stream* pe_idle, bool_stream* efinish, id_t* workload_counter){

	pe_idle->write(false);

	block temp_v, temp_c;

	id_t _workload_counter = 0;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE off
		pe_job curr_job = task->read();
		// if no jobs, PE won't read other FIFOs
		if (!curr_job.end) {
			id_t b_rlen = B_rlen->read();

			// control the merger of current B_row
			merge_control control;
			control.rid = curr_job.rid;
			control.rlen = b_rlen;
			control.end = false;

			mcontrol->write(control);

			_workload_counter += b_rlen;

			for (id_t i=0; i<b_rlen; ) {
	#pragma HLS PIPELINE II=1
				// PE receives cache blocks
				if (b_rlen > 16) {
					temp_v = B_vals->read();
					temp_c = B_cids->read();
				} else {
					temp_v = B_valsd->read();
					temp_c = B_cidsd->read();
				}

				val_t data1 = Reinterpret<val_t>(static_cast<ap_uint<32> >(temp_v.data(31, 0)));
				val_t data2 = Reinterpret<val_t>(static_cast<ap_uint<32> >(temp_v.data(63, 32)));
				val_t data3 = Reinterpret<val_t>(static_cast<ap_uint<32> >(temp_v.data(95, 64)));
				val_t data4 = Reinterpret<val_t>(static_cast<ap_uint<32> >(temp_v.data(127, 96)));

				val_t psum1 = data1 * curr_job.val;
			#pragma HLS BIND_OP variable=psum1 op=fmul impl=fulldsp
				val_t psum2 = data2 * curr_job.val;
			#pragma HLS BIND_OP variable=psum2 op=fmul impl=fulldsp
				val_t psum3 = data3 * curr_job.val;
			#pragma HLS BIND_OP variable=psum3 op=fmul impl=fulldsp
				val_t psum4 = data4 * curr_job.val;
			#pragma HLS BIND_OP variable=psum4 op=fmul impl=fulldsp

				ap_uint<128> result;
				result(31, 0) = Reinterpret<ap_uint<32> >(psum1);
				result(63, 32) = Reinterpret<ap_uint<32> >(psum2);
				result(95, 64) = Reinterpret<ap_uint<32> >(psum3);
				result(127, 96) = Reinterpret<ap_uint<32> >(psum4);

				partial_block pblock;
				pblock.begin = temp_v.begin;
				pblock.end = temp_v.end;
				pblock.cid = temp_c.data;
				pblock.data = result;
				pblock.finish = false;

				partial->write(pblock);

				i += temp_v.end - temp_v.begin + 1;
			}

			efinish->read();
			pe_idle->write(false);
		}
		enable = curr_job.end;
	}

	*workload_counter = _workload_counter;

	merge_control temp_mc;
	temp_mc.end = true;
	temp_mc.rlen = 0;
	mcontrol->write(temp_mc);

	partial_block temp_pb;
	temp_pb.finish = true;
	partial->write(temp_pb);
}

void merge_controller(mcontrol_stream* control, mcontrolr_stream* control_result){

	merge_control temp = control->read();
	id_t rid = temp.rid;

	// source --> the index of temp_result array
	// whether_push --> whether push the current results
	merge_control_r result;
	result.rlen = temp.rlen;
	result.source = 1;
	result.whether_push = false;

	control_result->write(result);

	bool source_choose = true;

	for (bool enable=temp.end; !enable; ){
#pragma HLS PIPELINE off
		if (!control->empty()) {
			temp = control->read();

			if (!temp.end) {
				result.whether_push = temp.rid != rid;
				result.source = (source_choose) ? 2 : 1;
				result.rlen = temp.rlen;
				result.end = false;

				control_result->write(result);

				rid = temp.rid;
				source_choose = !source_choose;
			}
			enable = temp.end;
		}
	}

	// push the remaining one
	result.rlen = 0;
	result.whether_push = true;
	result.source = (source_choose) ? 2 : 1;
	result.end = false;
	control_result->write(result);

	// terminate
	result.end = true;
	control_result->write(result);
}

void _merge_temp(merge_stream* from1, merge_stream* from2, id_t len1, id_t *len2_t,
		merge_stream* result, id_t* length){

	// shall we read from the streams
	bool hold_s1 = false;
	bool hold_s2 = false;

	merge_item temp, temp1;

	id_t counter = 0;
	id_t len2 = *len2_t;

	for (id_t i=0, j=0; i<len1 || j<len2; ){
#pragma HLS PIPELINE II=1
		if (!hold_s1 && i<len1)
			temp = from1->read();
		if (!hold_s2 && j<len2)
			temp1 = from2->read();

		if (i==len1)
			temp.cid = 0xFFFFFFFF;

		if (j==len2)
			temp1.cid = 0xFFFFFFFF;

		if (temp.cid < temp1.cid) {
			result->write(temp);
			i++;
			hold_s2 = true;
			hold_s1 = false;
			counter++;
		} else if (temp.cid > temp1.cid) {
			result->write(temp1);
			j++;
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

			i++;
			j++;
			hold_s1 = false;
			hold_s2 = false;
			counter++;
		}
	}

	*length = counter;
	*len2_t = 0;
}

void two_merger(merge_stream* from1, merge_stream* from2, id_t len1, id_t *len2_t,
		merge_stream* result, id_t* length, merge_stream* from11, merge_stream* from21, id_t len11, id_t *len2_t1,
		merge_stream* result1, id_t* length1){

#pragma HLS DATAFLOW disable_start_propagation

	merge_stream temp_fifo;
#pragma HLS stream variable=temp_fifo depth=4

	_merge_temp(from1, from2, len1, len2_t, &temp_fifo, length);
	_merge_temp(&temp_fifo, from21, len11, len2_t1, result1, length1);
}

void merge_two_sub(mcontrolr_stream* control_s, merge_stream* result, merge_stream* from, bool_stream* efinish){

#pragma HLS ALLOCATION function instances=two_merger limit=1

	merge_stream ping, pong;
#pragma HLS stream variable=ping depth=1000
#pragma HLS stream variable=pong depth=1000
	merge_stream ping1, pong1;
#pragma HLS stream variable=ping1 depth=1000
#pragma HLS stream variable=pong1 depth=1000

	bool previous_dest = true;
	bool previous_dest1 = true;

	merge_stream ef1, ef2, ef3;

	id_t ef1_l=0, ef2_l=0, ef3_l=0;

	merge_item temp_mi;
	merge_control_r temp_cr;

	temp_mi.global_end = false;

	id_t ping_len = 0;
	id_t pong_len = 0;
	id_t ping1_len = 0;
	id_t pong1_len = 0;

	bool first_or_second = true;

	id_t curr_iter = 0;

	id_t minimum_index = 0;
	id_t s_minimum_index = 0;
	id_t f_minimum_index = 0;

	id_t minimum_length = 0;
	id_t s_minimum_length = 0;
	id_t f_minimum_length = 0;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE off
		if (!control_s->empty()) {
			temp_cr = control_s->read();

			if (!temp_cr.end) {
				if (temp_cr.whether_push) {

					if (f_minimum_index == 0) {
						two_merger(&pong, &ping1, pong_len, &ping1_len, &ping, &ping_len, &ping, &pong1, pong1_len, &ping_len, result, &curr_iter);
					} else if (f_minimum_index == 1) {
						two_merger(&ping, &ping1, ping_len, &ping1_len, &pong, &pong_len, &pong, &pong1, pong_len, &pong1_len, result, &curr_iter);
					} else if (f_minimum_index == 2) {
						two_merger(&ping, &pong, ping_len, &pong_len, &ping1, &ping1_len, &ping1, &pong1, ping1_len, &pong1_len, result, &curr_iter);
					} else if (f_minimum_index == 3) {
						two_merger(&ping, &ping1, ping_len, &ping1_len, &pong1, &pong1_len, &pong1, &pong, pong_len, &pong1_len, result, &curr_iter);
					}

					temp_mi.end = true;
					temp_mi.global_end = false;
					result->write(temp_mi);
					pong_len = 0;
					ping_len = 0;
					ping1_len = 0;
					pong1_len = 0;
					curr_iter = 0;
					first_or_second = true;
				}

				// ping-pong buffer
				if (temp_cr.rlen > 0) {
					if (curr_iter == 0)
						_merge_temp(from, &ping, temp_cr.rlen, &ping_len, &pong, &pong_len);
					else if (curr_iter == 1)
						_merge_temp(from, &ping1, temp_cr.rlen, &ping1_len, &pong1, &pong1_len);
					else if (curr_iter == 2)
						_merge_temp(from, &ping, temp_cr.rlen, &ping_len, &ping1, &ping1_len);
					else {
						if (f_minimum_index == 0 && minimum_index == 1)
							_merge_temp(from, &pong, temp_cr.rlen, &pong_len, &ping, &ping_len);
						else if (f_minimum_index == 0 && minimum_index == 2)
							_merge_temp(from, &ping1, temp_cr.rlen, &ping1_len, &ping, &ping_len);
						else if (f_minimum_index == 0 && minimum_index == 3)
							_merge_temp(from, &pong1, temp_cr.rlen, &pong1_len, &ping, &ping_len);
						else if (f_minimum_index == 1 && minimum_index == 0)
							_merge_temp(from, &ping, temp_cr.rlen, &ping_len, &pong, &pong_len);
						else if (f_minimum_index == 1 && minimum_index == 2)
							_merge_temp(from, &ping1, temp_cr.rlen, &ping1_len, &pong, &pong_len);
						else if (f_minimum_index == 1 && minimum_index == 3)
							_merge_temp(from, &pong1, temp_cr.rlen, &pong1_len, &pong, &pong_len);
						else if (f_minimum_index == 2 && minimum_index == 0)
							_merge_temp(from, &ping, temp_cr.rlen, &ping_len, &ping1, &ping1_len);
						else if (f_minimum_index == 2 && minimum_index == 1)
							_merge_temp(from, &pong, temp_cr.rlen, &pong_len, &ping1, &ping1_len);
						else if (f_minimum_index == 2 && minimum_index == 3)
							_merge_temp(from, &pong1, temp_cr.rlen, &pong1_len, &ping1, &ping1_len);
						else if (f_minimum_index == 3 && minimum_index == 0)
							_merge_temp(from, &ping, temp_cr.rlen, &ping_len, &pong1, &pong1_len);
						else if (f_minimum_index == 3 && minimum_index == 1)
							_merge_temp(from, &pong, temp_cr.rlen, &pong_len, &pong1, &pong1_len);
						else if (f_minimum_index == 3 && minimum_index == 2)
							_merge_temp(from, &ping1, temp_cr.rlen, &ping1_len, &pong1, &pong1_len);
					}
				}

				minimum_index = (ping_len < pong_len) ? 0 : 1;
				minimum_length = (ping_len < pong_len) ? ping_len : pong_len;

				s_minimum_index = (ping1_len < pong1_len) ? 2 : 3;
				s_minimum_length = (ping1_len < pong1_len) ? ping1_len : pong1_len;

				f_minimum_index = (minimum_length < s_minimum_length) ? minimum_index : s_minimum_index;

				if (minimum_length < s_minimum_length) {
					if (minimum_index == 0)
						minimum_index = (pong_len < s_minimum_length) ? 1 : s_minimum_index;
					else
						minimum_index = (ping_len < s_minimum_length) ? 0 : s_minimum_index;
				} else {
					if (s_minimum_index == 2)
						minimum_index = (pong1_len < minimum_length) ? 3 : minimum_index;
					else
						minimum_index = (ping1_len < minimum_length) ? 2 : minimum_index;
				}

				curr_iter += 1;
				efinish->write(false);
			}

			enable = temp_cr.end;
		}
	}

	temp_mi.end = true;
	temp_mi.global_end = true;
	result->write(temp_mi);
}

void merger(mcontrol_stream* control, merge_stream* from, merge_stream* result, bool_stream* efinish){

#pragma HLS DATAFLOW

	mcontrolr_stream mresult;
#pragma HLS STREAM variable=mresult depth=8

	merge_controller(control, &mresult);
	merge_two_sub(&mresult, result, from, efinish);
}

void generate_mccontrol(mcontrol_stream* mcontrol, merge_stream* to, val_t* in, bool_stream* finish){

	for (id_t i=0; i<5; i++) {
#pragma HLS PIPELINE off
		merge_control control;
		control.rid = 0;
		control.rlen = 70;
		control.end = false;
		mcontrol->write(control);

		for (id_t j=0; j<70; j++) {
#pragma HLS PIPELINE II=1
			merge_item mi;
			mi.end = false;
			mi.global_end = false;
			mi.cid = i*70+j;
			mi.val = in[i*70+j];
			to->write(mi);
		}

		finish->read();
	}

	merge_control temp_mc;
	temp_mc.end = true;
	temp_mc.rlen = 0;
	temp_mc.rid = 0;
	mcontrol->write(temp_mc);
}

void from_ms_to_val(merge_stream* from, val_t* out){

	for (id_t i=0; i<350; i++) {
#pragma HLS PIPELINE II=1
		out[i] = from->read().val;
	}
}

void merger_test(val_t* in, val_t* out){

#pragma HLS INTERFACE m_axi depth=350 port=in offset=slave bundle=in
#pragma HLS INTERFACE m_axi depth=350 port=out offset=slave bundle=out

#pragma HLS INTERFACE s_axilite port=return bundle=ctr

#pragma HLS DATAFLOW disable_start_propagation

	mcontrol_stream mt;
	bool_stream bs;
	merge_stream ms, mr;
#pragma HLS STREAM variable=ms depth=70

	generate_mccontrol(&mt, &ms, in, &bs);
	merger(&mt, &ms, &mr, &bs);
	from_ms_to_val(&mr, out);
}

