#include "server_handlers.h"

int handle_tfs_mount(tfs_session_data_t *data) {
  data->client_pipe_fd = open(data->client_pipe_path, O_WRONLY);
  if (data->client_pipe_fd == -1) {
    fprintf(stderr, "[ERR]: client open failed: %s\n", strerror(errno));
    return -1;
  }

  // purposely ignoring case where session_id is -1 (take failed) because
  // the client needs to be informed of the problem anyway
  return write_client_pipe(data->client_pipe_fd, &data->session_id,
                           sizeof(int));
}

int handle_tfs_unmount(tfs_session_data_t *data) {
  int ret = 0;
  if (write_client_pipe(data->client_pipe_fd, &ret, sizeof(int)) == -1) {
    return -1;
  }
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

int handle_tfs_write(tfs_session_data_t *data) {
  int written = (int)tfs_write(data->fhandle, data->buffer, data->len);
  free(data->buffer);
  return write_client_pipe(data->client_pipe_fd, &written, sizeof(int));
}

int handle_tfs_read(tfs_session_data_t *data) {
  char *buffer = (char *)malloc(data->len * sizeof(char));
  ssize_t read_bytes = tfs_read(data->fhandle, buffer, data->len);
  int ret = (int)read_bytes; // required by spec
  if (write_client_pipe(data->client_pipe_fd, &ret, sizeof(int)) == -1) {
    free(buffer);
    return -1;
  }
  if (read_bytes != -1 &&
      write_client_pipe(data->client_pipe_fd, buffer,
                        (size_t)read_bytes * sizeof(char)) == -1) {
    free(buffer);
    return -1;
  }
  free(buffer);
  return 0;
}

int handle_tfs_shutdown_after_all_closed(tfs_session_data_t *data) {
  int ret = tfs_destroy_after_all_closed();
  return write_client_pipe(data->client_pipe_fd, &ret, sizeof(int));
}

int write_client_pipe(int client_pipe_fd, const void *buf, size_t n_bytes) {
  if (write(client_pipe_fd, buf, n_bytes) != n_bytes) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}