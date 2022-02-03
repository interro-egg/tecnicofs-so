#include "server_handlers.h"

int handle_tfs_mount(tfs_session_data_t *data) {
  data->client_pipe_fd = open(data->client_pipe_path, O_WRONLY);
  if (data->client_pipe_fd == -1) {
    fprintf(stderr, "[ERR]: client open failed: %s\n", strerror(errno));
    return -1;
  }

  // purposely ignoring case where session_id is -1 (take failed) because
  // the client needs to be informed of the problem anyway
  if (write(data->client_pipe_fd, &data->session_id, sizeof(int)) == -1) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    close(data->client_pipe_fd); // already erroring, no need to check
    return -1;
  } // TODO: write returns 0 if pipe closed

  return 0;
}

int handle_tfs_unmount(tfs_session_data_t *data) {
  return close(data->client_pipe_fd);
}

int handle_tfs_open(tfs_session_data_t *data) {
  int fd = tfs_open(data->name, data->flags);
  return write_client_pipe(data->client_pipe_fd, &fd, sizeof(int));
}

int handle_tfs_close(tfs_session_data_t *data) {
  int ret = tfs_close(data->fhandle);
  return write_client_pipe(data->client_pipe_fd, &ret, sizeof(int));
}

int write_client_pipe(int client_pipe_fd, const void *buf, size_t n_bytes) {
  if (write(client_pipe_fd, buf, n_bytes) == -1) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    return -1;
  } // TODO: check for 0
  return 0;
}