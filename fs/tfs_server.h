#ifndef TFS_SERVER_H
#define TFS_SERVER_H

#include "common/common.h"
#include <stdlib.h>

/* struct to pass parsed data from main to worker threads */
typedef struct {
  int opcode;
  int session_id;
  char client_pipe_path[MAX_PIPE_NAME];
  char name[MAX_FILE_NAME];
  int flags;
  int fhandle;
  size_t len;
  char *buffer; // char[len]
} tfs_request_t;

int dispatch(int opcode, int (*parser)(tfs_request_t *req),
             int (*handler)(tfs_request_t *req));
void lock_free_sessions();
void unlock_free_sessions();

#endif /* TFS_SERVER_H */