#include <stdlib.h>
#include <stdio.h>

#include "xhklib.h"


// Call this when "H" is pressed.
void print_hello(xhkEvent e, void *r1, void *r2, void *r3)
{
    printf("Hello\n");
}


int main()
{
    xhkConfig *hkconfig;
    hkconfig = xhkInit(NULL);

    xhkBindKey(hkconfig, 0, XK_H, 0, xhkKeyPress, &print_hello, 0, 0, 0);

    while (1) {
        xhkPollKeys(hkconfig, 1);
    }

    xhkClose(hkconfig);

    return 0;
}

    

