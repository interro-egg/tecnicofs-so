#include "server_parsers.h"
#include <errno.h>
#include <stdio.h>

int parse_tfs_mount(int server_pipe_fd, tfs_session_data_t *data) {
  // FIXME: vv is this if-read operation worth abstracting?
  if (read(server_pipe_fd, data->client_pipe_path,
           MAX_PIPE_NAME * sizeof(char)) == -1) {
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  } // TODO: read returns 0 if pipe closed (maybe close & re-open?)

  return 0;
}