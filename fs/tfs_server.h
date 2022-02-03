#ifndef TFS_SERVER_H
#define TFS_SERVER_H

#include "common/common.h"
#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* struct to pass parsed data from main to worker threads */
typedef struct {
  // persistent
  int session_id;
  int client_pipe_fd;

  // current request data
  int opcode;
  char client_pipe_path[MAX_PIPE_NAME];
  char name[MAX_FILE_NAME];
  int flags;
  int fhandle;
  size_t len;
  char *buffer; // char[len]
} tfs_session_data_t;

int dispatch(int opcode, int session_id,
             int (*parser)(tfs_session_data_t *data),
             int (*handler)(tfs_session_data_t *data));
void lock_free_sessions();
void unlock_free_sessions();
ssize_t read_server_pipe(void *buf, size_t n_bytes);

#endif /* TFS_SERVER_H */