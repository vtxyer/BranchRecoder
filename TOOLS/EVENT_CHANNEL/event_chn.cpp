#include <iostream>
#include <csignal>

#include "event_chn.h"
#include <set>
using namespace std;



int main(int argc, char *argv[])  
{ 
	int fd, ret, i, rc; 
	unsigned long offset = 0;
	

	if(argc<2){
		printf("%s domID\n", argv[0]);
		exit(1);
	}
	domID = atoi(argv[1]);

	xch1 = xc_evtchn_open(NULL, 0);
	if(xch1 == NULL){
		fprintf(stderr, "xch alloc error\n");
		exit(1);
	}
	fd = open("/proc/xen/privcmd", O_RDWR);  
	if (fd < 0) {  
		perror("open\n");  
		exit(1);  
	}
	ret = xc_evtchn_bind_virq(xch1, 20);
	if(ret == -1){
		fprintf(stderr, "bind virq error\n");
		return -1;
	}

	struct pollfd poll_fd;
	int port;

	poll_fd.fd = xc_evtchn_fd(xch1);
	poll_fd.events = POLLIN | POLLERR;

	rc = poll(&poll_fd, 1, -1);
	if(rc == -1){
		fprintf(stderr, "poll exitted with an error");
		return -1;
	}
	if(rc == 1){
        port = xc_evtchn_pending(xch1);
        if (port == -1) {
            perror("failed to read port from evtchn");
            exit(EXIT_FAILURE);
        }
		printf("port %d\n", port);
/*        if (port != virq_port) {
            fprintf(stderr,
                    "unexpected port returned from evtchn (got %d vs expected %d)\n",
                    port, virq_port);
			return -1;
        }*/
        rc = xc_evtchn_unmask(xch1, port);
        if (rc == -1) {
            perror("failed to write port to evtchn");
			return -1;
        }	
	}


	xc_interface_close(xch1);

	return 0;
}


