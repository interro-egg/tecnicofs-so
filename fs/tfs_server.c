#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int client_pipe_fds[MAX_SESSION_COUNT];
allocation_state_t free_sessions[MAX_SESSION_COUNT] = {FREE};

int main(int argc, char **argv)
{

	if (argc < 2) {
		printf("Please specify the pathname of the server's pipe.\n");
		return 1;
	}

	char *pipename = argv[1];
	printf("Starting TecnicoFS server with pipe called %s\n", pipename);

	// remove pipe if it already exists
	if (unlink(pipename) != 0 && errno != ENOENT) {
		fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipename,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	// create pipe
	if (mkfifo(pipename, 0640) != 0) {
		fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int rx = open(pipename, O_RDONLY); // blocks until someone is at the other end
	if (rx == -1) {
		fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (;;) {
		char opcode;
		ssize_t ret = read(rx, &opcode, sizeof(char)); // FIXME: this might not work
		if (ret == 0) {
			// ret == 0 signals EOF
			fprintf(stderr, "[INFO]: pipe closed\n"); // FIXME: remove this
			return 0;
		} else if (ret == -1) {
			// ret == -1 signals error
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "[INFO]: received %zd B\n", ret);

		switch (opcode) {
		case TFS_OP_CODE_MOUNT: {
			char buffer[MAX_PIPE_NAME];
			// TODO: check syscall
			read(rx, buffer, MAX_PIPE_NAME);
			int tx = open(buffer, O_WRONLY);
			for (int i = 0; i < MAX_SESSION_COUNT; i++) {
				if (free_sessions[i] == FREE) {
					free_sessions[i] = TAKEN;
					client_pipe_fds[i] = tx;
					write(tx, &i, sizeof(int));
					break;
				}
			}
			// TODO: if no free sessions
			break;
		}
		case TFS_OP_CODE_UNMOUNT: {
			printf("unmounting\n");
			break;
		}
		case TFS_OP_CODE_OPEN: {
			printf("opening\n");
			break;
		}
		case TFS_OP_CODE_CLOSE: {
			printf("closing\n");
			break;
		}
		case TFS_OP_CODE_WRITE: {
			printf("writing\n");
			break;
		}
		case TFS_OP_CODE_READ: {
			printf("reading\n");
			break;
		}
		case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED: {
			printf("shutting down\n");
			break;
		}
		default: {
			printf("not ok\n");
		}
		}
	}

	close(rx);

	return 0;
}