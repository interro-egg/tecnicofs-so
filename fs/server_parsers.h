#ifndef SERVER_PARSERS_H
#define SERVER_PARSERS_H

#include "tfs_server.h"

int parse_tfs_mount(tfs_session_data_t *data);
int parse_tfs_unmount(tfs_session_data_t *data);
int parse_tfs_open(tfs_session_data_t *data);
int parse_tfs_close(tfs_session_data_t *data);
int parse_tfs_write(tfs_session_data_t *data);
int parse_tfs_read(tfs_session_data_t *data);
int parse_tfs_shutdown_after_all_closed(tfs_session_data_t *data);

#endif /* SERVER_PARSERS_H */