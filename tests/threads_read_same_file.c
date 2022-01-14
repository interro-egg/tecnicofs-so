#include "fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>

void *function(void *istr) {

    char *str = (char *)istr;
    char *path = "/f1";
    char buffer[4000];

    int f;
    ssize_t r;

    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_close(f) != -1);

    f = tfs_open(path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str));

    buffer[r] = '\0';
    assert(strcmp(buffer, str) == 0);

    assert(tfs_close(f) != -1);

    pthread_exit(NULL);
}

int main() {
    char *str = (char *)malloc(sizeof(char) * 1555);
    memset(str, 'A', 1555);
    char *path = "/f1";

    int f;
    ssize_t r;

    assert(tfs_init() != -1);
    f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, str, strlen(str));
    assert(r == strlen(str));
    pthread_t tid[20];
    for (int i = 0; i < 20; i++) {
        pthread_create(&tid[i], NULL, &function, str);
    }
    for (int i = 0; i < 20; i++) {
        pthread_join(tid[i], NULL);
    }
    printf("Successful test.\n");
    return 0;
}