#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "xhklib.h"

Window get_active_window(Display *disp);
int activate_window (Display *disp, Window win, int switch_desktop);
char * get_property (Display *disp, Window win,
        Atom xa_prop_type, char *prop_name, unsigned long *size);
int client_msg(Display *disp, Window win, char *msg,
        unsigned long data0, unsigned long data1,
        unsigned long data2, unsigned long data3,
        unsigned long data4);


// User definable bind keys.
                                  // Bind key             // Set focus
const int bindkeys[][2][2] = {{ { ControlMask, XK_1 }, { Mod1Mask, XK_1 } },
                              { { ControlMask, XK_2 }, { Mod1Mask, XK_2 } },
                              { { ControlMask, XK_3 }, { Mod1Mask, XK_3 } },
                              { { ControlMask, XK_4 }, { Mod1Mask, XK_4 } },
                              { { ControlMask, XK_5 }, { Mod1Mask, XK_5 } },
                              { { ControlMask, XK_6 }, { Mod1Mask, XK_6 } },
                              { { ControlMask, XK_7 }, { Mod1Mask, XK_7 } },
                              { { ControlMask, XK_8 }, { Mod1Mask, XK_8 } }};

// END user definable keys


#define LEN_BINDKEYS (sizeof(bindkeys)/(4*sizeof(int)))

Window get_active_window(Display *disp);
int activate_window (Display *disp, Window win, int switch_desktop);

Window windows[LEN_BINDKEYS];


int XSilentErrorHandler(Display *display, XErrorEvent *event)
{
    return 0;
}


void focuswin(xhkEvent e, void *dispptr, void *winid, void *r3)
{
    Display *display = (Display *) dispptr;
    int win_id = (int) winid;
    printf("focuswin(): %i, key: %s, raising window %p\n", win_id,
            xhkModsKeyToString(e.modifiers, e.keysym), (void *) windows[win_id]);

    // Passivly ignore any undefined BadWindow X errors
    XSync(display, 0);
    XSetErrorHandler(&XSilentErrorHandler);

    activate_window(display, windows[win_id], 0);

    XSync(display, 0);
    XSetErrorHandler(NULL);
    return;
}


void bindwin(xhkEvent e, void *dispptr, void *winid, void *r3)
{
    Display *display = (Display *) dispptr;
    int win_id = (int) winid;
    windows[win_id] = get_active_window(display);
    printf("bindwin(): %i key: %s, binding to window %p\n", win_id,
            xhkModsKeyToString(e.modifiers, e.keysym), (void *) windows[win_id]);
    return;
}


int main()
{
    int k;

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "xhkInit(): Error, unable to open X display: %s\n",
                XDisplayName(NULL));
        return -1;
    }

    xhkConfig hkconfig;
    xhkInit(&hkconfig);

    for(k=0; k<LEN_BINDKEYS; k++) {
        xhkBindKey(&hkconfig, 0, bindkeys[k][0][1], bindkeys[k][0][0], 0, &bindwin, display, (void *) k, 0);
        xhkBindKey(&hkconfig, 0, bindkeys[k][1][1], bindkeys[k][1][0], 0, &focuswin, display, (void *) k, 0);
    }

    while(1)
        xhkPollKeys(&hkconfig, 1);

    xhkClose(&hkconfig);
    XCloseDisplay(display);
    return 0;
}


    // ***** wmctrl funcs *****

// get_active_window(), activate_window(), get_property(), client_msg()
// courtesy of wmctrl 1.0.7 (GPLv2) 
//             http://tomas.styblo.name/wmctrl/

Window get_active_window(Display *disp)
{
    char *prop;
    unsigned long size;
    Window ret = (Window)0;

    prop = get_property(disp, DefaultRootWindow(disp), XA_WINDOW, "_NET_ACTIVE_WINDOW", &size);
    if (prop) {
        ret = *((Window*)prop);
        free(prop);
    }

    return(ret);
}


int activate_window (Display *disp, Window win, int switch_desktop) {
    unsigned long *desktop;

    if ((desktop = (unsigned long *)get_property(disp, win, XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        if ((desktop = (unsigned long *)get_property(disp, win, XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
            printf("Cannot find desktop ID of the window.\n");
        }
    }

    if (switch_desktop && desktop) {
        if (client_msg(disp, DefaultRootWindow(disp), "_NET_CURRENT_DESKTOP", *desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
            printf("Cannot switch desktop.\n");
        }
        free(desktop);
    }

    client_msg(disp, win, "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    return EXIT_SUCCESS;
}


char * get_property (Display *disp, Window win,
        Atom xa_prop_type, char *prop_name, unsigned long *size)
{
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    char *ret;
    const int MAX_PROPERTY_VALUE_LEN = 4096;

    xa_prop_name = XInternAtom(disp, prop_name, False);

    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,
            &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
        printf("Cannot get %s property.\n", prop_name);
        return NULL;
    }

    tmp_size = (ret_format / 8) * ret_nitems;
    if(ret_format==32) tmp_size *= sizeof(long)/4;
    ret = malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}


int client_msg(Display *disp, Window win, char *msg,
        unsigned long data0, unsigned long data1,
        unsigned long data2, unsigned long data3,
        unsigned long data4)
{
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;

    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}



