#include "server_parsers.h"
#include <errno.h>
#include <stdio.h>

int parse_tfs_mount(tfs_session_data_t *data) {
  read_server_pipe(data->client_pipe_path, MAX_PIPE_NAME * sizeof(char));
  return 0;
}

int parse_tfs_open(tfs_session_data_t *data) {
  read_server_pipe(data->name, MAX_FILE_NAME * sizeof(char));
  read_server_pipe(&data->flags, sizeof(int));
  return 0;
}

int parse_tfs_close(tfs_session_data_t *data) {
  read_server_pipe(&data->fhandle, sizeof(int));
  return 0;
}

int parse_tfs_write(tfs_session_data_t *data) {
  read_server_pipe(&data->fhandle, sizeof(int));
  read_server_pipe(&data->len, sizeof(size_t));
  data->buffer = (char *)malloc(data->len * sizeof(char));
  if (data->buffer == NULL) {
    fprintf(stderr, "[ERR]: malloc failed: %s\n", strerror(errno));
    return -1;
  }
  read_server_pipe(data->buffer, data->len * sizeof(char));
  return 0;
}

int parse_tfs_read(tfs_session_data_t *data) {
  read_server_pipe(&data->fhandle, sizeof(int));
  read_server_pipe(&data->len, sizeof(size_t));
  return 0;
}