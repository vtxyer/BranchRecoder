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
			{ 0, domID, 0, (unsigned long)bts_flag}
		};  
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		
		if(ret<0){
			perror("get bts flag error\n");
			return ret;
		}
		else{
			return bts_flag[0];
		}
	}
	int sent_vpmu_data(int fd, int domID, struct vpmu_data *vpmu_data){
		int ret;
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 1, domID, 0, (unsigned long)vpmu_data}
		};  
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		
		return ret;
	}

}                                    


int init_vpmu_data(int domID, struct vpmu_data *vpmu_data, unsigned long bts_size_order, int fd)
{
	struct debug_store *ds;
	void *bts_buffer;
	int bts_enable;
//	int ret;
	unsigned long tmp;	

	bts_enable = get_bts_flag(fd, domID);
	if(bts_enable < 0){
		return -1;
	}

	if(bts_enable == 0){	
		vpmu_data->bts_size_order = bts_size_order;

		/*alloc debug store register*/
		ds = (struct debug_store *)memalign(PAGE_SIZE, PAGE_SIZE);	
		if(ds == NULL){
			printf("<VT> ds alloc error\n");
			return -1;
		}
		memset(ds, 0, sizeof(*ds));
		printf("ds base %lx.Waiting for Input:\n", (unsigned long)ds);
		scanf("%d", &tmp);
		vpmu_data->host_ds_addr = tmp; 
//		vpmu_data->host_ds_addr = (unsigned long)ds; 


		bts_buffer = memalign(PAGE_SIZE, bts_size_order*PAGE_SIZE);
		if(bts_buffer == NULL)
		{
			printf("<VT> bts buffer alloc error\n");
			return -1;
		}
		memset(bts_buffer, 0, bts_size_order*PAGE_SIZE);
		printf("bts base %lx.Waiting for Input:\n", (unsigned long)ds);
		scanf("%d", &tmp);
		vpmu_data->host_bts_base = tmp;
//		vpmu_data->host_bts_base = (unsigned long)bts_buffer;

		/*Send msgs to hypervisor*/
		sent_vpmu_data(fd, domID, vpmu_data);

		bts_enable = get_bts_flag(fd, domID);
		if(bts_enable < 0){
			return -1;
		}	
		printf("<VT>debug_store init ok\n");
		return 0;
	}
	else{
		printf("<VT> Already init debug store\n");
		return -2;
	}
}

