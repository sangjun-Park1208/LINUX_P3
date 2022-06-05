all: ssu_sfinder

ssu_sfinder : ssu_sfinder.o
	gcc ssu_sfinder.o -o ssu_sfinder -lcrypto

ssu_sfinder.o : ssu_sfinder.c
	gcc -c ssu_sfinder.c
