#ifndef SERVER_PARSERS_H
#define SERVER_PARSERS_H

#include "tfs_server.h"

int parse_tfs_mount(tfs_request_t *req);
int parse_tfs_unmount(tfs_request_t *req);
int parse_tfs_open(tfs_request_t *req);
int parse_tfs_close(tfs_request_t *req);
int parse_tfs_write(tfs_request_t *req);
int parse_tfs_read(tfs_request_t *req);
int parse_tfs_shutdown_after_all_closed(tfs_request_t *req);

#endif /* SERVER_PARSERS_H */