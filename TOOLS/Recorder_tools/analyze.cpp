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
	//signal(SIGINT, end_process);

	printf("PID: %d\n", getpid());
	printf("Input host domID: ");
	fscanf(stdin, "%d", &host_domID);
	printf("Input guest domID: ");
	fscanf(stdin, "%d", &guest_domID);
	printf("Input guest cr3: ");
	fscanf(stdin, "%lx", &guest_cr3);
	printf("Host pid %d, Input host cr3: ", getpid());
	fscanf(stdin, "%lx", &host_cr3);

    fd = open("/proc/xen/privcmd", O_RDWR);  
    if (fd < 0) {   
        perror("open");  
        exit(1);  
    }   


	page_table_space = memalign(PAGE_SIZE, PAGE_SIZE);
	printf("test page_table_space:%lx\n", (unsigned long)page_table_space);
	set_ept(fd, guest_domID, host_domID, host_cr3, (unsigned long)page_table_space);
	set_guest_page_table(fd, guest_domID, guest_cr3);

	vpmu_data = (struct vpmu_data *)memalign(PAGE_SIZE, PAGE_SIZE);
	if(vpmu_data == NULL){
		printf("vpmu_data alloc error\n");
		return -1;
	}
	//memset(vpmu_data, 0x99, sizeof(*vpmu_data));
	vpmu_data->host_domID = host_domID;	
	init_vpmu_data(guest_domID, vpmu_data, 5, fd, host_cr3);
	
	struct debug_store *ds;
	struct Bts_record *bts;


	while(1){
		for(int k=0; k<2; k++){
			ds = data_base[k].ds;
			bts = data_base[k].bts;
			printf("from:%lx to:%lx\n", bts->from, bts->to);
			printf("bts_buffer_base:%lx bts_index:%lx bts_absolute_maximum:%lx bts_interrupt_threshold:%lx\n",
				ds->bts_buffer_base, ds->bts_index, ds->bts_absolute_maximum, ds->bts_interrupt_threshold);
		}
		cin>>tmp;
	}

	close(fd);
	return 0;
}


