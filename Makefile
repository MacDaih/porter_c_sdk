build:
	gcc -c porter_sdk.c packet.c tcp_client.c

install: build
	ar -rcs libsdk.a *.o
	sudo install -m 755 porter_sdk.h /usr/include
	sudo install -m 755 libsdk.a /usr/lib/

uninstall:
	sudo rm /usr/lib/libsdk.a
	sudo rm /usr/include/porter_sdk.h
