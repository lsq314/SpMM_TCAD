#include "core.h"
#include <hls_burst_maxi.h>

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
		  id_t* fnum);
