set(SOURCES
    args.c
    byteswap.c
    error.c
    hash.c
    parsarg.c
    stdlib.c
    strutil.c
)

add_library(misc STATIC ${SOURCES})
