#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

#define LEN 1555

void *truncate(void *ipath) {

    char str[LEN];
    memset(str, 'A', LEN);
    char *path = (char *)ipath;

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_TRUNC);
    assert(f != -1);

    r = tfs_write(f, str, LEN);
    assert(r == LEN);

    assert(tfs_close(f) != -1);

    pthread_exit(NULL);
}

void *append(void *ipath) {

    char str[LEN];
    memset(str, 'A', LEN);
    char *path = (char *)ipath;

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_APPEND);
    assert(f != -1);

    r = tfs_write(f, str, LEN);
    assert(r == LEN);

    assert(tfs_close(f) != -1);

    pthread_exit(NULL);
}

void write_to_file(char *path, char *contents) {
    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);
    ssize_t r = tfs_write(f, contents, strlen(contents));
    assert(r == strlen(contents));
    assert(tfs_close(f) != -1);
}

int main() {
    char *path_append = "/append.txt";
    char *path_trunc = "/trunc.txt";
    char str[LEN];
    memset(str, 'A', LEN);
    assert(tfs_init() != -1);

    write_to_file(path_append, str);
    write_to_file(path_trunc, str);

    pthread_t tid[19];
    for (int i = 0; i < 19; i++) {
        if (i % 2 == 0)
            pthread_create(&tid[i], NULL, &append, (void *)path_append);
        else
            pthread_create(&tid[i], NULL, &truncate, (void *)path_trunc);
    }
    for (int i = 0; i < 19; i++) {
        pthread_join(tid[i], NULL);
    }

    // TODO: check

    /*f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);*/

    printf("Successful test.\n");
    return 0;
}