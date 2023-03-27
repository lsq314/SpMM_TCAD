#ifndef CONFIGCCCC
#define CONFIGCCCC

#include "/home/lsq/software/Vitis_2022.1/Vitis_HLS/2022.1/include/gmp.h"
#include <stdint.h>
#include <hls_stream.h>
#include <ap_fixed.h>
#include <string.h>

typedef float val_t;
typedef uint32_t id_t;

#define CACHE_FIRST_HIT 1
#define CACHE_FIRST_MISS 2
#define CACHE_IDLE 3
#define CACHE_READ 1
#define CACHE_WRITE 2
#define CACHE_FIRST_READ 0

#define RCACHE_CACHELINE_BYTES 16
#define RCACHE_SET_NUM 128
#define RCACHE_BANK_NUM 32
#define RCACHE_TAG_OFFSET 11

#define VCCACHE_CACHELINE_BYTES 16
#define VCCACHE_SET_NUM 128
#define VCCACHE_BANK_NUM 64
#define VCCACHE_TAG_OFFSET 11

#define B_VAL 0
#define B_CID 1

#define TERMINATE 0xFFFFFFFF

struct rcsr {
	id_t ridx;
	id_t rlen;
};

struct rinfo {
	id_t rid;
	id_t rlen;
};

struct Avc {
	val_t val;
	id_t cid;
	id_t rid;
};

struct __attribute__ ((packed)) pe_job {
	val_t val;
	id_t rid;
	bool partial_end;
	bool end;
};

struct __attribute__ ((packed)) rcache_request {
	ap_uint<3> dest;
	bool end;
	ap_uint<2> type;
	id_t Acid;
	id_t Acid1;
	ap_uint<160> data;
};

struct __attribute__ ((packed)) rcache_response {
	bool end;
	bool no_use;
	ap_uint<3> dest;
	ap_uint<3> result;
	ap_uint<160> data;
};

struct __attribute__ ((packed)) manager_idle {
	bool end;
	ap_uint<3> dest;
	bool idle;
};

struct __attribute__ ((packed)) B_row_request {
	bool end;
	ap_uint<3> peID;
	id_t bridx;
	id_t brlen;
};

struct __attribute__ ((packed)) lru_struct {
	id_t way_n;
	id_t counter;
};

struct __attribute__ ((packed)) vccache_request {
	bool end;
	ap_uint<2> target;
	ap_uint<3> type;
	ap_uint<3> peID;
	ap_uint<5> dest;
	ap_uint<3> write_idx;
	id_t brid;
	id_t brlen;
	ap_uint<128> data;
};

struct __attribute__ ((packed)) vccache_result {
	bool end;
	ap_uint<3> type;
	ap_uint<3> peID;
	ap_uint<1024> data;
};


struct __attribute__ ((packed)) vccache_response {
	ap_uint<2> result;
	bool end;
	ap_uint<3> peID;
};

struct __attribute__ ((packed)) internal_control {
	bool end;
	ap_uint<3> peID;
	id_t bridx;
	id_t brlen;
};

struct __attribute__ ((packed)) merge_item {
	val_t val;
	id_t cid;
	bool end;
	bool global_end;
};

struct __attribute__ ((packed)) merge_control {
	id_t rid;
	id_t rlen;
	bool end;
	bool partial_end;
};

struct __attribute__ ((packed)) merge_control_r {
	ap_uint<2> source;
	bool whether_push;
	id_t rlen;
	bool end;
};

struct __attribute__ ((packed)) vccache_block {
	ap_uint<3> peID;
	bool end;
	ap_uint<128> data;
};

struct __attribute__ ((packed)) block {
	ap_uint<3> begin;
	ap_uint<3> end;
	ap_uint<128> data;
};

struct __attribute__ ((packed)) partial_block {
	ap_uint<128> data;
	ap_uint<128> cid;
	ap_uint<3> begin;
	ap_uint<3> end;
	bool finish;
};

struct __attribute__ ((packed)) off_chip_access {
	bool end;
	ap_uint<3> type;
	id_t begin;
	id_t last;
	ap_uint<5> sourceID;
};

typedef hls::stream<id_t> id_stream;
typedef hls::stream<val_t> val_stream;
typedef hls::stream<rinfo> rinfo_stream;
typedef hls::stream<Avc> avc_stream;
typedef hls::stream<pe_job> pe_stream;
typedef hls::stream<rcache_request> rcache_stream;
typedef hls::stream<rcache_response> rresponse_stream;

typedef hls::stream<B_row_request> B_request_stream;
typedef hls::stream<vccache_request> vcrequest_stream;
typedef hls::stream<vccache_response> vcresponse_stream;
typedef hls::stream<vccache_result> vcresult_stream;


typedef hls::stream<internal_control> internal_control_stream;

typedef hls::stream<vccache_block> vcblock_stream;
typedef hls::stream<block> block_stream;

typedef hls::stream<partial_block> partial_stream;

typedef hls::stream<merge_item> merge_stream;
typedef hls::stream<merge_item, 1000> merge_stream_1000;

typedef hls::stream<merge_control> mcontrol_stream;
typedef hls::stream<merge_control_r> mcontrolr_stream;

typedef hls::stream<ap_uint<128> > B_data_stream;
typedef hls::stream<ap_uint<160> > rcacheline_stream;
typedef hls::stream<ap_uint<1024> > cacheline_stream;

typedef hls::stream<off_chip_access> off_chip_stream;

typedef hls::stream<bool> bool_stream;
typedef hls::stream<manager_idle> mi_stream;

template<typename To, typename From>
inline To Reinterpret(const From& val){
    return reinterpret_cast<const To&>(val);
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
		  id_t* fnum, id_t* workload_counter1, id_t* workload_counter2, id_t* workload_counter3, id_t* workload_counter4);

#endif
