#include "server_parsers.h"
#include <errno.h>
#include <stdio.h>

int parse_tfs_mount(int server_pipe_fd, tfs_session_data_t *data) {
  read_server_pipe(server_pipe_fd, data->client_pipe_path,
                   MAX_PIPE_NAME * sizeof(char));
  return 0;
}

void read_server_pipe(int server_pipe_fd, void *buf, size_t n_bytes) {
  if (read(server_pipe_fd, buf, n_bytes) == -1) {
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  } // TODO: read returns 0 if pipe closed (maybe close & re-open?)
}