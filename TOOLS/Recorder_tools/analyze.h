extern "C"{
#include <stdio.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <string.h>  
#include <xenctrl.h>  
#include <xen/sys/privcmd.h> 
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <xenstore.h>
//#include <xen/hvm/save.h>
#include <setjmp.h>
#include <malloc.h>
}
#include <map>
using namespace std;

typedef unsigned long u64;
#define PAGE_SIZE 4096
#define BTS_RECORD_SIZE		24
#define MAX_PEBS_EVENTS      8
struct debug_store {
	u64	bts_buffer_base;
	u64	bts_index;
	u64	bts_absolute_maximum;
	u64	bts_interrupt_threshold;
	u64	pebs_buffer_base;
	u64	pebs_index;
	u64	pebs_absolute_maximum;
	u64	pebs_interrupt_threshold;
	u64	pebs_event_reset[MAX_PEBS_EVENTS];
};
struct vpmu_data{
	unsigned long host_domID; 
	unsigned long host_cr3; 
	unsigned long host_ds_addr;
	unsigned long host_bts_base;
	unsigned long bts_size_order;
};

extern "C"
{
	int get_bts_flag(int fd, int domID){
		int ret;
		unsigned long bts_flag[2];
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 0, domID, 0, 0, 0}
		};  
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		
		if(ret<0){
			perror("get bts flag error\n");
			return ret;
		}
		else{
			return ret;
		}
	}
	int sent_vpmu_data(int fd, int domID, struct vpmu_data *vpmu_data){
		int ret;
		printf("host_domID:%d host_cr3:%lx vpmu_data:%lx\n",
			(vpmu_data->host_domID), (vpmu_data->host_cr3), (unsigned long)vpmu_data
		);
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 1, domID, (vpmu_data->host_domID), (vpmu_data->host_cr3), (unsigned long)vpmu_data}
		};  
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		
		return ret;
	}

	unsigned long set_ds_guest_map(int fd, int domID){
		unsigned long ret;
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 6, domID, 0, 0, 0}
		};  
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		return ret;
	}
	unsigned long set_bts_guest_map(int fd, int domID){
		int ret;
		unsigned long bts_base;
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 7, domID, 0, 0, 0}
		};  
		bts_base = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		return bts_base;
	}
}                                    


int init_vpmu_data(int domID, struct vpmu_data *vpmu_data, unsigned long bts_size_order, int fd)
{
	struct debug_store *ds;
	void *bts_buffer;
	int bts_enable;
    int max, thresh;
    unsigned long buffer_size;
	unsigned long ret;
	unsigned long tmp;	
	unsigned long cr3;
	unsigned long guest_ds_addr;

	bts_enable = get_bts_flag(fd, domID);
	if(bts_enable < 0){
		return -1;
	}

//	if(bts_enable == 0){	
	if(1){	
		vpmu_data->bts_size_order = bts_size_order;

		printf("Pid %d Input cr3: ", getpid());
		scanf("%lx", &cr3);
		vpmu_data->host_cr3 = cr3;


		/*alloc debug store register*/
		ds = (struct debug_store *)memalign(PAGE_SIZE, PAGE_SIZE);	
		if(ds == NULL){
			printf("<VT> ds alloc error\n");
			return -1;
		}
		memset(ds, 1, sizeof(*ds));
		memset(ds, 0, sizeof(*ds));
		vpmu_data->host_ds_addr = (unsigned long)ds; 

		bts_buffer = memalign(PAGE_SIZE, bts_size_order*PAGE_SIZE);
		if(bts_buffer == NULL)
		{
			printf("<VT> bts buffer alloc error\n");
			return -1;
		}
		memset(bts_buffer, 1, bts_size_order*PAGE_SIZE);
		memset(bts_buffer, 0, bts_size_order*PAGE_SIZE);
		vpmu_data->host_bts_base = (unsigned long)bts_buffer;

		/*Send msgs to hypervisor*/
		sent_vpmu_data(fd, domID, vpmu_data);


		/*Set ds*/
		buffer_size = PAGE_SIZE*(vpmu_data->bts_size_order);   
		max = buffer_size / BTS_RECORD_SIZE;
		thresh = max;


		printf("init vpmu ok waiting for input\n");
		scanf("%d", &cr3);

		guest_ds_addr = set_ds_guest_map(fd, domID);
		if(guest_ds_addr==-1){
			printf("set_ds_guest_map error %lx\n", ret);
			return -1;
		}

		printf("init ds ok waiting for input\n");
		scanf("%d", &cr3);


		ds->bts_buffer_base = set_bts_guest_map(fd, domID);
		/*!!!!!!!!!!!!!!!!!!*/
		ds->bts_buffer_base = 0xffff880000000000 | (0xffffffffff&(ds->bts_buffer_base ));
		
		if(ds->bts_buffer_base == -1){
			printf("set_bts_guest_map error\n");
			return -1;
		}

		ds->bts_index = ds->bts_buffer_base;
		ds->bts_absolute_maximum = ds->bts_buffer_base +
			max * BTS_RECORD_SIZE;
		ds->bts_interrupt_threshold = ds->bts_buffer_base + 
			thresh * BTS_RECORD_SIZE;


		bts_enable = get_bts_flag(fd, domID);
		if(bts_enable < 0){
			return -1;
		}	
		printf("<VT>debug_store init ok\n");

		while(1){
			printf("guest_ds_addr:%lx ds->bts_buffer_base:%lx ds->bts_index:%lx\n", 
					guest_ds_addr, ds->bts_buffer_base, ds->bts_index);
			cin>>tmp;
		}

		return 0;
	}
	else{
		printf("<VT> Already init debug store\n");
		return -2;
	}


}

