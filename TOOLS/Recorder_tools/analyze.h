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
#include <signal.h>
#include <poll.h>
}
#include <map>
using namespace std;

typedef unsigned long u64;
#define PAGE_SIZE 			4096
#define BTS_RECORD_SIZE		24
#define MAX_PEBS_EVENTS      8
#define MON_NUM				24

int fd, guest_domID;
xc_interface *xch;
struct vpmu_data *vpmu_data;
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
	unsigned long host_ds_addr[2];
	unsigned long host_bts_base[2];
	unsigned long bts_size_order;
};
struct Bts_record {
    u64 from;
    u64 to;
    u64 flags;
};
struct Node_val{
	struct debug_store *ds;
	struct Bts_record *bts;
	int waring;
};
struct Node_val data_base[MON_NUM];


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
	void va_to_mfn(int fd, int domID, unsigned long cr3, unsigned long gva){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 5, domID, cr3, gva, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);	
	}
	void clean_vpmu(int fd, int _guest_domID){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 6, _guest_domID, 0, 0, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
	void set_ept(int fd, int _guest_domID, int _host_domID, unsigned long _host_cr3, unsigned long _va){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 9, _guest_domID, _host_domID, _host_cr3, _va}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
	void set_guest_page_table(int fd, int _guest_domID, unsigned long _guest_cr3){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 9, _guest_domID, _guest_cr3, 0, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
	void start_monitoring(int fd, int _guest_domID, int op, unsigned long target_cr3){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 2, _guest_domID, op, target_cr3, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
	void stop_monitoring(int fd, int _guest_domID){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 3, _guest_domID, 0, 0, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
	void set_event_chn(int fd, int _host_domID){
		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 4, _host_domID, 0, 0, 0}
		};  
		ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);		
	}
}                                    


void end_process(int errno){
	clean_vpmu(fd, guest_domID);
	xc_interface_close(xch);
	close(fd);
    exit(errno);
}

void go_through_bts(Bts_record *bts, unsigned long start, unsigned long end){
	unsigned long ptr = start;
	Bts_record *bts_buffer = bts;
	while(ptr < end){
		printf("From:%lx To:%lx               ptr:%lx end:%lx\n", bts_buffer->from, bts_buffer->to, ptr, end);
		bts_buffer += 1;
		ptr += BTS_RECORD_SIZE;
	}
	printf("finish\n");
}

int init_vpmu_data(int domID, struct vpmu_data *vpmu_data, unsigned long bts_size_order, int fd, unsigned long host_cr3)
{
	void *bts_buffer;
	int bts_enable;
	unsigned long ret;
	unsigned long tmp;	
	int now_data_base_ptr = 0;
	struct debug_store *ds;

	bts_enable = get_bts_flag(fd, domID);
	if(bts_enable < 0){
		return -1;
	}

//	if(bts_enable == 0){	
	if(1){	
		vpmu_data->bts_size_order = bts_size_order;

		vpmu_data->host_cr3 = host_cr3;

		/*alloc debug store register*/
		for(int k=0; k<2; k++){
			//each vpmu have two bts info
			data_base[now_data_base_ptr].ds = (struct debug_store *)memalign(PAGE_SIZE, PAGE_SIZE);	
			ds = data_base[now_data_base_ptr].ds;
			if(ds == NULL){
				printf("<VT> ds alloc error\n");
				return -1;
			}
			memset(ds, 0, sizeof(*ds));
			vpmu_data->host_ds_addr[k] = (unsigned long)ds; 

			bts_buffer = memalign(PAGE_SIZE, bts_size_order*PAGE_SIZE);
			if(bts_buffer == NULL)
			{
				printf("<VT> bts buffer alloc error\n");
				return -1;
			}
			memset(bts_buffer, 0, bts_size_order*PAGE_SIZE);
			vpmu_data->host_bts_base[k] = (unsigned long)bts_buffer;
			data_base[now_data_base_ptr].bts = (struct Bts_record *)bts_buffer;

			/*0x123 for check in hypervisor map success or not*/
			data_base[now_data_base_ptr].ds->bts_buffer_base = 0x123;

			now_data_base_ptr++;
		}


		/*Clean first*/
		clean_vpmu(fd, guest_domID);
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

