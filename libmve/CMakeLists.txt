set(SOURCES
    decoder16.c
    decoder8.c
    mveaudio.c
    mvelib.c
    mveplay.c
)

add_library(libmve STATIC ${SOURCES})
