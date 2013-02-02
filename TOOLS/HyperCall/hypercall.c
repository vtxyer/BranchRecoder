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
#include <signal.h>
#include <malloc.h>

#define ADDR 0xffff88007a792200
int fd, ret, i, domID;

int main(int argc, char *argv[])  
{    
    double percent_threshold, percent;
    int monitor_time, sample_times;
    unsigned long *buff;
	unsigned long pointer;

	printf("Input Domain ID: \0");
	fscanf(stdin, "%d", &domID);
    fd = open("/proc/xen/privcmd", O_RDWR);  
    if (fd < 0) {  
        perror("open");  
        exit(1);  
    }

	buff = memalign(4096, 4096);
	memset(buff, 0, 4096);
	printf("buff %lx\n", (unsigned long)buff);


    //Hypercall Structure    
    privcmd_hypercall_t hcall_0 = { 
        __HYPERVISOR_vt_op, 
        { 1, domID, ADDR, 0, 0}
    };   
    privcmd_hypercall_t hcall_1 = { 
        __HYPERVISOR_vt_op, 
        { 2, domID, 0, 0, 0}
    };  
    privcmd_hypercall_t hcall_2 = { 
        __HYPERVISOR_vt_op, 
        { 3, domID, 0, 0, 0}
    };   

    privcmd_hypercall_t hcall_test = { 
        __HYPERVISOR_vt_op, 
        { 8, domID, (unsigned long)buff, 0, 0}
    }; 


	ret = 0;
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hcall_test);
    if(ret<0)
        printf("Hypercall error errno %d\n", errno);

	close(fd);
    return 0;

}  
