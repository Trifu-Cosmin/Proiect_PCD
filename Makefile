# Descriere:
# Acest Makefile automatizeaza compilarea programului principal.
# Programul este compilat cu gcc si legat cu bibliotecile externe:
# - libconfig
# - libclang

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11
INCLUDES = -I/usr/lib/llvm-18/include
LIBS = -L/usr/lib/llvm-18/lib -lconfig -lclang

all: main

main: src/main.c
	$(CC) $(CFLAGS) src/main.c -o main $(INCLUDES) $(LIBS)

clean:
	rm -f main