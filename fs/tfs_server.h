#ifndef TFS_SERVER_H
#define TFS_SERVER_H

#include "common/common.h"
#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* struct to pass parsed data from main to worker threads */
typedef struct tfs_session_data_t {
  // meta, persistent
  int session_id;
  int client_pipe_fd;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_t worker_thread;
  bool request_pending; // whether a request is ready to be handled

  // current request data
  int opcode;
  int (*handler)(struct tfs_session_data_t *data);
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
void *worker_thread(void *arg);
int take_session();
int free_session(int session_id);
ssize_t read_server_pipe(void *buf, size_t n_bytes);
void lock_mutex(pthread_mutex_t *mutex);
void unlock_mutex(pthread_mutex_t *mutex);

#endif /* TFS_SERVER_H */