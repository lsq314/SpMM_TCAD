/*
 * Empty C++ Application
 */
#include <stdio.h>
#include <xspmm.h>
#include <xtmrctr.h>
#include <xil_cache.h>
#include <math.h>
#include <ff.h>
#include <xil_exception.h>
#include <xscugic.h>

typedef float val_t;
typedef uint32_t id_t;

val_t eof = 1e-5;

static FATFS fatfs;
int rc;

#define val "val.BIN"
#define rid "csr.BIN"
#define cid "cid.BIN"

XSpmm x;
XSpmm_Config* xc;

XTmrCtr t;
XTmrCtr_Config* tc;

XScuGic s;
XScuGic_Config* sc;

int i, j, k;
u32 start_time, end_time;
bool isfinished=false;

u32 start_timer(XTmrCtr &t, uint8_t counter){
	XTmrCtr_Reset(&t, counter);
	u32 result = XTmrCtr_GetValue(&t, counter);
	XTmrCtr_Start(&t, counter);
	return result;
}

u32 stop_timer(XTmrCtr &t, uint8_t counter){
	u32 result = XTmrCtr_GetValue(&t, counter);
	XTmrCtr_Stop(&t, counter);
	return result;
}

void Handler(void *ptr){

	stop_timer(t, 0);
	end_time = Xil_In32((0xa0010000) + XTmrCtr_Offsets[(0)] + (XTC_TCR_OFFSET));
	printf("Totally run %u cycles\n", end_time - start_time);

	XSpmm *tx = (XSpmm *)ptr;
	XSpmm_InterruptClear(tx, 1);
//	XSpmm_InterruptDisable(tx, 0);
//	XSpmm_InterruptGlobalDisable(tx);

	isfinished = true;
	return;
}

int SD_Init(){
    FRESULT rc;
    rc = f_mount(&fatfs,"",0);
    if(rc){
        xil_printf("ERROR: f_mount returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int SD_Transfer_read(char *FileName,u64 DestinationAddress,u32 ByteLength){
    FIL fil;
    FRESULT rc;
    UINT br;

    rc=f_open(&fil,FileName,FA_READ);
    if(rc){
        xil_printf("ERROR:f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_lseek(&fil,0);
    if(rc){
        xil_printf("ERROR:f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_read(&fil,(void*)DestinationAddress,ByteLength,&br);
    if(rc){
        xil_printf("ERROR:f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    rc = f_close(&fil);
    if(rc){
        xil_printf("ERROR:f_open returned %d\r\n",rc);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

#define rlen 23561
#define vlen 484256

id_t r1acc, r1hit, r2acc, r2hit, r3acc, r3hit, r4acc, r4hit, r5acc, r5hit, r6acc, r6hit, r7acc, r7hit, r8acc, r8hit;

id_t vc1acc, vc1hit, vc2acc, vc2hit, vc3acc, vc3hit, vc4acc, vc4hit, vc5acc, vc5hit, vc6acc, vc6hit, vc7acc, vc7hit, vc8acc, vc8hit;

id_t fnum;

__attribute ((aligned (16))) id_t Arcsr[rlen];
__attribute ((aligned (16))) val_t Aval[vlen];
__attribute ((aligned (16))) id_t Acid[vlen];

__attribute ((aligned (16))) id_t Brcsr[rlen];

__attribute ((aligned (16))) id_t Crcsr[rlen];
__attribute ((aligned (16))) val_t Cval[352764];
__attribute ((aligned (16))) id_t Ccid[352764];

int main(){
    rc = SD_Init();

    rc = SD_Transfer_read(val,(u64)Aval,vlen*4);
    rc = SD_Transfer_read(cid,(u64)Acid,vlen*4);
    rc = SD_Transfer_read(rid,(u64)Arcsr,rlen*4);

	memcpy(Brcsr, Arcsr, rlen*4);

	tc = XTmrCtr_LookupConfig(XPAR_AXI_TIMER_0_DEVICE_ID);
	XTmrCtr_CfgInitialize(&t, tc, tc->BaseAddress);

	Xil_DCacheFlush();

	xc = XSpmm_LookupConfig(XPAR_SPMM_0_DEVICE_ID);
	XSpmm_CfgInitialize(&x, xc);

	id_t index = 0;
	id_t final_idx = rlen-1;

	XSpmm_Set_arnum(&x, final_idx-index);
	XSpmm_Set_atnum(&x, Arcsr[final_idx]-Arcsr[index]);

	XSpmm_Set_brnum(&x, rlen-1);
	XSpmm_Set_btnum(&x, vlen);

	XSpmm_Set_B_rcsr_base(&x, (u64)Brcsr);
	XSpmm_Set_B_val_base(&x, (u64)Aval);
	XSpmm_Set_B_cid_base(&x, (u64)Acid);

	XSpmm_Set_A_cid(&x, (u64)&Acid[Arcsr[index]]);
	XSpmm_Set_A_rcsr(&x, (u64)&Arcsr[index]);
	XSpmm_Set_A_val(&x, (u64)&Aval[Arcsr[index]]);

	XSpmm_Set_C_cid(&x, (u64)Ccid);
	XSpmm_Set_C_rcsr(&x, (u64)Crcsr);
	XSpmm_Set_C_val(&x, (u64)Cval);

	XSpmm_Set_B_rcsr1(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr2(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr3(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr4(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr5(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr6(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr7(&x, (u64)Brcsr);
	XSpmm_Set_B_rcsr8(&x, (u64)Brcsr);

	XSpmm_Set_B_cid1(&x, (u64)Acid);
	XSpmm_Set_B_val1(&x, (u64)Aval);
	XSpmm_Set_B_cid2(&x, (u64)Acid);
	XSpmm_Set_B_val2(&x, (u64)Aval);
	XSpmm_Set_B_cid3(&x, (u64)Acid);
	XSpmm_Set_B_val3(&x, (u64)Aval);
	XSpmm_Set_B_cid4(&x, (u64)Acid);
	XSpmm_Set_B_val4(&x, (u64)Aval);
	XSpmm_Set_B_cid5(&x, (u64)Acid);
	XSpmm_Set_B_val5(&x, (u64)Aval);
	XSpmm_Set_B_cid6(&x, (u64)Acid);
	XSpmm_Set_B_val6(&x, (u64)Aval);
	XSpmm_Set_B_cid7(&x, (u64)Acid);
	XSpmm_Set_B_val7(&x, (u64)Aval);
	XSpmm_Set_B_cid8(&x, (u64)Acid);
	XSpmm_Set_B_val8(&x, (u64)Aval);

	XSpmm_Set_B_cid_port1(&x, (u64)Acid);
	XSpmm_Set_B_val_port1(&x, (u64)Aval);
	XSpmm_Set_B_cid_port2(&x, (u64)Acid);
	XSpmm_Set_B_val_port2(&x, (u64)Aval);
	XSpmm_Set_B_cid_port3(&x, (u64)Acid);
	XSpmm_Set_B_val_port3(&x, (u64)Aval);
	XSpmm_Set_B_cid_port4(&x, (u64)Acid);
	XSpmm_Set_B_val_port4(&x, (u64)Aval);

	Xil_DCacheFlush();
	printf("Totally from %u, %u\n", index, final_idx);
	printf("Begin\n");

	start_time = start_timer(t, 0);
	XSpmm_Start(&x);
	while(!XSpmm_IsDone(&x));
	end_time = stop_timer(t, 0);

	r1acc = XSpmm_Get_r1acc(&x);
	r2acc = XSpmm_Get_r2acc(&x);
	r3acc = XSpmm_Get_r3acc(&x);
	r4acc = XSpmm_Get_r4acc(&x);
	r5acc = XSpmm_Get_r5acc(&x);
	r6acc = XSpmm_Get_r6acc(&x);
	r7acc = XSpmm_Get_r7acc(&x);
	r8acc = XSpmm_Get_r8acc(&x);

	r1hit = XSpmm_Get_r1hit(&x);
	r2hit = XSpmm_Get_r2hit(&x);
	r3hit = XSpmm_Get_r3hit(&x);
	r4hit = XSpmm_Get_r4hit(&x);
	r5hit = XSpmm_Get_r5hit(&x);
	r6hit = XSpmm_Get_r6hit(&x);
	r7hit = XSpmm_Get_r7hit(&x);
	r8hit = XSpmm_Get_r8hit(&x);

	vc1hit = XSpmm_Get_vc1hit(&x);
	vc2hit = XSpmm_Get_vc2hit(&x);
	vc3hit = XSpmm_Get_vc3hit(&x);
	vc4hit = XSpmm_Get_vc4hit(&x);
	vc5hit = XSpmm_Get_vc5hit(&x);
	vc6hit = XSpmm_Get_vc6hit(&x);
	vc7hit = XSpmm_Get_vc7hit(&x);
	vc8hit = XSpmm_Get_vc8hit(&x);

	vc1acc = XSpmm_Get_vc1acc(&x);
	vc2acc = XSpmm_Get_vc2acc(&x);
	vc3acc = XSpmm_Get_vc3acc(&x);
	vc4acc = XSpmm_Get_vc4acc(&x);
	vc5acc = XSpmm_Get_vc5acc(&x);
	vc6acc = XSpmm_Get_vc6acc(&x);
	vc7acc = XSpmm_Get_vc7acc(&x);
	vc8acc = XSpmm_Get_vc8acc(&x);

	fnum = XSpmm_Get_fnum(&x);

	printf("Totally spends %u cycles\n", end_time - start_time);
	printf("Rcache bank 1 totally %u accesses, hit %u accesses\n", r1acc, r1hit);
	printf("Rcache bank 2 totally %u accesses, hit %u accesses\n", r2acc, r2hit);
	printf("Rcache bank 3 totally %u accesses, hit %u accesses\n", r3acc, r3hit);
	printf("Rcache bank 4 totally %u accesses, hit %u accesses\n", r4acc, r4hit);
	printf("Rcache bank 5 totally %u accesses, hit %u accesses\n", r5acc, r5hit);
	printf("Rcache bank 6 totally %u accesses, hit %u accesses\n", r6acc, r6hit);
	printf("Rcache bank 7 totally %u accesses, hit %u accesses\n", r7acc, r7hit);
	printf("Rcache bank 8 totally %u accesses, hit %u accesses\n", r8acc, r8hit);

	printf("VCcache bank 1 totally %u accesses, hit %u accesses\n", vc1acc, vc1hit);
	printf("VCcache bank 2 totally %u accesses, hit %u accesses\n", vc2acc, vc2hit);
	printf("VCcache bank 3 totally %u accesses, hit %u accesses\n", vc3acc, vc3hit);
	printf("VCcache bank 4 totally %u accesses, hit %u accesses\n", vc4acc, vc4hit);
	printf("VCcache bank 5 totally %u accesses, hit %u accesses\n", vc5acc, vc5hit);
	printf("VCcache bank 6 totally %u accesses, hit %u accesses\n", vc6acc, vc6hit);
	printf("VCcache bank 7 totally %u accesses, hit %u accesses\n", vc7acc, vc7hit);
	printf("VCcache bank 8 totally %u accesses, hit %u accesses\n", vc8acc, vc8hit);
	printf("Totally %u numbers\n", fnum);

	return 0;
}
