
all:
	gcc -DDEBUG -Wall -o xhklib_test xhklib_test.c xhklib.c -lX11
