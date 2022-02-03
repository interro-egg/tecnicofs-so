#include "tfs_server.h"
#include "server_handlers.h"
#include "server_parsers.h"

char *server_pipe_name;
int server_pipe_fd;
int client_pipe_fds[MAX_SESSION_COUNT];
tfs_session_data_t session_data[MAX_SESSION_COUNT];
allocation_state_t free_sessions[MAX_SESSION_COUNT] = {FREE};
pthread_mutex_t free_sessions_lock = PTHREAD_MUTEX_INITIALIZER;

int dispatch(int opcode, int session_id,
             int (*parser)(tfs_session_data_t *data),
             int (*handler)(tfs_session_data_t *data)) {

  lock_free_sessions();
  if (free_sessions[session_id] != TAKEN) {
    unlock_free_sessions();
    fprintf(stderr, "[ERR]: session_id %d is not taken\n", session_id);
    return -1;
  }
  unlock_free_sessions();

  tfs_session_data_t *data = &session_data[session_id];

  data->session_id = session_id;
  data->opcode = opcode;

  if (parser != NULL && parser(data) == -1) {
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

ssize_t read_server_pipe(void *buf, size_t n_bytes) {
  ssize_t ret = read(server_pipe_fd, buf, n_bytes);
  if (ret == 0) {
    // ret == 0 signals EOF
    close(server_pipe_fd);
    fprintf(stderr, "[INFO]: no clients\n");
    server_pipe_fd = open(server_pipe_name, O_RDONLY);
    if (server_pipe_fd == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  } else if (ret == -1) {
    // ret == -1 signals error
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  return ret;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Please specify the pathname of the server's pipe.\n");
    return 1;
  }

  server_pipe_name = argv[1];
  tfs_init();
  printf("Starting TecnicoFS server with pipe called %s\n", server_pipe_name);

  // remove pipe if it already exists
  if (unlink(server_pipe_name) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", server_pipe_name,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // create pipe
  if (mkfifo(server_pipe_name, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  server_pipe_fd = open(server_pipe_name,
                        O_RDONLY); // blocks until someone is at the other end
  if (server_pipe_fd == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (;;) {
    char opcode;
    if (read_server_pipe(&opcode, sizeof(char)) == 0) {
      // EOF, pipe re-opened
      continue;
    }

    int session_id = -1;
    if (opcode != TFS_OP_CODE_MOUNT) {
      // TODO: handle error, possibly incorporating into the above if
      read(server_pipe_fd, &session_id, sizeof(int));
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
      dispatch(opcode, session_id, parse_tfs_open, handle_tfs_open);
      break;
    }
    case TFS_OP_CODE_CLOSE: {
      dispatch(opcode, session_id, parse_tfs_close, handle_tfs_close);
      break;
    }
    case TFS_OP_CODE_WRITE: {
      dispatch(opcode, session_id, parse_tfs_write, handle_tfs_write);
      break;
    }
    case TFS_OP_CODE_READ: {
      dispatch(opcode, session_id, parse_tfs_read, handle_tfs_read);
      break;
    }
    case TFS_OP_CODE_SHUTDOWN_AFTER_ALL_CLOSED: {
      dispatch(opcode, session_id, NULL, handle_tfs_shutdown_after_all_closed);
      fprintf(stderr, "[INFO]: server shutting down\n");
      exit(EXIT_SUCCESS);
    }
    default: {
      // can´t recover 💀 (unkown request length)
      fprintf(stderr, "[ERR]: unknown opcode %d\n", opcode);
      exit(EXIT_FAILURE);
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