set(SOURCES
    barbox.c
    button.c
    checkbox.c
    file.c
    func.c
    gadget.c
    icon.c
    inputbox.c
    keypad.c
    keypress.c
    keytrap.c
    listbox.c
    menubar.c
    menu.c
    message.c
    mouse.c
    number.c
    popup.c
    radio.c
    scroll.c
    ui.c
    uidraw.c
    userbox.c
    window.c
)

add_library(ui STATIC ${SOURCES})
