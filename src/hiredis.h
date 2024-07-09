#ifndef __HIREDIS_H
#define __HIREDIS_H

#include <stdint.h> /*例如: uint32_t, uint8_t 这样的类型*/
#include "read.h"

/* 基本的redis连接操作函数*/
typedef struct redisContextFuncs {
    void (*close)(struct redisContext *);
    void (*free_privctx)(void *);
    void (*async_read)(struct redisAsyncContext *);
    void (*async_write)(struct redisAsyncContext *);

    /* Read/Write data to the underlying communication stream, returning the
     * number of bytes read/written.  In the event of an unrecoverable error
     * these functions shall return a value < 0.  In the event of a
     * recoverable error, they should return 0. */
    ssize_t (*read)(struct redisContext *, char *, size_t);
    ssize_t (*write)(struct redisContext *);
} redisContextFuncs;

enum redisConnectionType {
    REDIS_CONN_TCP,
    REDIS_CONN_UNIX,
    REDIS_CONN_USERFD
};

#define REDIS_OPT_NONBLOCK 0x01
#define REDIS_OPT_REUSEADDR 0x02
#define REDIS_OPT_NOAUTOFREE 0x04        
                                         /* Don't automatically free the async
                                          * object on a connection failure, or
                                          * other implicit conditions. Only free
                                          * on an explicit call to disconnect()
                                          * or free() */
#define REDIS_OPT_NO_PUSH_AUTOFREE 0x08  
                                         /* Don't automatically intercept and
                                          * free RESP3 PUSH replies. */
#define REDIS_OPT_NOAUTOFREEREPLIES 0x10 /* Don't automatically free replies. */
#define REDIS_OPT_PREFER_IPV4 0x20       /* Prefer IPv4 in DNS lookups. */
#define REDIS_OPT_PREFER_IPV6 0x40       /* Prefer IPv6 in DNS lookups. */
#define REDIS_OPT_PREFER_IP_UNSPEC (REDIS_OPT_PREFER_IPV4 | REDIS_OPT_PREFER_IPV6)

/* In Unix systems a file descriptor is a regular signed int, with -1
 * representing an invalid descriptor. In Windows it is a SOCKET
 * (32- or 64-bit unsigned integer depending on the architecture), where
 * all bits set (~0) is INVALID_SOCKET.  */
#ifndef _WIN32
typedef int redisFD;
#define REDIS_INVALID_FD -1
#else
#ifdef _WIN64
typedef unsigned long long redisFD; /* SOCKET = 64-bit UINT_PTR */
#else
typedef unsigned long redisFD;      /* SOCKET = 32-bit UINT_PTR */
#endif
#define REDIS_INVALID_FD ((redisFD)(~0)) /* INVALID_SOCKET */
#endif

/**
 * Helper macros to initialize options to their specified fields.
 */
#define REDIS_OPTIONS_SET_TCP(opts, ip_, port_) do { \
        (opts)->type = REDIS_CONN_TCP;               \
        (opts)->endpoint.tcp.ip = ip_;               \
        (opts)->endpoint.tcp.port = port_;           \
    } while(0)

typedef void (redisPushFn)(void *, void *);

/* Redis数据库连接上下文类型 redisContext*/
typedef struct redisContext {
    const redisContextFuncs *funcs;   /* Function table */

    int err; /* 错误标志，0表示没有错误 */
    char errstr[128]; /* 错误信息，错误的字符串表示 */
    redisFD fd;
    int flags;
    char *obuf; /* Write buffer */
    redisReader *reader; /* Protocol reader */

    enum redisConnectionType connection_type;
    struct timeval *connect_timeout;
    struct timeval *command_timeout;

    struct {
        char *host;
        char *source_addr;
        int port;
    } tcp;

    struct {
        char *path;
    } unix_sock;

    /* 非阻塞连接使用, non-blocking */
    struct sockaddr *saddr;
    size_t addrlen;

    /* Optional data and corresponding destructor users can use to provide
     * context to a given redisContext.  Not used by hiredis. */
    void *privdata;
    void (*free_privdata)(void *);

    /* Internal context pointer presently used by hiredis to manage
     * SSL connections. */
    void *privctx;

    /* An optional RESP3 PUSH handler */
    redisPushFn *push_cb;
} redisContext;


typedef struct {
    // 连接类型，也表明使用的终端成员字段
    int type;
    /* bit field of REDIS_OPT_xxx */
    int options;
    /* timeout value for connect operation. If NULL, no timeout is used */
    const struct timeval *connect_timeout;
    /* timeout value for commands. If NULL, no timeout is used.  This can be
     * updated at runtime with redisSetTimeout/redisAsyncSetTimeout. */
    const struct timeval *command_timeout;
    union {
        /* TCP/IP连接的相关字段参数 tcp/ip connections */
        struct {
            const char *source_addr;
            const char *ip;
            int port;
        } tcp;
        /** use this field for unix domain sockets */
        const char *unix_socket;
        /**
         * use this field to have hiredis operate an already-open
         * file descriptor */
        redisFD fd;
    } endpoint;

    /* Optional user defined data/destructor */
    void *privdata;
    void (*free_privdata)(void *);

    /* A user defined PUSH message callback */
    redisPushFn *push_cb;
    redisAsyncPushFn *async_push_cb;
} redisOptions;

#endif