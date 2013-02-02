#include <iostream>
#include <csignal>

#include "analyze.h"
#include <set>
using namespace std;


int main(int argc, char *argv[])  
{
	int fd, host_domID, domID;
	struct vpmu_data vpmu_data;
	int tmp;
	printf("PID: %d\n", getpid());

	printf("Input host domID: ");
	fscanf(stdin, "%d", &host_domID);
	vpmu_data.host_domID = host_domID;	

	printf("Input guest domID: ");
	fscanf(stdin, "%d", &domID);

    fd = open("/proc/xen/privcmd", O_RDWR);  
    if (fd < 0) {   
        perror("open");  
        exit(1);  
    }   
	
	init_vpmu_data(domID, &vpmu_data, 1, fd);

	cin>>tmp;

	close(fd);
	return 0;
}


