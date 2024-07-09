#include "hiredis.h"

static redisContextFuncs redisContextDefaultFuncs = {
    .close = redisNetClose,
    .free_privctx = NULL,
    .async_read = redisAsyncRead,
    .async_write = redisAsyncWrite,
    .read = redisNetRead,
    .write = redisNetWrite
};

redisReader *redisReaderCreate(void) {
    return redisReaderCreateWithFunctions(&defaultFunctions);
}

void redisFree(redisContext *c) {
    if (c == NULL)
        return;

    if (c->funcs && c->funcs->close) {
        c->funcs->close(c);
    }

    sdsfree(c->obuf);
    redisReaderFree(c->reader);
    hi_free(c->tcp.host);
    hi_free(c->tcp.source_addr);
    hi_free(c->unix_sock.path);
    hi_free(c->connect_timeout);
    hi_free(c->command_timeout);
    hi_free(c->saddr);

    if (c->privdata && c->free_privdata)
        c->free_privdata(c->privdata);

    if (c->funcs && c->funcs->free_privctx)
        c->funcs->free_privctx(c->privctx);

    memset(c, 0xff, sizeof(*c));
    hi_free(c);
}

static redisContext *redisContextInit(void) {
    redisContext *c;

    c = hi_calloc(1, sizeof(*c));
    if (c == NULL)
        return NULL;

    c->funcs = &redisContextDefaultFuncs;

    c->obuf = sdsempty();
    c->reader = redisReaderCreate();
    c->fd = REDIS_INVALID_FD;

    if (c->obuf == NULL || c->reader == NULL) {
        redisFree(c);
        return NULL;
    }

    return c;
}

/* Set a user provided RESP3 PUSH handler and return any old one set. */
redisPushFn *redisSetPushCallback(redisContext *c, redisPushFn *fn) {
    redisPushFn *old = c->push_cb;
    c->push_cb = fn;
    return old;
}

redisContext *redisConnectWithOptions(const redisOptions *options) {
    redisContext *c = redisContextInit();
    if (c == NULL) {
        return NULL;
    }
    if (!(options->options & REDIS_OPT_NONBLOCK)) {
        c->flags |= REDIS_BLOCK;
    }
    if (options->options & REDIS_OPT_REUSEADDR) {
        c->flags |= REDIS_REUSEADDR;
    }
    if (options->options & REDIS_OPT_NOAUTOFREE) {
        c->flags |= REDIS_NO_AUTO_FREE;
    }
    if (options->options & REDIS_OPT_NOAUTOFREEREPLIES) {
        c->flags |= REDIS_NO_AUTO_FREE_REPLIES;
    }
    if (options->options & REDIS_OPT_PREFER_IPV4) {
        c->flags |= REDIS_PREFER_IPV4;
    }
    if (options->options & REDIS_OPT_PREFER_IPV6) {
        c->flags |= REDIS_PREFER_IPV6;
    }

    /* Set any user supplied RESP3 PUSH handler or use freeReplyObject
     * as a default unless specifically flagged that we don't want one. */
    if (options->push_cb != NULL)
        redisSetPushCallback(c, options->push_cb);
    else if (!(options->options & REDIS_OPT_NO_PUSH_AUTOFREE))
        redisSetPushCallback(c, redisPushAutoFree);

    c->privdata = options->privdata;
    c->free_privdata = options->free_privdata;

    if (redisContextUpdateConnectTimeout(c, options->connect_timeout) != REDIS_OK ||
        redisContextUpdateCommandTimeout(c, options->command_timeout) != REDIS_OK) {
        __redisSetError(c, REDIS_ERR_OOM, "Out of memory");
        return c;
    }

    if (options->type == REDIS_CONN_TCP) {
        redisContextConnectBindTcp(c, options->endpoint.tcp.ip,
                                   options->endpoint.tcp.port, options->connect_timeout,
                                   options->endpoint.tcp.source_addr);
    } else if (options->type == REDIS_CONN_UNIX) {
        redisContextConnectUnix(c, options->endpoint.unix_socket,
                                options->connect_timeout);
    } else if (options->type == REDIS_CONN_USERFD) {
        c->fd = options->endpoint.fd;
        c->flags |= REDIS_CONNECTED;
    } else {
        redisFree(c);
        return NULL;
    }

    if (c->err == 0 && c->fd != REDIS_INVALID_FD &&
        options->command_timeout != NULL && (c->flags & REDIS_BLOCK))
    {
        redisContextSetTimeout(c, *options->command_timeout);
    }

    return c;
}


/* Connect to a Redis instance. On error the field error in the returned
 * context will be set to the return value of the error function.
 * When no set of reply functions is given, the default set will be used. */
redisContext *redisConnect(const char *ip, int port) {
    redisOptions options = {0};
    REDIS_OPTIONS_SET_TCP(&options, ip, port);
    return redisConnectWithOptions(&options);
}