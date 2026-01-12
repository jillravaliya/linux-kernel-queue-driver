#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define DRIVER_NAME "/dev/jill"
#define SET_SIZE_OF_QUEUE _IOW('a', 'a', int*)


int main(void) {

	int fd = open(DRIVER_NAME, O_RDWR);
	int size = 100;
	int ret = ioctl(fd, SET_SIZE_OF_QUEUE, &size);
	close(fd);
	return ret;
}

