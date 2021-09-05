add_subdirectory(ogles)

set(SOURCES
    bitblt.c
    bitmap.c
    box.c
    canvas.c
    circle.c
    disc.c
    effects.c
    font.c
    gpixel.c
    gr.c
    ibitblt.c
    lbitblt.c
    linear.c
    line.c
    macstubs.c
    palette.c
    pcx.c
    pixel.c
    poly.c
    rect.c
    rle.c
    scale.c
    scanline.c
)

add_library(2d STATIC ${SOURCES})
