#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 2000
#define THREADS 20

void *function(void *ipath) {

    char *str = (char *)malloc(sizeof(char) * 1555);
    memset(str, 'A', 1554);
    str[1554] = '\0';
    char *path = (char *)ipath;

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));

    assert(tfs_close(f) != -1);

    free(str);

    pthread_exit(NULL);
}

void *function2(void *ipath) {

    char *str = (char *)malloc(sizeof(char) * 1555);
    memset(str, 'A', 1554);
    str[1554] = '\0';
    char *path = (char *)ipath;
    char buffer[BUFFER_SIZE];

    int f;
    ssize_t r;

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, BUFFER_SIZE);
    assert(r == strlen(str));

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);

    free(str);

    pthread_exit(NULL);
}

int main() {

    assert(tfs_init() != -1);

    pthread_t tid[THREADS];

    char path[THREADS][10];
    for (int i = 0; i < THREADS; i++) {
        sprintf(path[i], "/f%d", i);

        pthread_create(&tid[i], NULL, &function, (void *)path[i]);
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    for (int i = 0; i < THREADS; i++) {
        sprintf(path[i], "/f%d", i);

        pthread_create(&tid[i], NULL, &function2, (void *)path[i]);
    }
    for (int i = 0; i < THREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    printf("Successful test.\n");
    return 0;
}