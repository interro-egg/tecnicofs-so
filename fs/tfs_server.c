#include "tfs_server.h"
#include "operations.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int client_pipe_fds[MAX_SESSION_COUNT];
allocation_state_t free_sessions[MAX_SESSION_COUNT] = {FREE};
pthread_mutex_t free_sessions_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Please specify the pathname of the server's pipe.\n");
    return 1;
  }

  char *pipename = argv[1];
  printf("Starting TecnicoFS server with pipe called %s\n", pipename);

  // remove pipe if it already exists
  if (unlink(pipename) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipename,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // create pipe
  if (mkfifo(pipename, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int rx = open(pipename, O_RDONLY); // blocks until someone is at the other end
  if (rx == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (;;) {
    char opcode;
    ssize_t ret = read(rx, &opcode, sizeof(u_int8_t));
    if (ret == 0) {
      // ret == 0 signals EOF
      fprintf(stderr, "[INFO]: pipe closed\n"); // FIXME: remove this
      return 0;
    } else if (ret == -1) {
      // ret == -1 signals error
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "[INFO]: received %zd B\n", ret);

    switch (opcode) {
    case TFS_OP_CODE_MOUNT: {
      char buffer[MAX_PIPE_NAME];
      if (read(rx, buffer, MAX_PIPE_NAME * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      } // TODO: read returns 0 if pipe closed
      int tx = open(buffer, O_WRONLY);
      if (tx == -1) {
        fprintf(stderr, "[ERR]: client open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }

      int session_id = -1;
      lock_free_sessions();
      for (int i = 0; i < MAX_SESSION_COUNT; i++) {
        if (free_sessions[i] == FREE) {
          free_sessions[i] = TAKEN;
          client_pipe_fds[i] = tx;
          session_id = i;
          break;
        }
      }
      unlock_free_sessions();

      if (write(tx, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      } // TODO: write returns 0 if pipe closed

      break;
    }
    case TFS_OP_CODE_UNMOUNT: {
      int session_id;
      if (read(rx, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      int tx = client_pipe_fds[session_id];
      if (close(tx) == -1) {
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
      lock_free_sessions();
      free_sessions[session_id] = FREE;
      unlock_free_sessions();
      break;
    }
    case TFS_OP_CODE_OPEN: {
      printf("opening\n");
      break;
    }
    case TFS_OP_CODE_CLOSE: {
      printf("closing\n");
      break;
    }
    case TFS_OP_CODE_WRITE: {
      printf("writing\n");
      break;
    }
    case TFS_OP_CODE_READ: {
      printf("reading\n");
      break;
    }
    case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED: {
      printf("shutting down\n");
      break;
    }
    default: {
      printf("not ok\n");
    }
    }
  }

  close(rx);

  pthread_mutex_destroy(&free_sessions_lock);

  return 0;
}

void lock_free_sessions() {
  if (pthread_mutex_lock(&free_sessions_lock) != 0) {
    fprintf(stderr, "[ERR]: pthread_mutex_lock failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void unlock_free_sessions() {
  if (pthread_mutex_unlock(&free_sessions_lock) != 0) {
    fprintf(stderr, "[ERR]: pthread_mutex_lock failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}