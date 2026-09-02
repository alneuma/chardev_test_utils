#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STR_INVALID_CHUNK_SIZE \
	"chunk size must be an integer value within the range [1, SSIZE_MAX]"
#define ERR_INVALID_CHUNK_SIZE -1

#define STR_ZERO_WRITE "zero write"
#define ERR_ZERO_WRITE -2

#define USAGE(progname) "usage:\n%s [CHUNK-SIZE] [INPUT-STRING]\n", (progname)

static void print_err(const char *prog_name, int err);
static int buf_to_fd_chunked(int fd, const char *buf, size_t buf_len,
			     size_t chunk_size);

int main(int argc, char *argv[])
{
	char *endptr;
	unsigned long chunk_size;
	int ret;

	if (argc != 3) {
		fprintf(stderr, USAGE(argv[0]));
		return EXIT_FAILURE;
	}

	errno = 0;
	chunk_size = strtoul(argv[1], &endptr, 10);
	if (errno == ERANGE) {
		print_err(argv[0], ERANGE);
		return EXIT_FAILURE;
	} else if (*endptr != '\0' || chunk_size < 1 ||
		   chunk_size > SSIZE_MAX) {
		print_err(argv[0], ERR_INVALID_CHUNK_SIZE);
		return EXIT_FAILURE;
	}

	ret = buf_to_fd_chunked(STDOUT_FILENO, argv[2], strlen(argv[2]),
				chunk_size);
	if (ret) {
		print_err(argv[0], ret);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int buf_to_fd_chunked(int fd, const char *buf, size_t buf_len,
			     size_t chunk_size)
{
	ssize_t ret;
	size_t write_size;
	size_t idx = 0;

	while (idx < buf_len) {
		write_size = buf_len - idx;
		if (write_size > chunk_size)
			write_size = chunk_size;
		ret = write(fd, buf + idx, write_size);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return errno;
		} else if (ret == 0)
			return ERR_ZERO_WRITE;
		idx += (size_t)ret; // safe because ret > 0
	}
	return 0;
}

static void print_err(const char *prog_name, int err)
{
	char errbuf[256];

	switch (err) {
	case ERR_INVALID_CHUNK_SIZE:
		fprintf(stderr, "%s: %s\n", prog_name, STR_INVALID_CHUNK_SIZE);
		break;
	case ERR_ZERO_WRITE:
		fprintf(stderr, "%s: %s\n", prog_name, STR_ZERO_WRITE);
		break;
	default:
		if (strerror_r(err, errbuf, sizeof(errbuf)) !=
		    0) // POSIX not GNU
			fprintf(stderr, "%s: error %d\n", prog_name, err);
		else
			fprintf(stderr, "%s: %s\n", prog_name, errbuf);
	}
}
