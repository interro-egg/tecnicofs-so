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
  tfs_init();
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

  int server =
      open(pipename, O_RDONLY); // blocks until someone is at the other end
  if (server == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (;;) {
    char opcode;
    ssize_t ret = read(server, &opcode, sizeof(u_int8_t));
    if (ret == 0) {
      // ret == 0 signals EOF
      fprintf(stderr, "[INFO]: pipe closed\n"); // FIXME: remove this
      return 0;
    } else if (ret == -1) {
      // ret == -1 signals error
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "[INFO]: reading opcode: %d \n", opcode);

    switch (opcode) {
    case TFS_OP_CODE_MOUNT: {
      int session_id, client;
      char buffer[MAX_PIPE_NAME];
      if (read(server, buffer, MAX_PIPE_NAME * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      } // TODO: read returns 0 if pipe closed
      client = open(buffer, O_WRONLY);
      if (client == -1) {
        fprintf(stderr, "[ERR]: client open failed: %s\n", strerror(errno));
        continue;
      }

      lock_free_sessions();
      for (int i = 0; i < MAX_SESSION_COUNT; i++) {
        if (free_sessions[i] == FREE) {
          free_sessions[i] = TAKEN;
          client_pipe_fds[i] = client;
          session_id = i;
          break;
        }
      }
      unlock_free_sessions();

      if (write(client, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        lock_free_sessions();
        free_sessions[session_id] = FREE;
        unlock_free_sessions();
        close(client);
        continue;
      } // TODO: write returns 0 if pipe closed

      break;
    }
    case TFS_OP_CODE_UNMOUNT: {
      int session_id, client;
      if (read(server, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      if (close(client) == -1) {
        fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
        continue;
      }
      lock_free_sessions();
      free_sessions[session_id] = FREE;
      unlock_free_sessions();
      break;
    }
    case TFS_OP_CODE_OPEN: {
      char buffer[MAX_FILE_NAME];
      int flags, session_id, client;
      if (read(server, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      if (read(server, buffer, MAX_FILE_NAME * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      if (read(server, &flags, FLAGS_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      int fd = tfs_open(buffer, flags);
      printf("[INFO]: tfs_open(%s, %d) returned %d\n", buffer, flags, fd);
      if (write(client, &fd, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
      break;
    }
    case TFS_OP_CODE_CLOSE: {
      int fd, session_id, client;
      if (read(server, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      if (read(server, &fd, FHANDLE_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      int retval = tfs_close(fd);
      printf("[INFO]: tfs_close returned %d\n", retval);
      if (write(client, &retval, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
      break;
    }
    case TFS_OP_CODE_WRITE: {
      int fd, session_id, client;
      size_t size;
      ssize_t written;
      if (read(server, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      if (read(server, &fd, FHANDLE_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      // TODO: check if size > 1024 ?
      if (read(server, &size, SIZE_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      char *to_write = (char *)malloc(size * sizeof(char));
      if (read(server, to_write, size * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      written = tfs_write(fd, to_write, size);
      printf("[INFO]: written %zd B\n", written);
      if (write(client, &written, sizeof(ssize_t)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
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

  close(server);

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