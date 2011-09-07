#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "xhklib.h"

// get_offending_modifiers, *grab_key from xbindkeys: grab_key.c (GPLv2)

// TODO: Review this in the future:
// XkbSetDetectableAutoRepeat 
//     Review at future time.
//     This function is broken in some releases of Xorg.
//     This is the cleanest way to detect key repeats and should be 
//     used if possible. See bugs.freedesktop.org: 22515, 22194.
//     Fixed in Xorg ~ 2009-09-04.
//     Ubuntu 10.04: XOrg Release Date: 2009-2-25
//     Might take a while for some distros to catch up...
//
//     Although the current repeat_threshold method does help protect 
//     against spurious keyboard hardware events.

// The list of XK_* keys is in:
//   /usr/include/X11/keysym.h
//   /usr/include/X11/keysymdef.h
// Masks in /usr/include/X11/X.h
//
// This is also useful for finding which keys on a keyboard are mapped to:
//   xbindkeys -mk
//

static int global_last_X_error_code;

static xhkLockmasks get_offending_modifiers (Display * dpy);
static void grab_key(Display * dpy, Window grab_window, int keycode, 
        unsigned int modifiers, xhkLockmasks lmasks);
static void ungrab_key(Display * dpy, Window grab_window, int keycode, 
        unsigned int modifiers, xhkLockmasks lmasks);
static void grab_key_all_screens(Display * dpy, Window grab_window, 
        int keycode, unsigned int modifier, xhkLockmasks lmasks);
static void ungrab_key_all_screens(Display * dpy, Window grab_window, 
        int keycode, unsigned int modifier, xhkLockmasks lmasks);
static int XNonFatalErrorHandler(Display *display, XErrorEvent *event);
static int call_function(xhkConfig *config, XKeyEvent xkey, 
        unsigned int event_mask);


int xhkInit(xhkConfig *config)
{
    int i;
    config->display = XOpenDisplay(NULL);
    if (config->display == NULL) {
        fprintf(stderr, "xhkInit(): Error, unable to open X display: %s\n",
                XDisplayName(NULL));
        return -1;
    }
    config->hklist = malloc(sizeof(xhkHotkey));
    config->hklist->next = NULL;
    config->lmasks = get_offending_modifiers(config->display);
    config->repeat_threshold = 10; // Default to 10ms threshold period between 
                                   // registering key events as seperate key 
                                   // presses/releases.

    for(i=0; i<256; i++)
        config->last_key_state[i] = xhkKeyRelease;

// TODO: Maybe re-enable at later date. See comment at top of file.
#if 0
    XkbSetDetectableAutoRepeat(config->display, 1, &config->Xrepeat_detect);
    if (config->Xrepeat_detect == 0) {
        fprintf(stderr, 
                "hkcInit(): Warning, X server doesn't support key Autorepeat\n"
                "           detection. Falling back to not-so-reliable manual\n"
                "           repeat detection method.\n");
    }
#else
    config->Xrepeat_detect = 0;
#endif

    return 0;
}


void xhkClose(xhkConfig *config)
{
    xhkHotkey *h, *hn;
    h = config->hklist;
    while (h != NULL ) {
        hn = h;
        free(h);
        h = hn->next;
    }
    config->hklist = NULL;
    XCloseDisplay(config->display);
    config->display = NULL;
}


static xhkLockmasks get_offending_modifiers (Display * dpy)
{
    int i;
    XModifierKeymap *modmap;
    KeyCode nlock, slock;
    static int mask_table[8] = {
        ShiftMask, LockMask, ControlMask, Mod1Mask,
        Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
    };
    xhkLockmasks lmasks;
    lmasks.numlock = 0;
    lmasks.scrolllock = 0;
    lmasks.capslock = 0;

    nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
    slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);

    modmap = XGetModifierMapping (dpy);

    if (modmap != NULL && modmap->max_keypermod > 0)
    {
        for (i = 0; i < 8 * modmap->max_keypermod; i++)
        {
            if (modmap->modifiermap[i] == nlock && nlock != 0)
                lmasks.numlock = mask_table[i / modmap->max_keypermod];
            else if (modmap->modifiermap[i] == slock && slock != 0)
                lmasks.scrolllock = mask_table[i / modmap->max_keypermod];
        }
    }

    lmasks.capslock = LockMask;

    if (modmap)
        XFreeModifiermap (modmap);
    return lmasks;
}


static void grab_key(Display * dpy, Window grab_window, int keycode, 
        unsigned int modifiers, xhkLockmasks lmasks)
{
    if (grab_window == 0)
        grab_window = DefaultRootWindow(dpy);

    modifiers &= ~( lmasks.numlock | lmasks.capslock | lmasks.scrolllock );

    XGrabKey (dpy, keycode, modifiers, grab_window,
            False, GrabModeAsync, GrabModeAsync);

    if (modifiers == AnyModifier)
        return;

    if (lmasks.numlock)
        XGrabKey (dpy, keycode, modifiers | lmasks.numlock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.capslock)
        XGrabKey (dpy, keycode, modifiers | lmasks.capslock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.scrolllock)
        XGrabKey (dpy, keycode, modifiers | lmasks.scrolllock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.numlock && lmasks.capslock)
        XGrabKey (dpy, keycode, 
                modifiers | lmasks.numlock | lmasks.capslock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.numlock && lmasks.scrolllock)
        XGrabKey (dpy, keycode, 
                modifiers | lmasks.numlock | lmasks.scrolllock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.capslock && lmasks.scrolllock)
        XGrabKey (dpy, keycode, 
                modifiers | lmasks.capslock | lmasks.scrolllock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    if (lmasks.numlock && lmasks.capslock && lmasks.scrolllock)
        XGrabKey (dpy, keycode,
                modifiers | lmasks.numlock | lmasks.capslock |lmasks.scrolllock,
                grab_window, False, GrabModeAsync, GrabModeAsync);

    return;
}


static void ungrab_key(Display * dpy, Window grab_window, int keycode, 
        unsigned int modifiers, xhkLockmasks lmasks)
{
    if (grab_window == 0)
        grab_window = DefaultRootWindow(dpy);

    modifiers &= ~( lmasks.numlock | lmasks.capslock | lmasks.scrolllock );

    XUngrabKey (dpy, keycode, modifiers, grab_window);

    if (modifiers == AnyModifier)
        return;

    if (lmasks.numlock)
        XUngrabKey (dpy, keycode, modifiers | lmasks.numlock, grab_window);

    if (lmasks.capslock)
        XUngrabKey (dpy, keycode, modifiers | lmasks.capslock, grab_window);

    if (lmasks.scrolllock)
        XUngrabKey (dpy, keycode, modifiers | lmasks.scrolllock, grab_window);

    if (lmasks.numlock && lmasks.capslock)
        XUngrabKey (dpy, keycode, 
                modifiers | lmasks.numlock | lmasks.capslock, grab_window);

    if (lmasks.numlock && lmasks.scrolllock)
        XUngrabKey (dpy, keycode, 
                modifiers | lmasks.numlock | lmasks.scrolllock, grab_window);

    if (lmasks.capslock && lmasks.scrolllock)
        XUngrabKey (dpy, keycode, 
                modifiers | lmasks.capslock | lmasks.scrolllock, grab_window);

    if (lmasks.numlock && lmasks.capslock && lmasks.scrolllock)
        XUngrabKey (dpy, keycode,
                modifiers | lmasks.numlock | lmasks.capslock |lmasks.scrolllock,
                grab_window);

    return;
}


static void grab_key_all_screens(Display * dpy, Window grab_window, 
        int keycode, unsigned int modifier, xhkLockmasks lmasks)
{
    int screen;
    if (grab_window == 0) {
        for (screen = 0; screen < ScreenCount (dpy); screen++)
            grab_key(dpy, RootWindow (dpy, screen), keycode, modifier, lmasks);
    } else {
        grab_key(dpy, grab_window, keycode, modifier, lmasks);
    }
    return;
}


static void ungrab_key_all_screens(Display * dpy, Window grab_window, 
        int keycode, unsigned int modifier, xhkLockmasks lmasks)
{
    int screen;
    if (grab_window == 0) {
        for (screen = 0; screen < ScreenCount (dpy); screen++)
            ungrab_key(dpy, RootWindow (dpy, screen), keycode, modifier, lmasks);
    } else {
        ungrab_key(dpy, grab_window, keycode, modifier, lmasks);
    }
    return;
}


static int XNonFatalErrorHandler(Display *display, XErrorEvent *event) 
{
    // A (simple) non-fatal Xlib error handler to replace the default fatal one.
    // Pretty hacky...
    // Based on libx11: XlibInt.c: _XDefaultError() -> _XPrintDefaultError()
    // Try call the default handler to decode the error strings properly.
    // The Xlib _XDefaultError calls exit() unless 
    //      event->error_code == BadImplementation ...

    global_last_X_error_code = event->error_code;

#ifdef DEBUG
    const int kBUFSIZ = 2048;
    char buffer[kBUFSIZ];
    char mesg[kBUFSIZ];
    const char *mtype = "XlibMessage";
    int (*XDefaultError)(Display *, XErrorEvent *);

    XGetErrorText(display, event->error_code, buffer, kBUFSIZ);
    XGetErrorDatabaseText(display, mtype, "XError", "X Error", mesg, 
            kBUFSIZ);
    fprintf(stderr, "%s:  %s\n  (Ignore next line)\n  ", mesg, buffer);

    // Find and call the default Xlib error handler.
    event->error_code = BadImplementation;
    XSetErrorHandler(NULL);
    XDefaultError = XSetErrorHandler(XNonFatalErrorHandler);
    (*XDefaultError)(display, event);
    fprintf(stderr, "\n");
#endif

    return 0;
}



int xhkBindKey(xhkConfig *config, Window grab_window, KeySym keysym, 
        unsigned int modifiers, unsigned int event_mask,
        void (*func)(xhkEvent, void *, void *, void *), 
        void *arg1, void *arg2, void *arg3)
{
    int ret = 0;
    int keycode;
    xhkHotkey *h;

    keycode = XKeysymToKeycode(config->display, keysym);
    if (keycode == 0) {
        fprintf(stderr, 
            "xhkBindKey(): Error, unable to find keycode for keysym 0x%X\n",
                (unsigned int) keysym);
        return -1;
    }

    // Temporarily swap out Xlib fatal error handler for our own non-fatal
    // one to handle XGrabKey errors as warnings. (e.g. Duplicate binds).
    // As XGrabKey always returns 1, the Error handler is also the only way
    // we can tell if an XGrabKey call fails.
    XSync(config->display, 0);
    global_last_X_error_code = 0;
    XSetErrorHandler(&XNonFatalErrorHandler);
    grab_key_all_screens(config->display, grab_window, keycode, modifiers, 
            config->lmasks);
    XSync(config->display, 0);
    XSetErrorHandler(NULL);
    if (global_last_X_error_code != 0) {
        fprintf(stderr,
            "xhkBindKey(): Warning, unable to fully bind key..\n"
            "              ... already bound by another program?\n"
            "              mod: %s + key: '%s'\n",
            xhkModifiersToString(modifiers), XKeysymToString(keysym));
        ret = -1;
    }

    // Try search and redefine existing Hotkey. Add new one if none found.
    h = config->hklist;
    while (h->next != NULL 
            && !(h->next->modifiers == modifiers && h->next->keycode ==keycode
                 && h->next->event_mask == event_mask))
        h = h->next;
    if (h->next == NULL) {
        h->next = malloc(sizeof(xhkHotkey));
        h->next->next = NULL;
    } else {
        fprintf(stderr,
                "xhkBindKey(): Warning, changing existing binding\n"
                "              mod: %s + key: '%s'\n",
            xhkModifiersToString(modifiers), XKeysymToString(keysym));
    }

    h = h->next;

    h->keysym = keysym;
    h->keycode = keycode;
    h->modifiers = modifiers;
    h->event_mask = (event_mask != 0) ? event_mask : xhkKeyPress;
    h->func = func;
    h->arg1 = arg1;
    h->arg2 = arg2;
    h->arg3 = arg3;
    return ret;
}


int xhkUnBindKey(xhkConfig *config, Window grab_window, KeySym keysym, 
        unsigned int modifiers, unsigned int event_mask)
{
    int ret = 0;
    xhkHotkey *h, *hn;

    int keycode;
    keycode = XKeysymToKeycode(config->display, keysym);
    if (keycode == 0) {
        fprintf(stderr, 
            "xhkUnBindKey(): Error, unable to find keycode for keysym: 0x%x\n",
                (unsigned int) keysym);
        return -1;
    }

    XSync(config->display, 0);
    global_last_X_error_code = 0;
    XSetErrorHandler(&XNonFatalErrorHandler);
    ungrab_key_all_screens(config->display, grab_window, keycode, modifiers, 
            config->lmasks);
    XSync(config->display, 0);
    XSetErrorHandler(NULL);
    if (global_last_X_error_code != 0) {
        fprintf(stderr,
                "xhkUnBindKey(): X errors while unbinding\n"
                "                mod: %s + key: '%s'\n",
                xhkModifiersToString(modifiers), XKeysymToString(keysym));
        ret = -1;
    }

    h = config->hklist;
    while (h->next != NULL 
            && !(h->next->modifiers == modifiers && h->next->keycode ==keycode
                  && h->next->event_mask == event_mask))
        h = h->next;
    if (h->next == NULL) {
        fprintf(stderr,
                "xhkUnBindKey(): Key binding not found\n"
                "                mod: %s + key: '%s'\n",
                xhkModifiersToString(modifiers), XKeysymToString(keysym));
        return -1;
    }
    hn = h->next;
    h->next = h->next->next;
    free(hn);
    return ret;
}


static int call_function(xhkConfig *config, XKeyEvent xkey, 
        unsigned int event_mask)
{
    xhkEvent hevent;
    xhkHotkey *h;

    hevent.lmasks = config->lmasks;
    hevent.xkey = xkey;
    hevent.keycode = xkey.keycode;
    hevent.locks = xkey.state & ( config->lmasks.numlock
            | config->lmasks.capslock | config->lmasks.scrolllock );
    hevent.modifiers = xkey.state & ~(hevent.locks);
    hevent.event_mask = event_mask;

    // Can this function event matching be refactored any better? 
    // It probably as simple as its going to get.
    h = config->hklist;
    while (h != NULL &&
          !(h->modifiers == hevent.modifiers && h->keycode == hevent.keycode
           && (h->event_mask & (event_mask & (xhkKeyPress | xhkKeyRelease)))
           && !(!(h->event_mask & xhkKeyRepeat) && (event_mask & xhkKeyRepeat))))
        h = h->next;
    if (h != NULL) {
        hevent.keysym = h->keysym;
        (*h->func)(hevent, h->arg1, h->arg2, h->arg3);
        return 1;
    } else {
        return 0;
    }
}


// Process any pending key presses. Return when there
// are no more pending. To block and wait for at least one, set wait_block = 1.
void xhkPollKeys(xhkConfig *config, int wait_block)
{
    XEvent event, nev;
    int handled_one = 0;
    int matched;
    int repeat_flag;

    while (XPending(config->display) || (wait_block && !handled_one)) {

        XNextEvent(config->display, &event);   // Blocks when no events pending.

        repeat_flag = 0;

        // Reenable when XkbSetDetectableAutoRepeat is fixed.
#if 0
        if (config->Xrepeat_detect == 1) {
            if (event.type == KeyPress) {
                if (config->last_key_state[event.xkey.keycode] == xhkKeyPress)
                    repeat_flag = xhkKeyRepeat;
                config->last_key_state[event.xkey.keycode] = xhkKeyPress;
            }
            if (event.type == KeyRelease)
                config->last_key_state[event.xkey.keycode] = xhkKeyRelease;
        }
#endif

        // This is a bit of a hack. In the event that X can't detect
        // auto repeating keys itself, we try manually check whether the next
        // event is an automatic key repeat. 
        // Its a repeat when a KeyRelease is followed immedietly by a KeyPress.
        // (See: http://stackoverflow.com/questions/2100654)
        // One advantage is that using a small time difference threshold,
        // we can protect against flaky hardware reporting spurious KeyRelease
        // when the key is still held down, e.g. in my keyboard. User can 
        // change this with xhkChangeRepeatThreshold(k).
        //
        // Only repeat KeyPress events, like above.
        if (config->Xrepeat_detect == 0 && event.type == KeyRelease
                && XEventsQueued(config->display, QueuedAfterReading)) {
            XPeekEvent(config->display, &nev);
            if (nev.type == KeyPress 
                    && nev.xkey.time - event.xkey.time < config->repeat_threshold
                    && nev.xkey.keycode == event.xkey.keycode) {
                XNextEvent(config->display, &event);
                repeat_flag = xhkKeyRepeat;
            }
        }

        switch (event.type) {
            case KeyPress:
                matched = call_function(config, event.xkey, 
                                        xhkKeyPress | repeat_flag);
                if (matched == 1)
                    handled_one = 1;
                break;
            case KeyRelease:
                matched = call_function(config, event.xkey, 
                                        xhkKeyRelease | repeat_flag);
                if (matched == 1)
                    handled_one = 1;
                break;
            default:
                break;
        }
    }
    return;
}


void xhkPrintEvent(xhkEvent event)
{
    printf("xhkPrintEvent(): Event type: %s %s %s, time: %i ms\n"
           "                 mod: %s + key: '%s'\n",
           (event.event_mask & xhkKeyPress) ? "xhkKeyPress" : "",
           (event.event_mask & xhkKeyRelease) ? "xhkKeyRelease" : "",
           (event.event_mask & xhkKeyRepeat) ? "xhkKeyRepeat" : "",
           (int) event.xkey.time,
           xhkModifiersToString(event.modifiers), XKeysymToString(event.keysym));
    return;
}


char * xhkKeySymToString(KeySym keysym)
{
    static char strbuffer[11];             // max "0xFFFFFFFF" + '\0'
    if (XKeysymToString(keysym) == NULL) {
        snprintf(strbuffer, 11, "0x%x", (unsigned int) keysym);
        return strbuffer;
    }
    return XKeysymToString(keysym);
}


char * xhkModifiersToString(unsigned int modifiers)
{
    static char strbuffer[256];
    char tmpbuffer[14];             // max "0xFFFFFFFF | " + '\0'
    unsigned int mods;

    mods = modifiers;
    strbuffer[0] = '\0';

    if (mods == 0) {
        strncat(strbuffer, "0x0", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        return strbuffer;
    }
    if (mods & ShiftMask ) {
        strncat(strbuffer, "ShiftMask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~ShiftMask;
    }
    if (mods & LockMask ) {
        strncat(strbuffer, "LockMask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~LockMask;
    }
    if (mods & ControlMask ) {
        strncat(strbuffer, "ControlMask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~ControlMask;
    }
    if (mods & Mod1Mask ) {
        strncat(strbuffer, "Mod1Mask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~Mod1Mask;
    }
    if (mods & Mod2Mask ) {
        strncat(strbuffer, "Mod2Mask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~Mod2Mask;
    }
    if (mods & Mod3Mask ) {
        strncat(strbuffer, "Mod3Mask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~Mod3Mask;
    }
    if (mods & Mod4Mask ) {
        strncat(strbuffer, "Mod4Mask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~Mod4Mask;
    }
    if (mods & Mod5Mask ) {
        strncat(strbuffer, "Mod5Mask | ", 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        mods &= ~Mod5Mask;
    }
    if (mods != 0) {
        snprintf(tmpbuffer, sizeof(tmpbuffer), "0x%x | ", mods);
        strncat(strbuffer, tmpbuffer, 
                sizeof(strbuffer)-strlen(strbuffer)-1);
    }
    if (strlen(strbuffer) > 4)
        strbuffer[strlen(strbuffer)-3] = '\0';      // Chop extra " | "

    return strbuffer;
}


char * xhkModsKeyToString(unsigned int modifiers, KeySym keysym) 
{
    static char strbuffer[256];

    strbuffer[0] = '\0';
    if (modifiers != 0) {
        strncat(strbuffer, xhkModifiersToString(modifiers), 
                sizeof(strbuffer)-strlen(strbuffer)-1);
        strncat(strbuffer, " + ", sizeof(strbuffer)-strlen(strbuffer)-1);
    }
    strncat(strbuffer, xhkKeySymToString(keysym), 
            sizeof(strbuffer)-strlen(strbuffer)-1);

    return strbuffer;
}



int xhkGetfd(xhkConfig *config)
{
    return XConnectionNumber(config->display);
}


void xhkSetRepeatThreshold(xhkConfig *config, unsigned int repeat_threshold)
{
    config->repeat_threshold = repeat_threshold;
    return;
}


