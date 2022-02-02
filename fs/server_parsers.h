#ifndef SERVER_PARSERS_H
#define SERVER_PARSERS_H

#include "tfs_server.h"

int parse_tfs_mount(int server_pipe_fd, tfs_session_data_t *data);
int parse_tfs_open(int server_pipe_fd, tfs_session_data_t *data);
int parse_tfs_close(int server_pipe_fd, tfs_session_data_t *data);
int parse_tfs_write(int server_pipe_fd, tfs_session_data_t *data);
int parse_tfs_read(int server_pipe_fd, tfs_session_data_t *data);
int parse_tfs_shutdown_after_all_closed(int server_pipe_fd,
                                        tfs_session_data_t *data);

void read_server_pipe(int server_pipe_fd, void *buf, size_t n_bytes);

#endif /* SERVER_PARSERS_H */