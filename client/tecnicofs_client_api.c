#include "tecnicofs_client_api.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int client, server, session_id;
char pipename[MAX_PIPE_NAME];

int tfs_mount(char const *client_pipe_path, char const *server_pipe_path) {
  // remove pipe if it already exists
  if (unlink(client_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", client_pipe_path,
            strerror(errno));
    return -1;
  }

  // create pipe
  if (mkfifo(client_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return -1;
  }

  // open pipe for writing
  // this waits for someone to open it for reading
  int tx = open(server_pipe_path, O_WRONLY);
  if (tx == -1) {
    fprintf(stderr, "[ERR]: server open failed: %s\n", strerror(errno));
    return -1;
  }
  server = tx;
  // TODO: check syscall
  memcpy(pipename, client_pipe_path, MAX_PIPE_NAME);

  u_int8_t buffer[MAX_PIPE_NAME + 1];
  buffer[0] = TFS_OP_CODE_MOUNT;
  //  TODO: check syscalls
  memcpy(buffer + 1, client_pipe_path,
         (strlen(client_pipe_path) + 1) * sizeof(char));
  write(tx, buffer, (MAX_PIPE_NAME + 1) * sizeof(char));

  int rx = open(client_pipe_path, O_RDONLY);
  if (rx == -1) {
    fprintf(stderr, "[ERR]: client open failed: %s\n", strerror(errno));
    // TODO: close tx?
    return -1;
  }
  client = rx;
  read(rx, &session_id, sizeof(int));

  printf("[INFO]: session id: %d\n", session_id);
  return 0;
}

int tfs_unmount() {
  /* TODO: Implement this */
  char buffer[1 + SESSION_ID_LENGTH];
  buffer[0] = TFS_OP_CODE_UNMOUNT;
  // TODO: check syscalls
  memcpy(buffer + 1, &session_id, SESSION_ID_LENGTH);
  write(server, buffer, (1 + SESSION_ID_LENGTH) * sizeof(char));
  if (close(server) == -1) {
    fprintf(stderr, "[ERR]: server close failed: %s\n", strerror(errno));
    return -1;
  }
  if (unlink(pipename) != 0) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipename,
            strerror(errno));
    return -1;
  }
  return 0;
}

int tfs_open(char const *name, int flags) {
  /* TODO: Implement this */
  return -1;
}

int tfs_close(int fhandle) {
  /* TODO: Implement this */
  return -1;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t len) {
  /* TODO: Implement this */
  return -1;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
  /* TODO: Implement this */
  return -1;
}

int tfs_shutdown_after_all_closed() {
  /* TODO: Implement this */
  return -1;
}
