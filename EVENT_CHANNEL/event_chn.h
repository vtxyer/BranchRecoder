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
#include <xs.h>
//#include <xen/hvm/save.h>
#include <setjmp.h>
#include <poll.h>
}
#include <map>
using namespace std;

int domID;
xc_interface *xch4, *xch3, *xch2, *xch1;


extern "C"{
	int init_hypercall(int recent_cr3_size, int fd){
		int ret, i;

		//Init
/*		privcmd_hypercall_t hyper1 = { 
			__HYPERVISOR_vt_op, 
			{ 1, domID, recent_cr3_size, 0, 0}
		};
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);*/

		return ret;
	}

}
