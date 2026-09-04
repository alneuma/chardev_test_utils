/*
 * chunk_reader.c
 *
 * Reads and then immediately prints from a file to stdout ensuring, that read()
 * is never called with buffer sizes larger than [READ-SIZE].
 * Additionally a delay is passed as an argument. It specifies the number of
 * seconds to wait between each read-print cycle.
 * 
 */
#define _POSIX_C_SOURCE 200809L // for SSIZE_MAX

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STR_READ_SIZE \
	"read size must be an integer value within the range [1, SSIZE_MAX]"
#define STR_ZERO_WRITE "zero write"
#define STR_NUMBER_FORMAT "not correct number format"
#define STR_DELAY \
	"delay must be an integer value within the range [0, UINT_MAX]"

enum {
	ERR_READ_SIZE = -1,
	ERR_ZERO_WRITE = -2,
	ERR_NUMBER_FORMAT = -3,
	ERR_DELAY = -4,
};

#define USAGE(progname)                                         \
	"usage:\n%s [READ-SIZE] [DELAY] [INPUT-FILE]\n"         \
	"\nREAD-SIZE: buffer size of individual read() calls\n" \
	"DELAY: delay between reads in seconds\n"               \
	"INPUT-FILE: file to read from\n",                      \
		(progname)

/*
 * return values for read_and_print() and parse_ulong():
 * success	-> 0
 * custom error	-> < 0
 * unix error	-> > 0
 */
static int read_and_print(int fd, size_t read_size, unsigned int delay);
static int parse_ulong(unsigned long *res, const char *str);
static void print_err(const char *prog_name, const char *func, int err);

int main(int argc, char *argv[])
{
	unsigned long tmp_read_size;
	unsigned long tmp_delay;
	size_t read_size;
	unsigned int delay;
	int ret;
	int tmp_ret;
	int fd;

	if (argc != 4) {
		fprintf(stderr, USAGE(argv[0]));
		return EXIT_FAILURE;
	}

	ret = parse_ulong(&tmp_read_size, argv[1]);
	if (ret) {
		print_err(argv[0], "parse_ulong", ret);
		return EXIT_FAILURE;
	} else if (tmp_read_size < 1 || tmp_read_size > SSIZE_MAX) {
		print_err(argv[0], "parse_ulong", ERR_READ_SIZE);
		return EXIT_FAILURE;
	}
	read_size = (size_t)tmp_read_size;

	ret = parse_ulong(&tmp_delay, argv[2]);
	if (ret) {
		print_err(argv[0], "parse_ulong", ret);
		return EXIT_FAILURE;
	} else if (tmp_delay > UINT_MAX) {
		print_err(argv[0], "parse_ulong", ERR_DELAY);
		return EXIT_FAILURE;
	}
	delay = (unsigned int)tmp_delay; // tmp_delay <= UINT_MAX

	fd = open(argv[3], O_RDONLY);
	if (fd < 0) {
		print_err(argv[0], "open", errno);
		return EXIT_FAILURE;
	}

	ret = read_and_print(fd, read_size, delay);
	tmp_ret = close(fd);
	if (ret) {
		print_err(argv[0], "read_and_print", ret);
		return EXIT_FAILURE;
	}
	ret = tmp_ret;
	if (ret) {
		print_err(argv[0], "close", errno);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int read_and_print(int fd, size_t read_size, unsigned int delay)
{
	ssize_t ret_read;
	ssize_t ret_write;
	unsigned int to_sleep;
	int ret = 0;
	size_t offset = 0;
	size_t write_size;
	char *buf;

	buf = malloc(read_size);
	if (!buf) {
		ret = ENOMEM;
		goto done;
	}

	for (;;) {
		for (;;) {
			ret_read = read(fd, buf, read_size);
			if (ret_read < 0 && errno == EINTR)
				continue;
			if (ret_read < 0) {
				ret = errno;
				goto done;
			}
			if (ret_read == 0) {
				ret = 0;
				goto done;
			}
			break;
		}

		offset = 0;
		write_size = (size_t)ret_read;
		while (write_size) {
			// ret read > 0
			ret_write =
				write(STDOUT_FILENO, buf + offset, write_size);
			if (ret_write < 0 && errno == EINTR)
				continue;
			if (ret_write < 0) {
				ret = errno;
				goto done;
			}
			if (ret_write == 0) {
				ret = ERR_ZERO_WRITE;
				goto done;
			}
			if ((size_t)ret_write < write_size) {
				offset += (size_t)ret_write;
				write_size -= (size_t)ret_write;
				continue;
			}
			break;
		}

		to_sleep = delay;
		while (to_sleep)
			to_sleep = sleep(to_sleep);
	}

done:
	free(buf);
	return ret;
}

static int parse_ulong(unsigned long *res, const char *str)
{
	char *endptr;

	if (!isdigit((unsigned char)*str))
		return ERR_NUMBER_FORMAT;
	errno = 0;
	*res = strtoul(str, &endptr, 10);
	if (errno == ERANGE)
		return ERANGE;
	else if (*endptr != '\0')
		return ERR_NUMBER_FORMAT;
	return 0;
}

static void print_err(const char *prog_name, const char *func, int err)
{
	switch (err) {
	case ERR_READ_SIZE:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func, STR_READ_SIZE);
		break;
	case ERR_ZERO_WRITE:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func,
			STR_ZERO_WRITE);
		break;
	case ERR_NUMBER_FORMAT:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func,
			STR_NUMBER_FORMAT);
		break;
	case ERR_DELAY:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func, STR_DELAY);
		break;
	default:
		fprintf(stderr, "%s: %s: %s\n", prog_name, func, strerror(err));
	}
}
