#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>


#define DRIVER_NAME "/dev/jill"
#define PUSH_DATA _IOW('a', 'b', struct data*)


struct data {
	int length;
	char *data;
};

int main(void) {

	int fd = open(DRIVER_NAME, O_RDWR);
	struct data d;

	d.length = 3;
	d.data = malloc(3);
	memcpy(d.data, "xyz", 3);

	int ret = ioctl(fd, PUSH_DATA, &d);

	close(fd);
	free(d.data);

	return ret;
}

