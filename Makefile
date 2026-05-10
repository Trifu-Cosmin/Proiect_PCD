# Descriere:
# Acest Makefile compileaza toate componentele proiectului:
# - analyzer: modulul de analiza cu libconfig si libclang
# - main: launcher compatibil cu varianta initiala
# - server: server TCP
# - client: client normal
# - admin_client: client de administrare

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11
CLANG_INCLUDES = -I/usr/lib/llvm-18/include
CLANG_LIBS = -L/usr/lib/llvm-18/lib -lconfig -lclang

all: analyzer main server client admin_client

analyzer: src/analyzer.c
	$(CC) $(CFLAGS) src/analyzer.c -o analyzer $(CLANG_INCLUDES) $(CLANG_LIBS)

main: src/main.c analyzer
	$(CC) $(CFLAGS) src/main.c -o main

server: src/server.c analyzer
	$(CC) $(CFLAGS) src/server.c -o server

client: src/client.c
	$(CC) $(CFLAGS) src/client.c -o client

admin_client: src/admin_client.c
	$(CC) $(CFLAGS) src/admin_client.c -o admin_client

clean:
	rm -f analyzer main server client admin_client
	rm -rf uploads