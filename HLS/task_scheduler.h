#ifndef TASKS
#define TASKS

#include "core.h"

void A_row_reader(id_t* A_rcsr, id_t rnum, rinfo_stream* A_rlenstream){

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

void A_element_reader(val_t* A_val, id_t* A_cid, rinfo_stream* A_rlenstream, id_t tnum, avc_stream* Avcstream){

	rinfo temp_rinfo = A_rlenstream->read();
	Avc temp_avc;

    id_t counter = 0;
    id_t temp_counter = 0;

    // read the A_val and A_cid which belongs to a nonzero element, each element is a task processed by a PE
    // Avc: val, rid, cid, totally tnum elements
	for (id_t i=0; i<tnum; ) {
#pragma HLS PIPELINE II=1
		temp_avc.val = A_val[i];
		temp_avc.rid = temp_rinfo.rid;
		temp_avc.cid = A_cid[i];
		Avcstream->write(temp_avc);

        id_t next = i+1;
        temp_counter = counter+1;

        counter = (temp_counter < temp_rinfo.rlen)? temp_counter : 0;
        temp_rinfo = (temp_counter < temp_rinfo.rlen)? temp_rinfo : A_rlenstream->read();
        i = next;
	}
}

void A_element_distributor(avc_stream* Avcstream, id_t tnum,
		bool_stream* pe1_idle, bool_stream* pe2_idle, bool_stream* pe3_idle, bool_stream* pe4_idle,
		bool_stream* rm1_idle, bool_stream* rm2_idle, bool_stream* rm3_idle, bool_stream* rm4_idle,
		pe_stream* pe1_job, pe_stream* pe2_job, pe_stream* pe3_job, pe_stream* pe4_job,
		rcache_stream* cache_stream1, rcache_stream* cache_stream2, rcache_stream* cache_stream3, rcache_stream* cache_stream4){

	pe_job temp_pe;
	rcache_request temp_rcache_request;

	// if there is an available PE and task, assign the task
	// totally tnum tasks
	for (id_t i=0; i<tnum; ) {
#pragma HLS PIPELINE II=1
		if ((!pe1_idle->empty() && !rm1_idle->empty()) || (!pe2_idle->empty() && !rm2_idle->empty()) ||
				(!pe3_idle->empty() && !rm3_idle->empty()) || (!pe4_idle->empty() && !rm4_idle->empty())) {
			Avc temp_avc = Avcstream->read();

			// from distributor --> pe: value , rid
			temp_pe.rid = temp_avc.rid;
			temp_pe.val = temp_avc.val;
			temp_pe.end = false;

			temp_rcache_request.Acid = temp_avc.cid;
			temp_rcache_request.end = false;
			temp_rcache_request.type = CACHE_READ;
			temp_rcache_request.Acid1 = 0;
			temp_rcache_request.data = 0;

			if (!pe1_idle->empty() && !rm1_idle->empty()) {
				pe1_idle->read();
				rm1_idle->read();
				pe1_job->write(temp_pe);

				temp_rcache_request.dest = 1;
				cache_stream1->write(temp_rcache_request);
			} else if (!pe2_idle->empty() && !rm2_idle->empty()) {
				pe2_idle->read();
				rm2_idle->read();
				pe2_job->write(temp_pe);

				temp_rcache_request.dest = 2;
				cache_stream2->write(temp_rcache_request);
			} else if (!pe3_idle->empty() && !rm3_idle->empty()) {
				pe3_idle->read();
				rm3_idle->read();
				pe3_job->write(temp_pe);

				temp_rcache_request.dest = 3;
				cache_stream3->write(temp_rcache_request);
			} else if (!pe4_idle->empty() && !rm4_idle->empty()) {
				pe4_idle->read();
				rm4_idle->read();
				pe4_job->write(temp_pe);

				temp_rcache_request.dest = 4;
				cache_stream4->write(temp_rcache_request);
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

	temp_rcache_request.end = true;
	cache_stream1->write(temp_rcache_request);
	cache_stream2->write(temp_rcache_request);
	cache_stream3->write(temp_rcache_request);
	cache_stream4->write(temp_rcache_request);
}

#endif
