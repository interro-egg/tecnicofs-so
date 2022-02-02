#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "tfs_server.h"

int handle_tfs_mount(tfs_request_t *req);
int handle_tfs_unmount(tfs_request_t *req);
int handle_tfs_open(tfs_request_t *req);
int handle_tfs_close(tfs_request_t *req);
int handle_tfs_write(tfs_request_t *req);
int handle_tfs_read(tfs_request_t *req);
int handle_tfs_shutdown_after_all_closed(tfs_request_t *req);

#endif /* SERVER_HANDLERS_H */