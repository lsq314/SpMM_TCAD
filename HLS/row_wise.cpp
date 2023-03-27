#include "core.h"
#include "util.h"
#include "rcache.h"
#include "vccache.h"
#include "row_wise.h"

struct _pe_A_row {
	id_t ridx;
	id_t rlen;
	id_t rid;
	bool end;
};

typedef hls::stream<_pe_A_row> _pe_j_stream;

void A_row_reader_rw(id_t* A_rcsr, id_t rnum, rinfo_stream* A_rlenstream){

	// read A_csr pointer array, read r[i] and r[i+1], totally rnum+1 elements
	// rinfo: rid, rlen, totally rnum rows
	id_t previous_idx = 0;
	rinfo temp_rinfo;

	for (id_t i=0; i<rnum+1; i++) {
#pragma HLS PIPELINE II=1
		id_t current_idx = A_rcsr[i];
		if (i >= 1) {
			// TODO: empty rows, for now we assume no empty rows
			temp_rinfo.rid = i - 1;
			temp_rinfo.rlen = current_idx - previous_idx;
			A_rlenstream->write(temp_rinfo);
		}
		previous_idx = current_idx;
	}

	// to avoid pipeline stalling
	temp_rinfo.rid = 0;
	temp_rinfo.rlen = 0;
	A_rlenstream->write(temp_rinfo);
}

void A_element_distributor_rw(rinfo_stream* rinfos, id_t tnum,
		bool_stream* pe1_idle, bool_stream* pe2_idle, bool_stream* pe3_idle, bool_stream* pe4_idle,
		_pe_j_stream* pe1_job, _pe_j_stream* pe2_job, _pe_j_stream* pe3_job, _pe_j_stream* pe4_job,
		id_stream* target, id_stream* target2, id_stream* whether_assign){

	_pe_A_row temp_pe;
	id_t base_index = 0;
	id_t curr_target = 0;

	// if there is an available PE and task, assign the task
	// totally tnum tasks
	for (id_t i=0; i<tnum; ) {
#pragma HLS PIPELINE II=1
		if (i<4 || !whether_assign->empty()) {
//		if (!pe1_idle->empty() || !pe2_idle->empty() || !pe3_idle->empty() || !pe4_idle->empty()) {

			if (!whether_assign->empty())
				whether_assign->read();

			rinfo trinfo = rinfos->read();

			temp_pe.rid = trinfo.rid;
			temp_pe.rlen = trinfo.rlen;
			temp_pe.ridx = base_index;
			temp_pe.end = false;

			base_index += trinfo.rlen;

			curr_target = i%4 + 1;

			if (curr_target == 1) {
//				pe1_idle->read();
				pe1_job->write(temp_pe);

//				target->write(1);
//				target2->write(1);
			} else if (curr_target == 2) {
//				pe2_idle->read();
				pe2_job->write(temp_pe);

//				target->write(2);
//				target2->write(2);
			} else if (curr_target == 3) {
//				pe3_idle->read();
				pe3_job->write(temp_pe);

//				target->write(3);
//				target2->write(3);
			} else if (curr_target == 4) {
//				pe4_idle->read();
				pe4_job->write(temp_pe);

//				target->write(4);
//				target2->write(4);
			}

			i++;
		}

#ifndef __SYNTHESIS__
		i++;
#endif
	}

	temp_pe.end = true;

	pe1_job->write(temp_pe);
	pe2_job->write(temp_pe);
	pe3_job->write(temp_pe);
	pe4_job->write(temp_pe);
}

void reading_A_row_elements(_pe_j_stream* _c_pe_job, hls::burst_maxi<id_t> A_cid, hls::burst_maxi<val_t> A_val, pe_stream* _pe_s,
		id_t* Brcsr, id_stream* Brlen, ap_uint<128>* B_cid, ap_uint<128>* B_val, block_stream* _bval, block_stream* _bcid,
		bool_stream* pe_idle, bool_stream* finish){

	id_t Bridx[2];
//	pe_idle->write(true);

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!_c_pe_job->empty()) {
			_pe_A_row trow = _c_pe_job->read();
			if (!trow.end) {
				A_cid.read_request(trow.ridx, trow.rlen);
				A_val.read_request(trow.ridx, trow.rlen);
				for (id_t i=0; i<trow.rlen; i++) {
#pragma HLS PIPELINE off
					id_t c_cid = A_cid.read();
					val_t c_val = A_val.read();

					pe_job tpe;
					tpe.rid = trow.rid;
					tpe.val = c_val;
					tpe.end = false;
					tpe.partial_end = i==trow.rlen-1;
					_pe_s->write(tpe);

					for (id_t ii=c_cid; ii<2+c_cid; ii++)
						Bridx[ii-c_cid] = Brcsr[ii];

					Brlen->write(Bridx[1] - Bridx[0]);

					id_t begin_block_idx = Bridx[0] / 4;
					id_t end_block_idx = (Bridx[1] - 1) / 4;
					ap_uint<3> begin, end;
					block result_block, result_block1;

					for (id_t _t=begin_block_idx; _t<=end_block_idx; _t++){
						begin = (_t == begin_block_idx) ? (Bridx[0] % 4) : 0;
						end = (_t == end_block_idx) ? (Bridx[1] - 1) % 4 : 3;

						result_block.begin = begin;
						result_block.end = end;
						result_block.data = B_cid[_t];

						result_block1.begin = begin;
						result_block1.end = end;
						result_block1.data = B_val[_t];

						_bval->write(result_block1);
						_bcid->write(result_block);
					}

					finish->read();
				}
//				pe_idle->write(true);
			}
			enable = trow.end;
		}
	}

	pe_job tpe;
	tpe.end = true;
	_pe_s->write(tpe);
}

void pe_rw(block_stream* B_cids, block_stream* B_vals, pe_stream* task, partial_stream* partial,
        id_stream* B_rlen, mcontrol_stream* mcontrol){

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
			control.partial_end = curr_job.partial_end;

			mcontrol->write(control);

			for (id_t i=0; i<b_rlen; ) {
	#pragma HLS PIPELINE II=1
				// PE receives cache blocks
				block temp_v = B_vals->read();
				block temp_c = B_cids->read();

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
		}
		enable = curr_job.end;
	}

	merge_control control;
	control.end = true;
	control.rlen = 0;
	mcontrol->write(control);

	partial_block temp_pb;
	temp_pb.finish = true;
	partial->write(temp_pb);
}

void merge_controller_rw(mcontrol_stream* control, mcontrolr_stream* control_result){

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
				result.whether_push = temp.partial_end;
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
//	result.rlen = 0;
//	result.whether_push = true;
//	result.source = (source_choose) ? 2 : 1;
//	result.end = false;
//	control_result->write(result);

	// terminate
	result.end = true;
	control_result->write(result);
}

void _merge_temp_rw(merge_stream* from1, merge_stream* from2, id_t len1, id_t *len2_t,
		merge_stream* result, id_t* length){

#pragma HLS INLINE

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

void merge_two_sub_rw(mcontrolr_stream* control_s, merge_stream* result, merge_stream* from, bool_stream* efinish, id_stream* rlen, id_stream* rlen1){

	merge_stream ping, pong;
#pragma HLS stream variable=ping depth=1000
#pragma HLS stream variable=pong depth=1000

	bool previous_dest = true;

	merge_item temp_mi;
	merge_control_r temp_cr;

	temp_mi.global_end = false;

	id_t ping_len = 0;
	id_t pong_len = 0;

	for (bool enable=false; !enable; ) {
#pragma HLS PIPELINE off
		if (!control_s->empty()) {
			temp_cr = control_s->read();

			if (!temp_cr.end) {
				// ping-pong buffer
				if (temp_cr.rlen > 0) {
					if (temp_cr.source == 1)
						_merge_temp_rw(from, &ping, temp_cr.rlen, &ping_len, &pong, &pong_len);
					else
						_merge_temp_rw(from, &pong, temp_cr.rlen, &pong_len, &ping, &ping_len);
				}

				previous_dest = (temp_cr.source == 1) ? true : false;

				if (temp_cr.whether_push) {
					if (previous_dest){
						rlen->write(pong_len);
						rlen1->write(pong_len);
						for (id_t fi=0; fi<pong_len; fi++) {
					#pragma HLS PIPELINE II=1
							result->write(pong.read());
						}
//						temp_mi.end = true;
//						temp_mi.global_end = false;
//						result->write(temp_mi);
						pong_len = 0;
					} else {
						rlen->write(ping_len);
						rlen1->write(ping_len);
						for (id_t fi=0; fi<ping_len; fi++) {
					#pragma HLS PIPELINE II=1
							result->write(ping.read());
						}
//						temp_mi.end = true;
//						temp_mi.global_end = false;
//						result->write(temp_mi);
						ping_len = 0;
					}
				}

				efinish->write(false);
			}

			enable = temp_cr.end;
		}
	}

	temp_mi.end = true;
	temp_mi.global_end = true;
	result->write(temp_mi);
}

void merger_rw(mcontrol_stream* control, merge_stream* from, merge_stream* result, bool_stream* efinish, id_stream* rlen, id_stream* rlen1){

#pragma HLS DATAFLOW

	mcontrolr_stream mresult;
#pragma HLS STREAM variable=mresult depth=8

	merge_controller_rw(control, &mresult);
	merge_two_sub_rw(&mresult, result, from, efinish, rlen, rlen1);
}

void consume_result(merge_stream* ms) {
	for (bool enable=false; !enable; ){
		merge_item mi = ms->read();
		enable = mi.global_end;
	}
}

void write_C_r_rw(id_stream* rlen1, id_stream* rlen2, id_stream* rlen3, id_stream* rlen4, id_t* C_rcsr, id_t rnum, id_stream* target){

	id_t base_index = 0;

	for (id_t i=0; i<rnum; i++) {
#pragma HLS PIPELINE off
//		id_t curr_tar = target->read();
		id_t curr_tar = i%4 + 1;

#ifndef __SYNTHESIS__
		break;
#endif
		C_rcsr[i] = base_index;

		if (curr_tar == 1)
			base_index += rlen1->read();
		else if (curr_tar == 2)
			base_index += rlen2->read();
		else if (curr_tar == 3)
			base_index += rlen3->read();
		else if (curr_tar == 4)
			base_index += rlen4->read();
	}
}

void write_C_val_cid_rw(id_stream* target, val_t* C_val, id_t* C_cid, id_stream* whether_assign,
		merge_stream* result1, merge_stream* result2, merge_stream* result3, merge_stream* result4, id_t rnum,
		id_stream* rlen1, id_stream* rlen2, id_stream* rlen3, id_stream* rlen4){

	id_t base_index = 0;

	for (id_t i=0; i<rnum; i++) {
#pragma HLS PIPELINE off
//		id_t curr_tar = target->read();
		id_t curr_tar = i%4 + 1;

#ifndef __SYNTHESIS__
		break;
#endif

		if (curr_tar == 1){
			id_t curr_rlen = rlen1->read();
			for (id_t ii=base_index; ii<curr_rlen+base_index; ii++){
				merge_item mi = result1->read();
				C_val[ii] = mi.val;
				C_cid[ii] = mi.cid;
			}
			base_index += curr_rlen;
			whether_assign->write(1);
		} else if (curr_tar == 2){
			id_t curr_rlen = rlen2->read();
			for (id_t ii=base_index; ii<curr_rlen+base_index; ii++){
				merge_item mi = result2->read();
				C_val[ii] = mi.val;
				C_cid[ii] = mi.cid;
			}
			base_index += curr_rlen;
			whether_assign->write(1);
		} else if (curr_tar == 3){
			id_t curr_rlen = rlen3->read();
			for (id_t ii=base_index; ii<curr_rlen+base_index; ii++){
				merge_item mi = result3->read();
				C_val[ii] = mi.val;
				C_cid[ii] = mi.cid;
			}
			base_index += curr_rlen;
			whether_assign->write(1);
		} else if (curr_tar == 4){
			id_t curr_rlen = rlen4->read();
			for (id_t ii=base_index; ii<curr_rlen+base_index; ii++){
				merge_item mi = result4->read();
				C_val[ii] = mi.val;
				C_cid[ii] = mi.cid;
			}
			base_index += curr_rlen;
			whether_assign->write(1);
		}
	}
}


void spmm_row_wise(id_t* A_rcsr, id_t* A_cid, val_t* A_val, id_t arnum, id_t atnum,
		  id_t* B_rcsr1, id_t* B_rcsr2, id_t* B_rcsr3, id_t* B_rcsr4,
		  ap_uint<128>* B_rcsr5, ap_uint<128>* B_rcsr6, ap_uint<128>* B_rcsr7, ap_uint<128>* B_rcsr8,
		  hls::burst_maxi<val_t> B_val1, hls::burst_maxi<id_t> B_cid1, hls::burst_maxi<val_t> B_val2, hls::burst_maxi<id_t> B_cid2,
		  hls::burst_maxi<val_t> B_val3, hls::burst_maxi<id_t> B_cid3, hls::burst_maxi<val_t> B_val4, hls::burst_maxi<id_t> B_cid4,
		  ap_uint<128>* B_val5, ap_uint<128>* B_cid5, ap_uint<128>* B_val6, ap_uint<128>* B_cid6,
		  ap_uint<128>* B_val7, ap_uint<128>* B_cid7, ap_uint<128>* B_val8, ap_uint<128>* B_cid8,
		  id_t brnum, id_t btnum, ap_uint<128>* B_val_port1, ap_uint<128>* B_cid_port1,
		  ap_uint<128>* B_val_port2, ap_uint<128>* B_cid_port2, ap_uint<128>* B_val_port3, ap_uint<128>* B_cid_port3,
		  ap_uint<128>* B_val_port4, ap_uint<128>* B_cid_port4,
		  id_t* C_rcsr, id_t* C_cid, val_t* C_val,
		  id_t B_rcsr_base, id_t B_val_base, id_t B_cid_base,
		  id_t* r1acc, id_t* r1hit, id_t* r2acc, id_t* r2hit, id_t* r3acc, id_t* r3hit, id_t* r4acc, id_t* r4hit,
		  id_t* r5acc, id_t* r5hit, id_t* r6acc, id_t* r6hit, id_t* r7acc, id_t* r7hit, id_t* r8acc, id_t* r8hit,
		  id_t* vc1acc, id_t* vc1hit, id_t* vc2acc, id_t* vc2hit, id_t* vc3acc, id_t* vc3hit, id_t* vc4acc, id_t* vc4hit,
		  id_t* vc5acc, id_t* vc5hit, id_t* vc6acc, id_t* vc6hit, id_t* vc7acc, id_t* vc7hit, id_t* vc8acc, id_t* vc8hit,
		  id_t* fnum){

#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r1acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r2acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r3acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r4acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r5acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r6acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r7acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r8acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r1hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r2hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r3hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r4hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r5hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r6hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r7hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=r8hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=fnum

#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc1acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc1hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc2acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc2hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc3acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc3hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc4acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc4hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc5acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc5hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc6acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc6hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc7acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc7hit
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc8acc
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=vc8hit

#pragma HLS INTERFACE m_axi depth=13515 port=A_rcsr offset=slave bundle=arcsr
#pragma HLS INTERFACE m_axi depth=352762 port=A_cid offset=slave bundle=acid
#pragma HLS INTERFACE m_axi depth=352762 port=A_val offset=slave bundle=aval
#pragma HLS INTERFACE m_axi depth=1000 port=C_rcsr offset=slave bundle=crcsr
#pragma HLS INTERFACE m_axi depth=5000 port=C_cid offset=slave bundle=ccid
#pragma HLS INTERFACE m_axi depth=5000 port=C_val offset=slave bundle=cval
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr1 offset=slave bundle=brcsr1
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr2 offset=slave bundle=brcsr2
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr3 offset=slave bundle=brcsr3
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr4 offset=slave bundle=brcsr4
#pragma HLS INTERFACE m_axi depth=3379 port=B_rcsr5 offset=slave bundle=brcsr5
#pragma HLS INTERFACE m_axi depth=3379 port=B_rcsr6 offset=slave bundle=brcsr6
#pragma HLS INTERFACE m_axi depth=3379 port=B_rcsr7 offset=slave bundle=brcsr7
#pragma HLS INTERFACE m_axi depth=3379 port=B_rcsr8 offset=slave bundle=brcsr8
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port1 offset=slave bundle=bv1
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port1 offset=slave bundle=bc1
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port2 offset=slave bundle=bv2
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port2 offset=slave bundle=bc2
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port3 offset=slave bundle=bv3
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port3 offset=slave bundle=bc3
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port4 offset=slave bundle=bv4
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port4 offset=slave bundle=bc4

#pragma HLS INTERFACE m_axi depth=352762 port=B_val1 offset=slave bundle=bval1
#pragma HLS INTERFACE m_axi depth=352762 port=B_cid1 offset=slave bundle=bcid1
#pragma HLS INTERFACE m_axi depth=352762 port=B_val2 offset=slave bundle=bval2
#pragma HLS INTERFACE m_axi depth=352762 port=B_cid2 offset=slave bundle=bcid2
#pragma HLS INTERFACE m_axi depth=352762 port=B_val3 offset=slave bundle=bval3
#pragma HLS INTERFACE m_axi depth=352762 port=B_cid3 offset=slave bundle=bcid3
#pragma HLS INTERFACE m_axi depth=352762 port=B_val4 offset=slave bundle=bval4
#pragma HLS INTERFACE m_axi depth=352762 port=B_cid4 offset=slave bundle=bcid4
#pragma HLS INTERFACE m_axi depth=88191 port=B_val5 offset=slave bundle=bval5
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid5 offset=slave bundle=bcid5
#pragma HLS INTERFACE m_axi depth=88191 port=B_val6 offset=slave bundle=bval6
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid6 offset=slave bundle=bcid6
#pragma HLS INTERFACE m_axi depth=88191 port=B_val7 offset=slave bundle=bval7
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid7 offset=slave bundle=bcid7
#pragma HLS INTERFACE m_axi depth=88191 port=B_val8 offset=slave bundle=bval8
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid8 offset=slave bundle=bcid8

#pragma HLS INTERFACE s_axilite port=arnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=atnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=brnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=btnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_rcsr_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_val_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_cid_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=return bundle=ctr

#pragma HLS DATAFLOW disable_start_propagation

	// reading A rows and elements
	rinfo_stream _A_reader_rs;

	// indicate the PE is idle
	bool_stream pe1_idle, pe2_idle, pe3_idle, pe4_idle;

	// pass the task
	_pe_j_stream pe1_job, pe2_job, pe3_job, pe4_job;

	pe_stream pe1_j, pe2_j, pe3_j, pe4_j;

	id_stream brlen_pe1, brlen_pe2, brlen_pe3, brlen_pe4;

	id_stream whether_assign;
#pragma HLS STREAM variable=whether_assign depth=8

	id_stream target, target2;
#pragma HLS STREAM variable=target depth=100
#pragma HLS STREAM variable=target2 depth=100

	id_stream rlen1, rlen2, rlen3, rlen4,
			  rlen1r, rlen2r, rlen3r, rlen4r;
#pragma HLS STREAM variable=rlen1 depth=100
#pragma HLS STREAM variable=rlen2 depth=100
#pragma HLS STREAM variable=rlen3 depth=100
#pragma HLS STREAM variable=rlen4 depth=100
#pragma HLS STREAM variable=rlen1r depth=100
#pragma HLS STREAM variable=rlen2r depth=100
#pragma HLS STREAM variable=rlen3r depth=100
#pragma HLS STREAM variable=rlen4r depth=100

	B_data_stream vresult1, vresult2, vresult3, vresult4, cresult1, cresult2, cresult3, cresult4;

	// data block to PEs
	block_stream vblock1, vblock2, vblock3, vblock4, cblock1, cblock2, cblock3, cblock4;
#pragma HLS STREAM variable=vblock1 depth=100
#pragma HLS STREAM variable=vblock2 depth=100
#pragma HLS STREAM variable=vblock3 depth=100
#pragma HLS STREAM variable=vblock4 depth=100
#pragma HLS STREAM variable=cblock1 depth=100
#pragma HLS STREAM variable=cblock2 depth=100
#pragma HLS STREAM variable=cblock3 depth=100
#pragma HLS STREAM variable=cblock4 depth=100

	// merger control messages
	mcontrol_stream mc_pe1, mc_pe2, mc_pe3, mc_pe4;
	bool_stream efinish1, efinish2, efinish3, efinish4;

	merge_stream m_pe1, m_pe2, m_pe3, m_pe4;
#pragma HLS STREAM variable=m_pe1 depth=1000
#pragma HLS STREAM variable=m_pe2 depth=1000
#pragma HLS STREAM variable=m_pe3 depth=1000
#pragma HLS STREAM variable=m_pe4 depth=1000
	partial_stream p_pe1, p_pe2, p_pe3, p_pe4;
#pragma HLS STREAM variable=p_pe1 depth=1000
#pragma HLS STREAM variable=p_pe2 depth=1000
#pragma HLS STREAM variable=p_pe3 depth=1000
#pragma HLS STREAM variable=p_pe4 depth=1000

	// final merger for the current row
	merge_stream result_pe1, result_pe2, result_pe3, result_pe4;
#pragma HLS STREAM variable=result_pe1 depth=3000
#pragma HLS STREAM variable=result_pe2 depth=3000
#pragma HLS STREAM variable=result_pe3 depth=3000
#pragma HLS STREAM variable=result_pe4 depth=3000

	// read A's row infos: rid, rlen, ridx
	A_row_reader_rw(A_rcsr, arnum, &_A_reader_rs);

	// assign tasks to PEs and access to the rcache to get B_ridx and B_rlen
	A_element_distributor_rw(&_A_reader_rs, arnum, &pe1_idle, &pe2_idle, &pe3_idle, &pe4_idle,
			&pe1_job, &pe2_job, &pe3_job, &pe4_job, &target, &target2, &whether_assign);

	reading_A_row_elements(&pe1_job, B_cid1, B_val1, &pe1_j, B_rcsr1, &brlen_pe1, B_cid5, B_val5, &vblock1, &cblock1, &pe1_idle, &efinish1);
	reading_A_row_elements(&pe2_job, B_cid2, B_val2, &pe2_j, B_rcsr2, &brlen_pe2, B_cid6, B_val6, &vblock2, &cblock2, &pe2_idle, &efinish2);
	reading_A_row_elements(&pe3_job, B_cid3, B_val3, &pe3_j, B_rcsr3, &brlen_pe3, B_cid7, B_val7, &vblock3, &cblock3, &pe3_idle, &efinish3);
	reading_A_row_elements(&pe4_job, B_cid4, B_val4, &pe4_j, B_rcsr4, &brlen_pe4, B_cid8, B_val8, &vblock4, &cblock4, &pe4_idle, &efinish4);

	pe_rw(&cblock1, &vblock1, &pe1_j, &p_pe1, &brlen_pe1, &mc_pe1);
	pe_rw(&cblock2, &vblock2, &pe2_j, &p_pe2, &brlen_pe2, &mc_pe2);
	pe_rw(&cblock3, &vblock3, &pe3_j, &p_pe3, &brlen_pe3, &mc_pe3);
	pe_rw(&cblock4, &vblock4, &pe4_j, &p_pe4, &brlen_pe4, &mc_pe4);

	convert_pblock_to_mergeitem(&p_pe1, &m_pe1);
	convert_pblock_to_mergeitem(&p_pe2, &m_pe2);
	convert_pblock_to_mergeitem(&p_pe3, &m_pe3);
	convert_pblock_to_mergeitem(&p_pe4, &m_pe4);

	merger_rw(&mc_pe1, &m_pe1, &result_pe1, &efinish1, &rlen1, &rlen1r);
	merger_rw(&mc_pe2, &m_pe2, &result_pe2, &efinish2, &rlen2, &rlen2r);
	merger_rw(&mc_pe3, &m_pe3, &result_pe3, &efinish3, &rlen3, &rlen3r);
	merger_rw(&mc_pe4, &m_pe4, &result_pe4, &efinish4, &rlen4, &rlen4r);

//	consume_result(&result_pe1);
//	consume_result(&result_pe2);
//	consume_result(&result_pe3);
//	consume_result(&result_pe4);

	write_C_val_cid_rw(&target, C_val, C_cid, &whether_assign, &result_pe1, &result_pe2, &result_pe3, &result_pe4, arnum,
			&rlen1, &rlen2, &rlen3, &rlen4);
	write_C_r_rw(&rlen1r, &rlen2r, &rlen3r, &rlen4r, C_rcsr, arnum, &target2);

}


