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