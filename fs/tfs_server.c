#include "tfs_server.h"
#include "server_handlers.h"
#include "server_parsers.h"

int server_pipe_fd;
int client_pipe_fds[MAX_SESSION_COUNT];
tfs_session_data_t session_data[MAX_SESSION_COUNT];
allocation_state_t free_sessions[MAX_SESSION_COUNT] = {FREE};
pthread_mutex_t free_sessions_lock = PTHREAD_MUTEX_INITIALIZER;

int dispatch(int opcode, int session_id,
             int (*parser)(int server_pipe_fd, tfs_session_data_t *data),
             int (*handler)(tfs_session_data_t *data)) {

  lock_free_sessions();
  if (free_sessions[session_id] != TAKEN) {
    unlock_free_sessions();
    return -1;
  }
  unlock_free_sessions();

  tfs_session_data_t *data = &session_data[session_id];

  data->session_id = session_id;
  data->opcode = opcode;

  if (parser != NULL && parser(server_pipe_fd, data) == -1) {
    fprintf(stderr, "[ERR]: parser (opcode=%d) failed\n", opcode);
    return -1;
  }

  if (handler(data) == -1) {
    // in the future, this probably won't be a parameter and each thread will
    // pick the respective handler when taking up the task
    fprintf(stderr, "[ERR]: handler (opcode=%d) failed\n", opcode);
    return -1;
  }

  return 0;
}

// Takes the first free session, returns session ID or -1 if none are available
int take_session() {
  int session_id = -1;
  lock_free_sessions();
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (free_sessions[i] == FREE) {
      free_sessions[i] = TAKEN;
      session_id = i;
      break;
    }
  }
  unlock_free_sessions();
  return session_id;
}

int free_session(int session_id) {
  if (session_id < 0 || session_id > MAX_SESSION_COUNT)
    return -1;
  lock_free_sessions();
  free_sessions[session_id] = FREE;
  unlock_free_sessions();
  return 0;
}

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

  server_pipe_fd =
      open(pipename, O_RDONLY); // blocks until someone is at the other end
  if (server_pipe_fd == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (;;) {
    char opcode;
    ssize_t ret = read(server_pipe_fd, &opcode, sizeof(char));
    if (ret == 0) {
      // ret == 0 signals EOF
      fprintf(stderr, "[INFO]: pipe closed\n"); // FIXME: remove this
      return 0;
    } else if (ret == -1) {
      // ret == -1 signals error
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    int session_id = -1;
    if (opcode != TFS_OP_CODE_MOUNT) {
      // TODO: handle error, possibly incorporating into the above if
      read(server_pipe_fd, &session_id, sizeof(char));
    }

    switch (opcode) {
    case TFS_OP_CODE_MOUNT: {
      session_id = take_session();
      if (dispatch(opcode, session_id, parse_tfs_mount, handle_tfs_mount) ==
          -1) {
        free_session(session_id);
      }
      break;
    }
    case TFS_OP_CODE_UNMOUNT: {
      // no need to check for errors
      dispatch(opcode, session_id, NULL, handle_tfs_unmount);
      free_session(session_id); // FIXME: does this need to be in the thread?
      break;
    }
    case TFS_OP_CODE_OPEN: {
      // TODO: maybe check for errors?
      dispatch(opcode, session_id, parse_tfs_open, handle_tfs_open);
      break;
    }
    case TFS_OP_CODE_CLOSE: {
      // TODO: maybe check for errors?
      dispatch(opcode, session_id, parse_tfs_close, handle_tfs_close);
      break;
    }
    case TFS_OP_CODE_WRITE: {
      // TODO: maybe check for errors?
      dispatch(opcode, session_id, parse_tfs_write, handle_tfs_write);
      break;
    }
    case TFS_OP_CODE_READ: {
      int fd, session_id, client;
      size_t size;
      ssize_t was_read;
      if (read(server_pipe_fd, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      if (read(server_pipe_fd, &fd, FHANDLE_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      // TODO: check if size > 1024 ?
      if (read(server_pipe_fd, &size, SIZE_LENGTH * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      char *buffer = (char *)malloc(size * sizeof(char));
      was_read = tfs_read(fd, buffer, size);
      printf("[INFO]: read %zd B\n", was_read);
      if (write(client, &was_read, sizeof(ssize_t)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
      if (write(client, buffer, (size_t)was_read * sizeof(char)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
      free(buffer);
      break;
    }
    case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED: {
      int session_id, client, ret;
      if (read(server_pipe_fd, &session_id, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
        continue;
      }
      client = client_pipe_fds[session_id];
      ret = tfs_destroy_after_all_closed();
      printf("[INFO]: tfs_shutdown_after_all_closed returned %d\n", ret);
      if (write(client, &ret, sizeof(int)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        continue;
      }
      break;
    }
    default: {
      printf("not ok\n");
    }
    }
  }

  close(server_pipe_fd); // no need to check, we are exiting anyway
  lock_free_sessions();
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    if (free_sessions[i] == TAKEN) {
      close(session_data[i].client_pipe_fd);
    }
  }
  unlock_free_sessions();
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