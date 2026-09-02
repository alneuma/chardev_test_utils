/*
 * chunk_reader.c
 * reads from a fil ensuring, that read() is never called with buffer sizes
 * larger than [READ-SIZE]
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STR_INVALID_READ_SIZE \
	"read size must be an integer value within the range [1, SSIZE_MAX]"
#define ERR_INVALID_READ_SIZE -1

#define STR_ZERO_WRITE "zero write"
#define ERR_ZERO_WRITE -2

#define BUFSIZE_INTERNAL 4096

#define USAGE(progname) "usage:\n%s [READ-SIZE] [INPUT-FILE]\n", (progname)

static void print_err(const char *prog_name, const char *func, int err);
static ssize_t fd_to_buf(int *err, int fd, char *buf, size_t size);
static ssize_t buf_to_fd(int *err, int fd, const char *buf, size_t size);
static int transfer_read_size(int out_fd, int in_fd, size_t read_size);

int main(int argc, char *argv[])
{
	char *endptr;
	size_t read_size;
	unsigned long num_arg;
	int ret;
	int in_fd;

	if (argc != 3) {
		fprintf(stderr, USAGE(argv[0]));
		return EXIT_FAILURE;
	}

	errno = 0;
	num_arg = strtoul(argv[1], &endptr, 10);
	if (errno == ERANGE) {
		print_err(argv[0], "strtoul", ERANGE);
		return EXIT_FAILURE;
	} else if (*endptr != '\0' || num_arg < 1 || num_arg > SSIZE_MAX) {
		print_err(argv[0], "strtoul", ERR_INVALID_READ_SIZE);
		return EXIT_FAILURE;
	}
	read_size = (size_t)num_arg;

	errno = 0;
	in_fd = open(argv[2], O_RDONLY);
	if (in_fd < 0) {
		print_err(argv[0], "open", errno);
		return EXIT_FAILURE;
	}

	ret = transfer_read_size(STDOUT_FILENO, in_fd, read_size);
	if (ret) {
		print_err(argv[0], "transfer_read_size", ret);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int transfer_read_size(int out_fd, int in_fd, size_t read_size)
{
	char buf[BUFSIZE_INTERNAL];
	ssize_t ret;
	size_t buf_idx = 0;
	size_t next_read;
	size_t write_total;
	int err = 0;
	bool done = false;

	while (!done) {
		while (buf_idx < BUFSIZE_INTERNAL && !done) {
			next_read = read_size < BUFSIZE_INTERNAL - buf_idx ?
					    read_size :
					    BUFSIZE_INTERNAL - buf_idx;
			ret = fd_to_buf(&err, in_fd, buf + buf_idx, next_read);
			if (ret < 0)
				return err;
			if (ret == 0)
				done = true;
			buf_idx += (size_t)ret; /* safe because ret >= 0 */
		}
		write_total = 0;
		while (write_total < buf_idx) {
			ret = buf_to_fd(&err, out_fd, buf + write_total,
					buf_idx - write_total);
			if (ret < 0)
				return err;
			write_total += (size_t)ret; /* safe because ret >= 0 */
		}
		buf_idx = 0;
	}
	return 0;
}

static ssize_t fd_to_buf(int *err, int fd, char *buf, size_t size)
{
	ssize_t ret;

	while (true) {
		ret = read(fd, buf, size);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			*err = errno;
			return -1;
		}
		return ret;
	}
}

static ssize_t buf_to_fd(int *err, int fd, const char *buf, size_t size)
{
	ssize_t ret;

	while (true) {
		ret = write(fd, buf, size);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			*err = errno;
			return -1;
		}
		if (ret == 0) {
			*err = ERR_ZERO_WRITE;
			return -1;
		}
		return ret;
	}
}

static void print_err(const char *prog_name, const char *func, int err)
{
	char errbuf[256];

	switch (err) {
	case ERR_INVALID_READ_SIZE:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func,
			STR_INVALID_READ_SIZE);
		break;
	case ERR_ZERO_WRITE:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func,
			STR_ZERO_WRITE);
		break;
	default:
		if (strerror_r(err, errbuf, sizeof(errbuf)) !=
		    0) /* POSIX not GNU */
			fprintf(stderr, "%s: %s: error %d\n", prog_name, func,
				err);
		else
			fprintf(stderr, "%s: %s: %s\n", prog_name, func,
				errbuf);
	}
}
