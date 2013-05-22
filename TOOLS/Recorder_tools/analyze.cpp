#include <iostream>
#include <csignal>

#include "analyze.h"
#include <set>
using namespace std;


int main(int argc, char *argv[])  
{
	int host_domID;
	int tmp = 0;
	unsigned long guest_cr3, host_cr3;
	void *page_table_space;
	struct debug_store *ds;
	struct Bts_record *bts;
	struct pollfd poll_fd;
	unsigned long bts_size_order;
	int port, rc;
	Node_val* container[MON_NUM] = {NULL};
	//signal(SIGINT, end_process);


	/*Init data_base*/
	for(int k=0; k<MON_NUM; k++){
		data_base[k].ds = NULL;
		data_base[k].bts = NULL;
	}


	printf("PID: %d\n", getpid());
	printf("Input host domID: ");
	fscanf(stdin, "%d", &host_domID);
	printf("Input guest domID: ");
	fscanf(stdin, "%d", &guest_domID);
	printf("Input guest cr3: ");
	fscanf(stdin, "%lx", &guest_cr3);
	printf("Host pid %d, Input host cr3: ", getpid());
	fscanf(stdin, "%lx", &host_cr3);
	printf("BTS size[4K]: ");
	fscanf(stdin, "%lu", &bts_size_order);

    fd = open("/proc/xen/privcmd", O_RDWR);  
    if (fd < 0) {   
        perror("open");  
        exit(1);  
    }   


	/*Alloc space for page table*/
	page_table_space = memalign(PAGE_SIZE, PAGE_SIZE);
	set_ept(fd, guest_domID, host_domID, host_cr3, (unsigned long)page_table_space);
	set_guest_page_table(fd, guest_domID, guest_cr3);

	vpmu_data = (struct vpmu_data *)memalign(PAGE_SIZE, PAGE_SIZE);
	if(vpmu_data == NULL){
		printf("vpmu_data alloc error\n");
		return -1;
	}
	//memset(vpmu_data, 0x99, sizeof(*vpmu_data));
	vpmu_data->host_domID = host_domID;	
	init_vpmu_data(guest_domID, vpmu_data, bts_size_order, fd, host_cr3);


	//Event channel init	
	xch = xc_evtchn_open(NULL, 0);
	if(xch == NULL){
		fprintf(stderr, "xch alloc error\n");
		exit(1);
	}
	xc_evtchn_bind_virq(xch, 20);
	poll_fd.fd = xc_evtchn_fd(xch);
	poll_fd.events = POLLIN | POLLERR;
	set_event_chn(fd, host_domID);	


	rc = 0;
	/*Waiting for VIRQ*/
	while(1){
		rc = poll(&poll_fd, 1, -1);
		if(rc == -1){
			fprintf(stderr, "poll exitted with an error");
			return -1;
		}
		else if(rc == 1){
			port = xc_evtchn_pending(xch);
			if (port == -1) {
				perror("failed to read port from evtchn");
				exit(EXIT_FAILURE);
			}
			printf("port %d\n", port);
			rc = xc_evtchn_unmask(xch, port);
			if (rc == -1) {
				perror("failed to write port to evtchn");
				return -1;
			}		
			/*proceed data*/
			for(int k=0; k<MON_NUM; k++){
				if(data_base[k].ds != NULL && data_base[k].bts != NULL){
					ds = data_base[k].ds;
					bts = data_base[k].bts;
					if(ds->bts_interrupt_threshold == ds->bts_index){
//						go_through_bts(bts, ds->bts_buffer_base, ds->bts_index);
						printf("ds->bts_interrupt_threshold:%lx ds->bts_index: %lx  ds->bts_buffer_base:%lx ptr:%d\n", 
								data_base[0].ds->bts_interrupt_threshold, data_base[0].ds->bts_index, data_base[0].ds->bts_buffer_base, k);
						printf("ds->bts_interrupt_threshold:%lx ds->bts_index: %lx  ds->bts_buffer_base:%lx ptr:%d\n", 
								data_base[1].ds->bts_interrupt_threshold, data_base[1].ds->bts_index, data_base[1].ds->bts_buffer_base, k);
						ds->bts_index = ds->bts_buffer_base;
					}
				}
			}
		}

	}

	return 0;
}


