#include "core.h"
#include "util.h"
#include "rcache.h"
#include "vccache.h"
#include "pe.h"
#include "task_scheduler.h"

void read_Bridx(rcache_stream* from, rcache_stream* to, ap_uint<128>* B_rcsr){

	ap_uint<128> Bridx[2];
	rcache_request curr_result;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!from->empty()) {
			rcache_request temp = from->read();

			if (!temp.end) {
				id_t curr_begin_idx = temp.Acid / 4;
				id_t curr_offset = temp.Acid % 4;

				for (id_t i=curr_begin_idx; i<curr_begin_idx+2; i++) {
					Bridx[i-curr_begin_idx] = B_rcsr[i];
				}
				curr_result.Acid = Bridx[0](curr_offset*32+31, curr_offset*32);
				curr_result.Acid1 = (curr_offset == 3) ? Bridx[1](31, 0) : Bridx[0]((curr_offset+1)*32+31, (curr_offset+1)*32);
				curr_result.dest = temp.dest;
				curr_result.data = 0;
				curr_result.type = CACHE_READ;
				curr_result.end = false;
				to->write(curr_result);
			}

			enable = temp.end;
		}
	}
	curr_result.dest = 7;
	to->write(curr_result);
}

void generate_data(B_request_stream* from, ap_uint<128>* B_val, ap_uint<128>* B_cid, block_stream* vblock, block_stream* cblock){

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE off
		if (!from->empty()) {
			B_row_request tb = from->read();
			if (!tb.end) {
				id_t begin_block_idx = tb.bridx / 4;
				id_t end_block_idx = (tb.bridx + tb.brlen - 1) / 4;
				ap_uint<3> begin, end;
				block result_block, result_block1;

				for (id_t _t=begin_block_idx; _t<=end_block_idx; _t++){
					begin = (_t == begin_block_idx) ? (tb.bridx % 4) : 0;
					end = (_t == end_block_idx) ? (tb.bridx + tb.brlen - 1) % 4 : 3;

					result_block.begin = begin;
					result_block.end = end;
					result_block.data = B_cid[_t];

					result_block1.begin = begin;
					result_block1.end = end;
					result_block1.data = B_val[_t];

					vblock->write(result_block1);
					cblock->write(result_block);
				}
			}
			enable = tb.end;
		}
	}
}

void merge_4_to_1_mi(mi_stream* from1, mi_stream* from2, mi_stream* from3,
		mi_stream* from4, bool_stream* to){

	to->write(true);

	bool from1_end = false;
	bool from2_end = false;
	bool from3_end = false;
	bool from4_end = false;

	manager_idle temp;

	for (bool enable=false; !enable; ){
#pragma HLS PIPELINE II=1
		if (!enable) {
			if (!from1->empty()) {
				temp = from1->read();
				from1_end = temp.end;
				if (!from1_end && temp.idle)
					to->write(true);
			} else if (!from2->empty()) {
				temp = from2->read();
				from2_end = temp.end;
				if (!from2_end && temp.idle)
					to->write(true);
			} else if (!from3->empty()) {
				temp = from3->read();
				from3_end = temp.end;
				if (!from3_end && temp.idle)
					to->write(true);
			} else if (!from4->empty()) {
				temp = from4->read();
				from4_end = temp.end;
				if (!from4_end && temp.idle)
					to->write(true);
			}
		}
		enable = from1_end && from2_end && from3_end && from4_end;
	}
}


void distributer_mi(mi_stream* from1, mi_stream* to1, mi_stream* to2, mi_stream* to3, mi_stream* to4){

	manager_idle temp;

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

void consume_merge(merge_stream* from){
	for (bool enable=false; !enable; ){
		if (!from->empty()) {
			merge_item ti = from->read();
			enable = ti.global_end;
		}
	}
}

void spmm(id_t* A_rcsr, id_t* A_cid, val_t* A_val, id_t arnum, id_t atnum,
		  ap_uint<128>* B_rcsr1, ap_uint<128>* B_rcsr2, ap_uint<128>* B_rcsr3, ap_uint<128>* B_rcsr4,
		  ap_uint<128>* B_val1, ap_uint<128>* B_cid1, ap_uint<128>* B_val2, ap_uint<128>* B_cid2,
		  ap_uint<128>* B_val3, ap_uint<128>* B_cid3, ap_uint<128>* B_val4, ap_uint<128>* B_cid4,
		  id_t brnum, id_t btnum, ap_uint<128>* B_val_port1, ap_uint<128>* B_cid_port1,
		  ap_uint<128>* B_val_port2, ap_uint<128>* B_cid_port2, ap_uint<128>* B_val_port3, ap_uint<128>* B_cid_port3,
		  ap_uint<128>* B_val_port4, ap_uint<128>* B_cid_port4,
		  id_t* C_rcsr, id_t* C_cid, val_t* C_val,
		  id_t B_rcsr_base, id_t B_val_base, id_t B_cid_base,
		  id_t* r1acc, id_t* r1hit, id_t* r2acc, id_t* r2hit, id_t* r3acc, id_t* r3hit, id_t* r4acc, id_t* r4hit,
		  id_t* r5acc, id_t* r5hit, id_t* r6acc, id_t* r6hit, id_t* r7acc, id_t* r7hit, id_t* r8acc, id_t* r8hit,
		  id_t* vc1acc, id_t* vc1hit, id_t* vc2acc, id_t* vc2hit, id_t* vc3acc, id_t* vc3hit, id_t* vc4acc, id_t* vc4hit,
		  id_t* vc5acc, id_t* vc5hit, id_t* vc6acc, id_t* vc6hit, id_t* vc7acc, id_t* vc7hit, id_t* vc8acc, id_t* vc8hit,
		  id_t* fnum, id_t* workload_counter1, id_t* workload_counter2, id_t* workload_counter3, id_t* workload_counter4){

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
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=workload_counter1
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=workload_counter2
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=workload_counter3
#pragma HLS INTERFACE mode=s_axilite bundle=ctr port=workload_counter4

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
#pragma HLS INTERFACE m_axi depth=13515 port=C_rcsr offset=slave bundle=crcsr
#pragma HLS INTERFACE m_axi depth=2957530 port=C_cid offset=slave bundle=ccid
#pragma HLS INTERFACE m_axi depth=2957530 port=C_val offset=slave bundle=cval
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr1 offset=slave bundle=brcsr1
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr2 offset=slave bundle=brcsr2
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr3 offset=slave bundle=brcsr3
#pragma HLS INTERFACE m_axi depth=13515 port=B_rcsr4 offset=slave bundle=brcsr4
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port1 offset=slave bundle=bv1
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port1 offset=slave bundle=bc1
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port2 offset=slave bundle=bv2
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port2 offset=slave bundle=bc2
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port3 offset=slave bundle=bv3
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port3 offset=slave bundle=bc3
#pragma HLS INTERFACE m_axi depth=88191 port=B_val_port4 offset=slave bundle=bv4
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid_port4 offset=slave bundle=bc4

#pragma HLS INTERFACE m_axi depth=88191 port=B_val1 offset=slave bundle=bval1
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid1 offset=slave bundle=bcid1
#pragma HLS INTERFACE m_axi depth=88191 port=B_val2 offset=slave bundle=bval2
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid2 offset=slave bundle=bcid2
#pragma HLS INTERFACE m_axi depth=88191 port=B_val3 offset=slave bundle=bval3
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid3 offset=slave bundle=bcid3
#pragma HLS INTERFACE m_axi depth=88191 port=B_val4 offset=slave bundle=bval4
#pragma HLS INTERFACE m_axi depth=88191 port=B_cid4 offset=slave bundle=bcid4

#pragma HLS INTERFACE s_axilite port=arnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=atnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=brnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=btnum bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_rcsr_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_val_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=B_cid_base bundle=ctr
#pragma HLS INTERFACE s_axilite port=return bundle=ctr

#pragma HLS DATAFLOW disable_start_propagation

	// reading A rows
	rinfo_stream _A_reader_rs;

	// reading A val and cids
	avc_stream _A_element_as;

	// indicate the PE is idle
	bool_stream pe1_idle, pe2_idle, pe3_idle, pe4_idle;

	bool_stream rm1_idle, rm2_idle, rm3_idle, rm4_idle;
	bool_stream vcm1_idle, vcm2_idle, vcm3_idle, vcm4_idle;

	mi_stream mi1, mi2, mi3, mi4,
	          mi1_1, mi2_1, mi3_1, mi4_1,
			  mi1_2, mi2_2, mi3_2, mi4_2,
			  mi1_3, mi2_3, mi3_3, mi4_3,
			  mi1_4, mi2_4, mi3_4, mi4_4;

	mi_stream vcmi1, vcmi2, vcmi3, vcmi4,
	          vcmi1_1, vcmi2_1, vcmi3_1, vcmi4_1,
			  vcmi1_2, vcmi2_2, vcmi3_2, vcmi4_2,
			  vcmi1_3, vcmi2_3, vcmi3_3, vcmi4_3,
			  vcmi1_4, vcmi2_4, vcmi3_4, vcmi4_4;

	// the task stream
	pe_stream pe1_job, pe2_job, pe3_job, pe4_job;

	// request to rcache manager
	rcache_stream rcache_stream1, rcache_stream2, rcache_stream3, rcache_stream4;

	// rcache manager to rcache
	rcache_stream rrequest1, rrequest2, rrequest3, rrequest4, rrequestf;
#pragma HLS STREAM variable=rrequest1 depth=4
#pragma HLS STREAM variable=rrequest2 depth=4
#pragma HLS STREAM variable=rrequest3 depth=4
#pragma HLS STREAM variable=rrequest4 depth=4
#pragma HLS STREAM variable=rrequestf depth=4

	// rcache responses, results to rcache manager
	rresponse_stream rresponse1, rresponse2, rresponse3, rresponse4;
#pragma HLS STREAM variable=rresponse1 depth=16
#pragma HLS STREAM variable=rresponse2 depth=16
#pragma HLS STREAM variable=rresponse3 depth=16
#pragma HLS STREAM variable=rresponse4 depth=16

	rresponse_stream rresult1, rresult2, rresult3, rresult4;

	// rcache manager final results
	rcache_stream rblock1, rblock2, rblock3, rblock4;

	// rcache banks
	rcache_stream rcache_re_bank1, rcache_re_bank2, rcache_re_bank3, rcache_re_bank4;
#pragma HLS STREAM variable=rcache_re_bank1 depth=8
#pragma HLS STREAM variable=rcache_re_bank2 depth=8
#pragma HLS STREAM variable=rcache_re_bank3 depth=8
#pragma HLS STREAM variable=rcache_re_bank4 depth=8

	rresponse_stream rcache_re_bank1_1, rcache_re_bank1_2, rcache_re_bank1_3, rcache_re_bank1_4,
				rcache_re_bank2_1, rcache_re_bank2_2, rcache_re_bank2_3, rcache_re_bank2_4,
				rcache_re_bank3_1, rcache_re_bank3_2, rcache_re_bank3_3, rcache_re_bank3_4,
				rcache_re_bank4_1, rcache_re_bank4_2, rcache_re_bank4_3, rcache_re_bank4_4;

	// rcache bank responses
	rresponse_stream  rcache_bank1_res, rcache_bank2_res, rcache_bank3_res, rcache_bank4_res;
#pragma HLS STREAM variable=rcache_bank1_res depth=8
#pragma HLS STREAM variable=rcache_bank2_res depth=8
#pragma HLS STREAM variable=rcache_bank3_res depth=8
#pragma HLS STREAM variable=rcache_bank4_res depth=8

	// final response
	rresponse_stream rcache_res_final;
#pragma HLS STREAM variable=rcache_res_final depth=8

	// rcache results of banks and merge to one stream
	B_request_stream rcache_result_bank1_b, rcache_result_bank2_b, rcache_result_bank3_b, rcache_result_bank4_b,
	 	 	 	 	 rcache_result_bank1_d, rcache_result_bank2_d, rcache_result_bank3_d, rcache_result_bank4_d;

	// B row length to control the merger
	id_stream B_rlen_pe1, B_rlen_pe2, B_rlen_pe3, B_rlen_pe4;

	// request to vccache and its banks
	vcrequest_stream vrequest1, vrequest2, vrequest3, vrequest4, crequest1, crequest2, crequest3, crequest4, requests_f;
#pragma HLS STREAM variable=requests_f depth=8

	vcrequest_stream vccache_re_bank1, vccache_re_bank2, vccache_re_bank3, vccache_re_bank4;
#pragma HLS STREAM variable=vccache_re_bank1 depth=64
#pragma HLS STREAM variable=vccache_re_bank2 depth=64
#pragma HLS STREAM variable=vccache_re_bank3 depth=64
#pragma HLS STREAM variable=vccache_re_bank4 depth=64

	vcrequest_stream vccache_re_bank1_pe1, vccache_re_bank1_pe2, vccache_re_bank1_pe3, vccache_re_bank1_pe4,
	                 vccache_re_bank1_pe5, vccache_re_bank1_pe6, vccache_re_bank1_pe7, vccache_re_bank1_pe8,
					 vccache_re_bank2_pe1, vccache_re_bank2_pe2, vccache_re_bank2_pe3, vccache_re_bank2_pe4,
					 vccache_re_bank2_pe5, vccache_re_bank2_pe6, vccache_re_bank2_pe7, vccache_re_bank2_pe8;
#pragma HLS STREAM variable=vccache_re_bank1_pe1 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe2 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe3 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe4 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe5 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe6 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe7 depth=8
#pragma HLS STREAM variable=vccache_re_bank1_pe8 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe1 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe2 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe3 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe4 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe5 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe6 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe7 depth=8
#pragma HLS STREAM variable=vccache_re_bank2_pe8 depth=8
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe1 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe2 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe3 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe4 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe5 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe6 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe7 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank1_pe8 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe1 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe2 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe3 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe4 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe5 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe6 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe7 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vccache_re_bank2_pe8 type=fifo impl=srl

	// vcache banks' response and result
	vcresponse_stream vcache_bank1_vres, vcache_bank2_vres, vcache_bank3_vres, vcache_bank4_vres,
	                  vcache_bank5_vres, vcache_bank6_vres, vcache_bank7_vres, vcache_bank8_vres,
					  vcache_bank1_cres, vcache_bank2_cres, vcache_bank3_cres, vcache_bank4_cres,
					  vcache_bank5_cres, vcache_bank6_cres, vcache_bank7_cres, vcache_bank8_cres;
#pragma HLS STREAM variable=vcache_bank1_vres depth=16
#pragma HLS STREAM variable=vcache_bank2_vres depth=16
#pragma HLS STREAM variable=vcache_bank3_vres depth=16
#pragma HLS STREAM variable=vcache_bank4_vres depth=16
#pragma HLS STREAM variable=vcache_bank5_vres depth=16
#pragma HLS STREAM variable=vcache_bank6_vres depth=16
#pragma HLS STREAM variable=vcache_bank7_vres depth=16
#pragma HLS STREAM variable=vcache_bank8_vres depth=16
#pragma HLS STREAM variable=vcache_bank1_cres depth=16
#pragma HLS STREAM variable=vcache_bank2_cres depth=16
#pragma HLS STREAM variable=vcache_bank3_cres depth=16
#pragma HLS STREAM variable=vcache_bank4_cres depth=16
#pragma HLS STREAM variable=vcache_bank5_cres depth=16
#pragma HLS STREAM variable=vcache_bank6_cres depth=16
#pragma HLS STREAM variable=vcache_bank7_cres depth=16
#pragma HLS STREAM variable=vcache_bank8_cres depth=16

	vcresponse_stream vcache_res_vfinal, vcache_res_cfinal;

	vcresult_stream vcache_bank1_result, vcache_bank2_result, vcache_bank3_result, vcache_bank4_result,
					vccache_banks_final;
#pragma HLS STREAM variable=vcache_bank1_result depth=8
#pragma HLS STREAM variable=vcache_bank2_result depth=8
#pragma HLS STREAM variable=vcache_bank3_result depth=8
#pragma HLS STREAM variable=vcache_bank4_result depth=8

	vcresult_stream vcache_bank1_vresult1, vcache_bank1_cresult1;
#pragma HLS STREAM variable=vcache_bank1_vresult1 depth=8
#pragma HLS STREAM variable=vcache_bank1_cresult1 depth=8
#pragma HLS BIND_STORAGE variable=vcache_bank1_vresult1 type=fifo impl=srl
#pragma HLS BIND_STORAGE variable=vcache_bank1_cresult1 type=fifo impl=srl

	// vresponse and vresult to manager
	vcresponse_stream vresponse1, vresponse2, vresponse3, vresponse4, cresponse1, cresponse2, cresponse3, cresponse4;
	cacheline_stream vresult1, vresult2, vresult3, vresult4, cresult1, cresult2, cresult3, cresult4;

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

	block_stream vblock1d, vblock2d, vblock3d, vblock4d, cblock1d, cblock2d, cblock3d, cblock4d;
#pragma HLS STREAM variable=vblock1d depth=100
#pragma HLS STREAM variable=vblock2d depth=100
#pragma HLS STREAM variable=vblock3d depth=100
#pragma HLS STREAM variable=vblock4d depth=100
#pragma HLS STREAM variable=cblock1d depth=100
#pragma HLS STREAM variable=cblock2d depth=100
#pragma HLS STREAM variable=cblock3d depth=100
#pragma HLS STREAM variable=cblock4d depth=100

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
#pragma HLS STREAM variable=result_pe1 depth=1000
#pragma HLS STREAM variable=result_pe2 depth=1000
#pragma HLS STREAM variable=result_pe3 depth=1000
#pragma HLS STREAM variable=result_pe4 depth=1000

	merge_stream final_merger_1, final_merger_2, final_merger_3;
	id_stream final_len_1, final_len_2;
#pragma HLS STREAM variable=final_merger_1 depth=1000
#pragma HLS STREAM variable=final_merger_2 depth=1000
#pragma HLS STREAM variable=final_merger_3 depth=1000
#pragma HLS STREAM variable=final_len_1 depth=16
#pragma HLS STREAM variable=final_len_2 depth=16

	// read A's row infos: rid, rlen, ridx
	A_row_reader(A_rcsr, arnum, &_A_reader_rs);

	// read A's element: A_cid, A_val, A_rid, i.e., the task
	A_element_reader(A_val, A_cid, &_A_reader_rs, atnum, &_A_element_as);

	// assign tasks to PEs and access to the rcache to get B_ridx and B_rlen
	A_element_distributor(&_A_element_as, atnum, &pe1_idle, &pe2_idle, &pe3_idle, &pe4_idle,
			&rm1_idle, &rm2_idle, &rm3_idle, &rm4_idle,
			&pe1_job, &pe2_job, &pe3_job, &pe4_job, &rcache_stream1, &rcache_stream2, &rcache_stream3, &rcache_stream4);

//	read_Bridx(&rcache_stream1, &rblock1, B_rcsr1);
//	read_Bridx(&rcache_stream2, &rblock2, B_rcsr2);
//	read_Bridx(&rcache_stream3, &rblock3, B_rcsr3);
//	read_Bridx(&rcache_stream4, &rblock4, B_rcsr4);

	rcrequest_manager(&rcache_stream1, &rrequest1, &rresponse1, &rblock1, B_rcsr1);
	rcrequest_manager(&rcache_stream2, &rrequest2, &rresponse2, &rblock2, B_rcsr2);
	rcrequest_manager(&rcache_stream3, &rrequest3, &rresponse3, &rblock3, B_rcsr3);
	rcrequest_manager(&rcache_stream4, &rrequest4, &rresponse4, &rblock4, B_rcsr4);

	merge_4_to_1_rrequest(&rrequest1, &rrequest2, &rrequest3, &rrequest4, &rrequestf);
	request_to_rbanks(&rrequestf, B_rcsr_base, &rcache_re_bank1, &rcache_re_bank2, &rcache_re_bank3, &rcache_re_bank4);

	rcache(&rcache_re_bank1, B_rcsr_base, &rcache_bank1_res, r1acc, r1hit, &mi1);
	rcache(&rcache_re_bank2, B_rcsr_base, &rcache_bank2_res, r2acc, r2hit, &mi2);
	rcache(&rcache_re_bank3, B_rcsr_base, &rcache_bank3_res, r3acc, r3hit, &mi3);
	rcache(&rcache_re_bank4, B_rcsr_base, &rcache_bank4_res, r4acc, r4hit, &mi4);

	distributer_mi(&mi1, &mi1_1, &mi1_2, &mi1_3, &mi1_4);
	distributer_mi(&mi2, &mi2_1, &mi2_2, &mi2_3, &mi2_4);
	distributer_mi(&mi3, &mi3_1, &mi3_2, &mi3_3, &mi3_4);
	distributer_mi(&mi4, &mi4_1, &mi4_2, &mi4_3, &mi4_4);

	merge_4_to_1_mi(&mi1_1, &mi2_1, &mi3_1, &mi4_1, &rm1_idle);
	merge_4_to_1_mi(&mi1_2, &mi2_2, &mi3_2, &mi4_2, &rm2_idle);
	merge_4_to_1_mi(&mi1_3, &mi2_3, &mi3_3, &mi4_3, &rm3_idle);
	merge_4_to_1_mi(&mi1_4, &mi2_4, &mi3_4, &mi4_4, &rm4_idle);

	distributer_rresponse(&rcache_bank1_res, &rcache_re_bank1_1, &rcache_re_bank1_2, &rcache_re_bank1_3, &rcache_re_bank1_4);
	distributer_rresponse(&rcache_bank2_res, &rcache_re_bank2_1, &rcache_re_bank2_2, &rcache_re_bank2_3, &rcache_re_bank2_4);
	distributer_rresponse(&rcache_bank3_res, &rcache_re_bank3_1, &rcache_re_bank3_2, &rcache_re_bank3_3, &rcache_re_bank3_4);
	distributer_rresponse(&rcache_bank4_res, &rcache_re_bank4_1, &rcache_re_bank4_2, &rcache_re_bank4_3, &rcache_re_bank4_4);

	merge_4_to_1_rresponse(&rcache_re_bank1_1, &rcache_re_bank2_1, &rcache_re_bank3_1, &rcache_re_bank4_1, &rresponse1);
	merge_4_to_1_rresponse(&rcache_re_bank1_2, &rcache_re_bank2_2, &rcache_re_bank3_2, &rcache_re_bank4_2, &rresponse2);
	merge_4_to_1_rresponse(&rcache_re_bank1_3, &rcache_re_bank2_3, &rcache_re_bank3_3, &rcache_re_bank4_3, &rresponse3);
	merge_4_to_1_rresponse(&rcache_re_bank1_4, &rcache_re_bank2_4, &rcache_re_bank3_4, &rcache_re_bank4_4, &rresponse4);

	from_rcache_result_to_B_vcrequest(&rblock1, &rcache_result_bank1_b, &rcache_result_bank1_d, &B_rlen_pe1);
	from_rcache_result_to_B_vcrequest(&rblock2, &rcache_result_bank2_b, &rcache_result_bank2_d, &B_rlen_pe2);
	from_rcache_result_to_B_vcrequest(&rblock3, &rcache_result_bank3_b, &rcache_result_bank3_d, &B_rlen_pe3);
	from_rcache_result_to_B_vcrequest(&rblock4, &rcache_result_bank4_b, &rcache_result_bank4_d, &B_rlen_pe4);

	generate_data(&rcache_result_bank1_d, B_val1, B_cid1, &vblock1d, &cblock1d);
	generate_data(&rcache_result_bank2_d, B_val2, B_cid2, &vblock2d, &cblock2d);
	generate_data(&rcache_result_bank3_d, B_val3, B_cid3, &vblock3d, &cblock3d);
	generate_data(&rcache_result_bank4_d, B_val4, B_cid4, &vblock4d, &cblock4d);

	vcrequest_manager(&rcache_result_bank1_b, &vrequest1, &crequest1, &vresponse1, &cresponse1, &vresult1, &cresult1, &vblock1, &cblock1, 0, B_val_port1, B_cid_port1);
	vcrequest_manager(&rcache_result_bank2_b, &vrequest2, &crequest2, &vresponse2, &cresponse2, &vresult2, &cresult2, &vblock2, &cblock2, 1, B_val_port2, B_cid_port2);
	vcrequest_manager(&rcache_result_bank3_b, &vrequest3, &crequest3, &vresponse3, &cresponse3, &vresult3, &cresult3, &vblock3, &cblock3, 2, B_val_port3, B_cid_port3);
	vcrequest_manager(&rcache_result_bank4_b, &vrequest4, &crequest4, &vresponse4, &cresponse4, &vresult4, &cresult4, &vblock4, &cblock4, 3, B_val_port4, B_cid_port4);

	request_to_vcbanks(&vrequest1, B_val_base, B_cid_base, &vccache_re_bank1_pe1, &vccache_re_bank2_pe1);
	request_to_vcbanks(&vrequest2, B_val_base, B_cid_base, &vccache_re_bank1_pe2, &vccache_re_bank2_pe2);
	request_to_vcbanks(&vrequest3, B_val_base, B_cid_base, &vccache_re_bank1_pe3, &vccache_re_bank2_pe3);
	request_to_vcbanks(&vrequest4, B_val_base, B_cid_base, &vccache_re_bank1_pe4, &vccache_re_bank2_pe4);
	request_to_vcbanks(&crequest1, B_val_base, B_cid_base, &vccache_re_bank1_pe5, &vccache_re_bank2_pe5);
	request_to_vcbanks(&crequest2, B_val_base, B_cid_base, &vccache_re_bank1_pe6, &vccache_re_bank2_pe6);
	request_to_vcbanks(&crequest3, B_val_base, B_cid_base, &vccache_re_bank1_pe7, &vccache_re_bank2_pe7);
	request_to_vcbanks(&crequest4, B_val_base, B_cid_base, &vccache_re_bank1_pe8, &vccache_re_bank2_pe8);

	merge_4_to_1_vcrequest(&vccache_re_bank1_pe1, &vccache_re_bank1_pe2, &vccache_re_bank1_pe3, &vccache_re_bank1_pe4, &vccache_re_bank1);
	merge_4_to_1_vcrequest(&vccache_re_bank2_pe1, &vccache_re_bank2_pe2, &vccache_re_bank2_pe3, &vccache_re_bank2_pe4, &vccache_re_bank2);
	merge_4_to_1_vcrequest(&vccache_re_bank1_pe5, &vccache_re_bank1_pe6, &vccache_re_bank1_pe7, &vccache_re_bank1_pe8, &vccache_re_bank3);
	merge_4_to_1_vcrequest(&vccache_re_bank2_pe5, &vccache_re_bank2_pe6, &vccache_re_bank2_pe7, &vccache_re_bank2_pe8, &vccache_re_bank4);

	vcache(&vccache_re_bank1, B_val_base, &vcache_bank1_vres, &vcache_bank1_result, vc1acc, vc1hit);
	vcache(&vccache_re_bank2, B_val_base, &vcache_bank2_vres, &vcache_bank2_result, vc2acc, vc2hit);
	ccache(&vccache_re_bank3, B_cid_base, &vcache_bank3_cres, &vcache_bank3_result, vc3acc, vc3hit);
	ccache(&vccache_re_bank4, B_cid_base, &vcache_bank4_cres, &vcache_bank4_result, vc4acc, vc4hit);

	merge_2_to_1_vcresponse(&vcache_bank1_vres, &vcache_bank2_vres, &vcache_res_vfinal);
	merge_2_to_1_vcresponse(&vcache_bank3_cres, &vcache_bank4_cres, &vcache_res_cfinal);

	distributer_vcresponse(&vcache_res_vfinal, &vresponse1, &vresponse2, &vresponse3, &vresponse4);
	distributer_vcresponse(&vcache_res_cfinal, &cresponse1, &cresponse2, &cresponse3, &cresponse4);

	merge_2_to_1_vcresult(&vcache_bank1_result, &vcache_bank2_result, &vcache_bank1_vresult1);
	merge_2_to_1_vcresult(&vcache_bank3_result, &vcache_bank4_result, &vcache_bank1_cresult1);

	distributer_vcresult(&vcache_bank1_vresult1, &vresult1, &vresult2, &vresult3, &vresult4);
	distributer_vcresult(&vcache_bank1_cresult1, &cresult1, &cresult2, &cresult3, &cresult4);

	pe(&cblock1, &vblock1, &cblock1d, &vblock1d, &pe1_job, &p_pe1, &B_rlen_pe1, &mc_pe1, &pe1_idle, &efinish1, workload_counter1);
	pe(&cblock2, &vblock2, &cblock2d, &vblock2d, &pe2_job, &p_pe2, &B_rlen_pe2, &mc_pe2, &pe2_idle, &efinish2, workload_counter2);
	pe(&cblock3, &vblock3, &cblock3d, &vblock3d, &pe3_job, &p_pe3, &B_rlen_pe3, &mc_pe3, &pe3_idle, &efinish3, workload_counter3);
	pe(&cblock4, &vblock4, &cblock4d, &vblock4d, &pe4_job, &p_pe4, &B_rlen_pe4, &mc_pe4, &pe4_idle, &efinish4, workload_counter4);

	convert_pblock_to_mergeitem(&p_pe1, &m_pe1);
	convert_pblock_to_mergeitem(&p_pe2, &m_pe2);
	convert_pblock_to_mergeitem(&p_pe3, &m_pe3);
	convert_pblock_to_mergeitem(&p_pe4, &m_pe4);

	merger(&mc_pe1, &m_pe1, &result_pe1, &efinish1);
	merger(&mc_pe2, &m_pe2, &result_pe2, &efinish2);
	merger(&mc_pe3, &m_pe3, &result_pe3, &efinish3);
	merger(&mc_pe4, &m_pe4, &result_pe4, &efinish4);

	sub_final_merger(&result_pe1, &result_pe2, &final_merger_1);
	sub_final_merger(&result_pe3, &result_pe4, &final_merger_2);

	final_merger(&final_merger_1, &final_merger_2, &final_merger_3, &final_len_1, &final_len_2);

	write_C_rid_rlen(&final_len_1, C_rcsr, arnum);
	write_C_val_cid(&final_merger_3, C_val, C_cid, fnum, &final_len_2);
}


