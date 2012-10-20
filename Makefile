all:
	gcc -DDEBUG -Wall -o xhklib_test xhklib_test.c -I.. ../xhklib.c -lX11
	gcc -Wall -o xbindwin xbindwin.c -I.. ../xhklib.c -lX11

clean:
	rm -f xhklib_test
	rm -f xbindwin
