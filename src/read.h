#ifndef __HIREDIS_READ_H
#define __HIREDIS_READ_H

#include <stdio.h> 

typedef struct redisReadTask {
    int type;
    long long elements; /* number of elements in multibulk container */
    int idx; /* index in parent (array) object */
    void *obj; /* holds user-generated value for a read task */
    struct redisReadTask *parent; /* parent task */
    void *privdata; /* user-settable arbitrary field */
} redisReadTask;

typedef struct redisReplyObjectFunctions {
    void *(*createString)(const redisReadTask*, char*, size_t);
    void *(*createArray)(const redisReadTask*, size_t);
    void *(*createInteger)(const redisReadTask*, long long);
    void *(*createDouble)(const redisReadTask*, double, char*, size_t);
    void *(*createNil)(const redisReadTask*);
    void *(*createBool)(const redisReadTask*, int);
    void (*freeObject)(void*);
} redisReplyObjectFunctions;

typedef struct redisReader {
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */

    char *buf; /* Read buffer */
    size_t pos; /* Buffer cursor */
    size_t len; /* Buffer length */
    size_t maxbuf; /* Max length of unused buffer */
    long long maxelements; /* Max multi-bulk elements */

    redisReadTask **task;
    int tasks;

    int ridx; /* Index of current read task */
    void *reply; /* Temporary reply pointer */

    redisReplyObjectFunctions *fn;
    void *privdata;
} redisReader;

#endif