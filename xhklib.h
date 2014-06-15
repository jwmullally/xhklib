#ifndef __XHKLIB_H__
#define __XHKLIB_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

static const int xhkKeyPress =   (1<<0);
static const int xhkKeyRelease = (1<<1);
static const int xhkKeyRepeat =  (1<<2);

typedef struct {
    unsigned int numlock;
    unsigned int capslock;
    unsigned int scrolllock;
} xhkLockmasks;

typedef struct {
    KeySym keysym;
    int keycode;
    unsigned int modifiers;
    unsigned int event_mask;
    unsigned int locks;
    xhkLockmasks lmasks;
    XKeyEvent xkey;
} xhkEvent;

typedef struct xhkHotkey {
    KeySym keysym;
    int keycode;
    unsigned int modifiers;
    unsigned int event_mask;
    void (*func)(xhkEvent, void *, void *, void *);
    void *arg1;
    void *arg2;
    void *arg3;
    struct xhkHotkey *next;
} xhkHotkey;

typedef struct {
    Display *display;
    int close_display_on_exit;
    xhkHotkey *hklist;        // LL: First node is sentinel.
    xhkLockmasks lmasks;
    unsigned char last_key_state[256]; //KeyCode is unsigned char in range[8,255]
    int Xrepeat_detect;
    unsigned int repeat_threshold;
} xhkConfig;


xhkConfig * xhkInit(Display *xDisplay);
void xhkClose(xhkConfig *config);
void* xhkBindKey(xhkConfig *config, Window grab_window, KeySym keysym, 
        unsigned int modifiers, unsigned int event_mask,
        void (*func)(xhkEvent, void *, void *, void *), 
        void *arg1, void *arg2, void *arg3);
int xhkUnBindKey(xhkConfig *config, Window grab_window, KeySym keysym, 
        unsigned int modifiers, unsigned int event_mask);
int xhkUnBindKeyByHotkey(xhkConfig *config, Window grab_window, void* hotkey);
void xhkPollKeys(xhkConfig *config, int wait_block);
void xhkPrintEvent(xhkEvent event);
char * xhkKeySymToString(KeySym keysym);
char * xhkModifiersToString(unsigned int modifiers);
char * xhkModsKeyToString(unsigned int modifiers, KeySym keysym);
Display * xhkGetXDisplay(xhkConfig *config);
void xhkSetRepeatThreshold(xhkConfig *config, unsigned int repeat_threshold);

#endif  // __XHKLIB_H__
