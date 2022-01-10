#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for (int i = 0; i < NUM_DIRECT_BLOCKS; i++) {
                    if (inode->i_direct_data_blocks[i] != -1) {
                        if (data_block_free(inode->i_direct_data_blocks[i]) ==
                            -1) {
                            return -1;
                        }
                    }
                    if (inode->i_indirect_data_block != -1) {
                        int *indirect_data_block =
                            (int *)data_block_get(inode->i_indirect_data_block);
                        if (indirect_data_block == NULL) {
                            return -1;
                        }
                        for (int j = 0; j < NUM_INDIRECT_ENTRIES; j++) {
                            if (indirect_data_block[j] != -1 &&
                                data_block_free(indirect_data_block[j]) == -1) {
                                return -1;
                            }
                        }
                        if (data_block_free(inode->i_indirect_data_block) ==
                            -1) {
                            return -1;
                        }
                    }
                }
                inode->i_size = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be
         * created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and
     * there is an error adding an entry to the open file table, the file is
     * not opened but it remains created */
}

int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

/* Returns how much was just written or -1 if an error occurs */
ssize_t tfs_write_aux(size_t written, size_t to_write, void const *buffer,
                      int *blocks, size_t i, size_t block_offset) {
    void *block = NULL;
    if (blocks[i] == -1) {
        int b = data_block_alloc();
        if (b == -1) {
            return -1;
        }
        blocks[i] = b;
        block = data_block_get(b);
    } else {
        block = data_block_get(blocks[i]);
    }
    // write to block and increase written
    /* Perform the actual write */

    size_t remaining_to_write = to_write - written;
    size_t block_space = BLOCK_SIZE - block_offset;
    size_t len =
        remaining_to_write > block_space ? block_space : remaining_to_write;

    memcpy(block + block_offset, buffer + written, len);
    return (ssize_t)len;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to write */
    if (to_write + file->of_offset > MAX_FILE_SIZE) {
        to_write = MAX_FILE_SIZE - file->of_offset;
    }

    size_t written = 0; // maybe use file->of_offset?

    for (size_t i = 0; i < NUM_DIRECT_BLOCKS && written < to_write; i++) {
        ssize_t just_written = tfs_write_aux(written, to_write, buffer,
                                             inode->i_direct_data_blocks, i,
                                             file->of_offset % BLOCK_SIZE);
        if (just_written == -1) {
            inode->i_size += written;
            return -1;
        }
        written += (size_t)just_written;
        file->of_offset += (size_t)just_written;
    }

    if (written < to_write) { // need indirect blocks
        if (inode->i_indirect_data_block == -1) {
            int b = data_block_alloc();
            if (b == -1) {
                inode->i_size += written;
                return -1;
            }
        }
        int *indirect_data_block =
            (int *)data_block_get(inode->i_indirect_data_block);
        if (indirect_data_block == NULL) {
            inode->i_size += written;
            return -1;
        }
        for (size_t i = 0; i < NUM_INDIRECT_ENTRIES && written < to_write;
             i++) {
            ssize_t just_written =
                tfs_write_aux(written, to_write, buffer, indirect_data_block, i,
                              file->of_offset % BLOCK_SIZE);
            if (just_written == -1) {
                inode->i_size += written;
                return -1;
            }
            written += (size_t)just_written;
            file->of_offset += (size_t)just_written;
        }
    }

    inode->i_size =
        file->of_offset; // in theory, the same as inode->i_size += written;
    return (ssize_t)written;
}

/* Returns how much was just read or -1 if an error occurs */
ssize_t tfs_read_aux(size_t read, size_t to_read, void const *buffer,
                     int *blocks, size_t i, size_t block_offset) {
    if (blocks[i] == -1) {
        return -1;
    }
    void *block = data_block_get(blocks[i]);

    // read from block and increase read
    /* Perform the actual read */

    size_t remaining_to_read = to_read - read;
    size_t block_space = BLOCK_SIZE - block_offset;
    size_t len =
        remaining_to_read > block_space ? block_space : remaining_to_read;

    memcpy((void *)buffer + read, block + block_offset, len);
    return (ssize_t)len;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    size_t read = 0;

    for (size_t i = 0; i < NUM_DIRECT_BLOCKS && read < to_read; i++) {
        ssize_t just_read =
            tfs_read_aux(read, to_read, buffer, inode->i_direct_data_blocks, i,
                         file->of_offset % BLOCK_SIZE); // TODO: review args
        if (just_read == -1) {
            return -1;
        }
        read += (size_t)just_read;
        file->of_offset += (size_t)just_read;
    }

    if (read < to_read) { // need indirect blocks
        if (inode->i_indirect_data_block == -1) {
            return -1;
        }
        int *indirect_data_block =
            (int *)data_block_get(inode->i_indirect_data_block);
        if (indirect_data_block == NULL) {
            return -1;
        }
        for (size_t i = 0; i < NUM_INDIRECT_ENTRIES && read < to_read; i++) {
            ssize_t just_read =
                tfs_read_aux(read, to_read, buffer, indirect_data_block, i,
                             file->of_offset % BLOCK_SIZE);
            if (just_read == -1) {
                return -1;
            }
            read += (size_t)just_read;
            file->of_offset += (size_t)just_read;
        }
    }

    return (ssize_t)to_read;
}