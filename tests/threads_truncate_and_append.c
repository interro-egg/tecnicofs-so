#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

#define LEN 1555
#define THREADS 19

void *truncate(void *ipath) {

    char str[LEN];
    memset(str, 'C', LEN);
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
    memset(str, 'B', LEN);
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
    ssize_t r = tfs_write(f, contents, LEN);
    assert(r == LEN);
    assert(tfs_close(f) != -1);
}

int main() {
    char path[THREADS][10];
    char strA[LEN];
    memset(strA, 'A', LEN);
    char strB[LEN];
    memset(strB, 'B', LEN);
    char strC[LEN];
    memset(strC, 'C', LEN);
    assert(tfs_init() != -1);
    for (int i = 0; i < THREADS; i++) {
        sprintf(path[i], "/%d.txt", i);
        write_to_file(path[i], strA);
    }
    pthread_t tid[THREADS];
    for (int i = 0; i < THREADS; i++) {
        if (i % 2 == 0) {
            pthread_create(&tid[i], NULL, &append, (void *)path[i]);
        } else {
            pthread_create(&tid[i], NULL, &truncate, (void *)path[i]);
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(tid[i], NULL);

        int f = tfs_open(path[i], 0);
        assert(f != -1);

        char tmp[LEN];
        if (i % 2 == 0) {
            // append
            assert(tfs_read(f, tmp, LEN) == LEN);
            assert(memcmp(strA, tmp, LEN) == 0);
            assert(tfs_read(f, tmp, LEN) == LEN);
            assert(memcmp(strB, tmp, LEN) == 0);
            assert(tfs_read(f, tmp, LEN) == 0);
        } else {
            // truncate
            assert(tfs_read(f, tmp, LEN) == LEN);
            assert(memcmp(strC, tmp, LEN) == 0);

            assert(tfs_read(f, tmp, LEN) == 0);
        }
        assert(tfs_close(f) == 0);
    }

    assert(tfs_destroy() == 0);

    printf("Successful test.\n");
    return 0;
}