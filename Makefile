openssl_lib_dir = ../openssl

SRC = $(wildcard src/*.c)
SRC_OBJ = $(SRC:src/%.c=obj/%.o)

TEST_SRC = $(wildcard test/*.c)
TEST_BIN = $(TEST_SRC:test/%.c=bin/%)

ifeq ($(OS),Windows_NT)
	LIB_NAME = dtorr.dll
else
	LIB_NAME = libdtorr.so
endif

all: lib obj bin lib/$(LIB_NAME) $(TEST_BIN) 

bin/test_%: obj/test_%.o bin/$(LIB_NAME)
	gcc $(ADD_FLAGS) -lm -o $@ $< -L./lib -l:$(LIB_NAME)

bin/$(LIB_NAME): lib/$(LIB_NAME)
	cp lib/$(LIB_NAME) bin/$(LIB_NAME)

obj/%.o: test/%.c
	gcc $(ADD_FLAGS) -Wall -O -c $< -Iinclude -Isrc -o $@

obj/%.o: src/%.c
	gcc $(ADD_FLAGS) -Wall -O -c $< -Iinclude -I$(openssl_lib_dir)/include -o $@

lib/$(LIB_NAME): $(SRC_OBJ)
	gcc $(ADD_FLAGS) -shared -o ./lib/$(LIB_NAME) $(SRC_OBJ) -L$(openssl_lib_dir)/lib -l:libcrypto.a -l:libssl.a -lws2_32 -mwindows

lib:
	mkdir lib

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* lib/* bin/*
