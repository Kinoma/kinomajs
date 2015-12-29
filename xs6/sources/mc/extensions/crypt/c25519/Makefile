# Curve25519 implementation for small-memory devices
# Daniel Beer <dlbeer@gmail.com>
#
# This file is in the public domain.

CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
HOST_CFLAGS = -O1 -Wall -ggdb -Isrc $(CFLAGS)
TESTS = \
    tests/f25519.test \
    tests/c25519.test \
    tests/ed25519.test \
    tests/morph25519.test \
    tests/fprime.test \
    tests/sha512.test \
    tests/edsign.test

all: $(TESTS)

test: $(TESTS)
	@@for x in $(TESTS); do echo $$x; ./$$x > /dev/null || exit 255; done

tests/f25519.test: src/f25519.o tests/test_f25519.o
	$(CC) -o $@ $^

tests/c25519.test: src/f25519.o src/c25519.o tests/test_c25519.o
	$(CC) -o $@ $^

tests/ed25519.test: src/f25519.o src/ed25519.o tests/test_ed25519.o
	$(CC) -o $@ $^

tests/morph25519.test: src/f25519.o src/c25519.o src/ed25519.o \
		src/morph25519.o tests/test_morph25519.o
	$(CC) -o $@ $^

tests/fprime.test: src/fprime.o tests/test_fprime.o
	$(CC) -o $@ $^

tests/sha512.test: src/sha512.o tests/test_sha512.o
	$(CC) -o $@ $^

tests/edsign.test: src/f25519.o src/ed25519.o src/fprime.o src/sha512.o \
		src/edsign.o tests/test_edsign.o
	$(CC) -o $@ $^

clean:
	rm -f */*.o
	rm -f */*.su
	rm -f tests/*.test

%.o: %.c
	$(CC) $(HOST_CFLAGS) -o $*.o -c $*.c
