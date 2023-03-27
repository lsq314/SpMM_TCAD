#include "core.h"
#include <stdio.h>
#include "row_wise.h"
#include "pe.h"

FILE *fp;

#define rlen 13515
#define vlen 352762

__attribute ((aligned (16))) id_t Arcsr[13516];
__attribute ((aligned (16))) val_t Aval[352764];
__attribute ((aligned (16))) id_t Acid[352764];

__attribute ((aligned (16))) id_t Brcsr[13516];

__attribute ((aligned (16))) id_t Crcsr[13516];
__attribute ((aligned (16))) val_t Cval[2957530];
__attribute ((aligned (16))) id_t Ccid[2957530];

id_t r1acc, r1hit, r2acc, r2hit, r3acc, r3hit, r4acc, r4hit, r5acc, r5hit, r6acc, r6hit, r7acc, r7hit, r8acc, r8hit;

id_t vc1acc, vc1hit, vc2acc, vc2hit, vc3acc, vc3hit, vc4acc, vc4hit, vc5acc, vc5hit, vc6acc, vc6hit, vc7acc, vc7hit, vc8acc, vc8hit;

id_t fnum;

id_t workload_counter1, workload_counter2, workload_counter3, workload_counter4;

__attribute__ ((aligned(16))) uint32_t source1[100], source2[100], source3[100], source4[100];

int main(){

	fp = fopen("/home/lsq/projects/pycharm_projects/spmm/bin/poisson3Da/csr.BIN", "rb");
	fread(Arcsr, 4, rlen, fp);
	fclose(fp);

	fp = fopen("/home/lsq/projects/pycharm_projects/spmm/bin/poisson3Da/val.BIN", "rb");
	fread(Aval, 4, vlen, fp);
	fclose(fp);

	fp = fopen("/home/lsq/projects/pycharm_projects/spmm/bin/poisson3Da/cid.BIN", "rb");
	fread(Acid, 4, vlen, fp);
	fclose(fp);

	memcpy(Brcsr, Arcsr, rlen*4);

//	merger_test(Aval, Cval);

	spmm(Arcsr, Acid, Aval, 10, Arcsr[10], (ap_uint<128> *)Brcsr, (ap_uint<128> *)Brcsr,
			(ap_uint<128> *)Brcsr, (ap_uint<128> *)Brcsr,
			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
			13514, vlen, (ap_uint<128> *)Aval,
			(ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
			Crcsr, Ccid, Cval, reinterpret_cast<uintptr_t>(Brcsr), reinterpret_cast<uintptr_t>(Aval), reinterpret_cast<uintptr_t>(Acid),
			&r1acc, &r1hit, &r2acc, &r2hit, &r3acc, &r3hit, &r4acc, &r4hit, &r5acc, &r5hit, &r6acc, &r6hit, &r7acc, &r7hit, &r8acc, &r8hit,
			&vc1acc, &vc1hit, &vc2acc, &vc2hit, &vc3acc, &vc3hit, &vc4acc, &vc4hit,
			&vc5acc, &vc5hit, &vc6acc, &vc6hit, &vc7acc, &vc7hit, &vc8acc, &vc8hit, &fnum, &workload_counter1,
			&workload_counter2, &workload_counter3, &workload_counter4);

//	spmm_row_wise(Arcsr, Acid, Aval, 10, 257, Brcsr, Brcsr,
//			Brcsr, Brcsr, (ap_uint<128> *)Brcsr, (ap_uint<128> *)Brcsr,
//			(ap_uint<128> *)Brcsr, (ap_uint<128> *)Brcsr,
//			Aval, Acid, Aval, Acid,
//			Aval, Acid, Aval, Acid,
//			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
//			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
//			13514, vlen, (ap_uint<128> *)Aval,
//			(ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
//			(ap_uint<128> *)Aval, (ap_uint<128> *)Acid, (ap_uint<128> *)Aval, (ap_uint<128> *)Acid,
//			Crcsr, Ccid, Cval, reinterpret_cast<uintptr_t>(Brcsr), reinterpret_cast<uintptr_t>(Aval), reinterpret_cast<uintptr_t>(Acid),
//			&r1acc, &r1hit, &r2acc, &r2hit, &r3acc, &r3hit, &r4acc, &r4hit, &r5acc, &r5hit, &r6acc, &r6hit, &r7acc, &r7hit, &r8acc, &r8hit,
//			&vc1acc, &vc1hit, &vc2acc, &vc2hit, &vc3acc, &vc3hit, &vc4acc, &vc4hit,
//			&vc5acc, &vc5hit, &vc6acc, &vc6hit, &vc7acc, &vc7hit, &vc8acc, &vc8hit, &fnum);

	return 0;
}
