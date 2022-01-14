#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

void *function(void *ipath) {

    char *str = (char *)malloc(sizeof(char) * 1555);
    memset(str, 'A', 1554);
    char *path = (char *)ipath;

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_TRUNC);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    assert(tfs_close(f) != -1);

    pthread_exit(NULL);
}

int main() {
    char *path = "/f1";
    char *str = (char *)malloc(sizeof(char) * 1555);
    memset(str, 'A', 1554);
    int f;
    ssize_t r;
    char buffer[2000];
    assert(tfs_init() != -1);

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    assert(tfs_close(f) != -1);
    pthread_t tid[20];
    for (int i = 0; i < 20; i++) {
        pthread_create(&tid[i], NULL, &function, (void *)path);
    }
    for (int i = 0; i < 20; i++) {
        pthread_join(tid[i], NULL);
    }

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);

    printf("Successful test.\n");
    return 0;
}