#include <stdlib.h>
#include <stdio.h>

#include "xhklib.h"


int vquit;

void setquit(xhkEvent e, void *msg, void *r2, void *r3)
{
    printf("\nQuitting: %s\n", (char *) msg);
    vquit = 1;
}

void testPress(xhkEvent e, void *r1, void *r2, void *r3)
{
    printf("\n--pressed key: %s\n\n", xhkModsKeyToString(e.modifiers, e.keysym));
    if (e.event_mask & xhkKeyRepeat)
        printf("\b  (Repeat)\n");
    xhkPrintEvent(e);
    printf("\n");
}

void testRelease(xhkEvent e, void *r1, void *r2, void *r3)
{
    printf("\n--released key: %i, %i\n\n", e.keycode, e.modifiers);
    if (e.event_mask & xhkKeyRepeat)
        printf("\b  (Repeat)\n\n");
    xhkPrintEvent(e);
    printf("\n");
}

void testPressRelease(xhkEvent e, void *r1, void *r2, void *r3)
{
    printf("\n--pressed or released key: %i, %i\n\n", e.keycode, e.modifiers);
    if (e.event_mask & xhkKeyRepeat)
        printf("\b  (Repeat)\n");
    xhkPrintEvent(e);
    printf("\n");
}

void unbind_K(xhkEvent e, void *r1, void *config, void *r3)
{
    xhkConfig *hkconfig = (xhkConfig *) config;
    printf("\n--Unbinding 'K' (%i, %i)\n\n", e.keycode, e.modifiers);
    xhkUnBindKey(hkconfig, 0, XK_K, ControlMask | ShiftMask, xhkKeyPress);
    printf("\n\n");
}


int main()
{
    vquit = 0;

    xhkConfig hkconfig;
    xhkInit(&hkconfig);

    // On Debian Linux, the list of XK_* keys are in:
    //   /usr/include/X11/keysym.h
    //   /usr/include/X11/keysymdef.h
    // Modifier masks in /usr/include/X11/X.h
    //
    // This is also useful for finding which keys on a keyboard are mapped to:
    //   xbindkeys -mk

    xhkBindKey(&hkconfig, 0, XK_P, 0, xhkKeyPress|xhkKeyRelease, &testPressRelease, 0, 0, 0);

    xhkBindKey(&hkconfig, 0, XK_K, ControlMask | ShiftMask, xhkKeyPress, &testPress, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_K, ControlMask | ShiftMask, xhkKeyPress, &testPress, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_L, ControlMask | ShiftMask, xhkKeyPress, &testPress, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_M, ControlMask | ShiftMask, xhkKeyRelease, &testRelease, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_N, ControlMask | ShiftMask, xhkKeyPress | xhkKeyRelease, &testPressRelease, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, 1234, ControlMask | ShiftMask, xhkKeyPress, &testPress, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_R, 0, xhkKeyPress | xhkKeyRelease | xhkKeyRepeat, &testPressRelease, 0, 0, 0);
    
    xhkBindKey(&hkconfig, 0, XK_O, ControlMask | ShiftMask, xhkKeyPress | xhkKeyRepeat, &testPress, 0, 0, 0);
    xhkBindKey(&hkconfig, 0, XK_O, ControlMask | ShiftMask, xhkKeyRelease | xhkKeyRepeat, &testRelease, 0, 0, 0);

    xhkSetRepeatThreshold(&hkconfig, 10);

    xhkBindKey(&hkconfig, 0, XK_Q, 0, 0, &setquit, "quitting now...", 0, 0);
    xhkBindKey(&hkconfig, 0, XK_U, 0, 0, &unbind_K, 0, (void *) &hkconfig, 0);

    printf("xhk text: X11 fd: %x\n", (unsigned int) xhkGetfd(&hkconfig));

    // This loop waits on X11 keyboard events.
    // If you want to continue running other code while capturing the hotkeys
    // in the background, you'll have to run xhkPollKeys in a seperate thread.
    while (1) {
        xhkPollKeys(&hkconfig, 1);
        if (vquit == 1)
            break;
    }

    xhkClose(&hkconfig);

    return 0;
}

    

