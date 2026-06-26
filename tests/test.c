
MODULE_NAME = driver

obj-m += $(MODULE_NAME).o

export CROSS_COMPILE=riscv64-linux-gnu-
export ARCH=riscv

.PHONY: build clean run



build:
	$(MAKE) $(MODULE_NAME).ko

$(MODULE_NAME).ko: $(MODULE_NAME).c
	$(MAKE) -C ~/linux modules M=$(shell pwd)
	cp $(MODULE_NAME).ko ~/rootfs
	cp $(MODULE_NAME).mod ~/rootfs
	cd ~/rootfs && find . | cpio -co > ../rootfs.cpio && cd ..

clean:
	$(MAKE) -C ~/linux clean M=$(shell pwd)


run: $(MODULE_NAME).ko
	cd ~/linux && qemu-system-riscv64 -machine virt -nographic -no-reboot -kernel arch/riscv/boot/Image -initrd ~/rootfs.cpio -append 'panic=-1' # init=~/rootfs/sh
[nicholas_calabro@kdlp character_device]$ ^C
(arg: 1) ^C
[nicholas_calabro@kdlp character_device]$ cat tests/test.c 
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/errno.h>


char* validate_lseek_set();
char* validate_lseek_cur();
char* validate_lose();

char* validate_empty_read();

char* validate_input_rejection();

static char* (*tests[]) (void) = {
	validate_lseek_set,
	validate_lseek_cur,
	validate_lose,
	validate_empty_read,
	validate_input_rejection,
};

static const char *names[] = {
	"validate_lseek_set",
	"validate_lseek_cur",
	"validate_lose",
	"validate_empty_read",
	"validate_input_rejection",
};

#define NUM_TESTS (sizeof(tests) / sizeof(tests[0]))



static const char error_message;

/* driver for test functions */
int main () {

	puts("TAP version 15");
	printf("1..%d\n", NUM_TESTS);

	for (int i = 0; i < NUM_TESTS; i++) {

		char* result = tests[i] ();

		if (result == NULL)
			printf(    "ok %d - %s\n", i + 1, names[i]);
		else {
			printf("not ok %d - %s\n", i + 1, names[i]);
			if (result != NULL)
				printf("  ---\n  message: %s\n  ...\n", result);
		}
	}

	return 0;
}

/* test definitions */

char* validate_lseek_set() {
	int fd = open("/dev/wordle", O_RDWR);
	if (fd == -1)
		return "open returned -1";

	const char *str = "AEIOS"; // common letters
	int str_len = strlen(str);
	if (write(fd, str, str_len) == -1)
		return "write returned -1";

	if (lseek(fd, 0, SEEK_SET) < 0)
		return "lseek returned -1";

	char buf[8];
	if (read(fd, buf, 5) != 5)
		return "read return != 5";

	buf[5] = '\0';



	if (strncmp("00000", buf, str_len) != 0)
		return "strcmp not equal";

	close(fd);
	return NULL;
}

char* validate_lseek_cur() {
	fprintf(stderr, "run\n");
	int ret;
	int fd = open("/dev/wordle", O_RDWR);
	if (fd == -1)
		return "open returned -1";

	// reset guesses
	if (lseek(fd, 0, SEEK_SET) < 0)
		return "lseek0 returned -1";

	fprintf(stderr, "run until 0\n");

	const char *str = "AEIOS"; // common letters
	int str_len = strlen(str);

	if ((ret = lseek(fd, 0, SEEK_CUR)) != 5){
		fprintf(stderr, "lseek returned %d\n", ret);
		return "lseek2 wrong return";
	}
	fprintf(stderr, "run until 2\n");
	if (write(fd, str, str_len) == -1)
		return "write returned -1";

	if (lseek(fd, 0, SEEK_CUR) != 4)
		return "lseek3 wrong return";
	if (write(fd, str, str_len) == -1)
		return "write returned -1";

	if (lseek(fd, 0, SEEK_CUR) != 3)
		return "lseek wrong return";
	if (write(fd, str, str_len) == -1)
		return "write returned -1";

	if (lseek(fd, 0, SEEK_CUR) != 2)
		return "lseek wrong return";

	if (write(fd, str, str_len) == -1)
		return "write returned -1";

	if (lseek(fd, 0, SEEK_CUR) != 1)
		return "lseek wrong return";


	close(fd);
	return NULL;
}

char* validate_lose() {
    int fd = open("/dev/wordle", O_RDWR);
    if (fd == -1)
        return "open returned -1";

    if (lseek(fd, 0, SEEK_SET) < 0)
        return "lseek returned -1";

    const char *str = "AEIOS";
    int str_len = strlen(str);

    for (int i = 0; i < 5; i++)
        if (write(fd, str, str_len) == -1)
            return "write returned -1";

    char buf[6] = {0};
    if (read(fd, buf, 5) != 5)
        return "read did not return 5";

    buf[5] = '\0';
    if (strncmp("33333", buf, 5) != 0)
        return "strcmp not equal";

    close(fd);
    return NULL;
}

char* validate_empty_read() {
    int fd = open("/dev/wordle", O_RDWR);
    if (fd == -1)
        return "open returned -1";

    if (lseek(fd, 0, SEEK_SET) < 0)
        return "lseek returned -1";

    char buf[6] = {0};
    ssize_t n = read(fd, buf, 5);
    if (n != 5)
        return "read did not return 5 bytes";

    buf[5] = '\0';
    if (strncmp("00000", buf, 5) != 0)
        return "initial feedback is not 00000";

    n = read(fd, buf, 5);
    if (n != 0)
        return "second read should return 0 (EOF)";

    close(fd);
    return NULL;
}


char* validate_input_rejection() {
	int fd = open("/dev/wordle", O_RDWR);
	if (fd == -1)
		return "open returned -1";

	if (lseek(fd, 0, SEEK_SET) < 0)
		return "lseek returned -1";

	const char *str = "AEIOs"; // lowercase forbidden
	int str_len = strlen(str);
	if (write(fd, str, str_len) != -1)
		return "write didnt return -1";

	const char *str2 = "AEIO0"; // must be only capicat letters no numbers
	int str_len2 = strlen(str2);
	if (write(fd, str2, str_len2) != -1)
		return "write didnt return -1";


	close(fd);
	return NULL;
}

