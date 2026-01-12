#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define IOCTL_SET_SIZE _IOW('a', 'a', int*)

int main() {

	int fd;
	int size = 10;

	printf("Opening device .... \n");
	fd = open("/dev/jill", O_RDWR);
	if(fd < 0) {
		printf("Failed to open device\n");
		return -1;
	}

	printf("Sending SET_SIZE ioctl...\n");
	ioctl(fd, IOCTL_SET_SIZE, &size);

	printf("Closing device...\n");
	close(fd);

	return 0;
}
