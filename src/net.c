int redisContextUpdateConnectTimeout(redisContext *c, const struct timeval *timeout) {
    /* Same timeval struct, short circuit */
    if (c->connect_timeout == timeout)
        return REDIS_OK;

    /* Allocate context timeval if we need to */
    if (c->connect_timeout == NULL) {
        c->connect_timeout = hi_malloc(sizeof(*c->connect_timeout));
        if (c->connect_timeout == NULL)
            return REDIS_ERR;
    }

    memcpy(c->connect_timeout, timeout, sizeof(*c->connect_timeout));
    return REDIS_OK;
}