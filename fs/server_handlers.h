#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "tfs_server.h"

int handle_tfs_mount(tfs_session_data_t *data);
int handle_tfs_unmount(tfs_session_data_t *data);
int handle_tfs_open(tfs_session_data_t *data);
int handle_tfs_close(tfs_session_data_t *data);
int handle_tfs_write(tfs_session_data_t *data);
int handle_tfs_read(tfs_session_data_t *data);
int handle_tfs_shutdown_after_all_closed(tfs_session_data_t *data);
int handle_tfs_ping(tfs_session_data_t *data);

int write_client_pipe(int client_pipe_fd, const void *buf, size_t n_bytes);

#endif /* SERVER_HANDLERS_H */