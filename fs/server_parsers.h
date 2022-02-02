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

#endif /* SERVER_PARSERS_H */