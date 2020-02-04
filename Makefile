openssl_lib_dir = ../openssl

SRC = $(wildcard src/*.c)
SRC_OBJ = $(SRC:src/%.c=obj/%.o)

TEST_SRC = $(wildcard test/*.c)
TEST_BIN = $(TEST_SRC:test/%.c=bin/%)

all: lib obj bin lib/libdtorr.so $(TEST_BIN) 

bin/test_%: obj/test_%.o bin/libdtorr.so
	gcc $(ADD_FLAGS) -lm -o $@ $< -L./lib -l:libdtorr.so

bin/libdtorr.so: lib/libdtorr.so
	cp lib/libdtorr.so bin/libdtorr.so

obj/%.o: test/%.c
	gcc $(ADD_FLAGS) -Wall -O -c $< -Iinclude -Isrc -o $@

obj/%.o: src/%.c
	gcc $(ADD_FLAGS) -Wall -O -c $< -Iinclude -I$(openssl_lib_dir)/include -o $@

lib/libdtorr.so: $(SRC_OBJ)
	gcc $(ADD_FLAGS) -shared -o ./lib/libdtorr.so $(SRC_OBJ) -L$(openssl_lib_dir)/lib -l:libcrypto.a -l:libssl.a -lws2_32 -mwindows

lib:
	mkdir lib

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* lib/* bin/*
