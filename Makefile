openssl_lib_dir = ../openssl

SRC = $(wildcard src/*.c)
SRC_OBJ = $(SRC:src/%.c=obj/%.o)

TEST_SRC = $(wildcard test/*.c)
TEST_BIN = $(TEST_SRC:test/%.c=bin/%)

ifeq ($(OS),Windows_NT)
	LIB_NAME = dtorr.dll
	ADD_LIB_FLAGS = -lws2_32 -mwindows
	FADD_FLAGS = $(ADD_FLAGS)
else
	LIB_NAME = libdtorr.so
	FADD_FLAGS = $(ADD_FLAGS) -fPIC -Wl,-rpath='$$ORIGIN'
endif

all: lib obj bin lib/$(LIB_NAME) $(TEST_BIN) 

bin/test_%: obj/test_%.o bin/$(LIB_NAME)
	gcc $(FADD_FLAGS) -lm -o $@ $< -L./lib -l:$(LIB_NAME)

bin/$(LIB_NAME): lib/$(LIB_NAME)
	cp lib/$(LIB_NAME) bin/$(LIB_NAME)

obj/%.o: test/%.c
	gcc $(FADD_FLAGS) -Wall -O -c $< -Iinclude -Isrc -o $@

obj/%.o: src/%.c
	gcc $(FADD_FLAGS) -Wall -O -c $< -Iinclude -I$(openssl_lib_dir)/include -o $@

lib/$(LIB_NAME): $(SRC_OBJ)
	gcc $(FADD_FLAGS) -shared -o ./lib/$(LIB_NAME) $(SRC_OBJ) -L$(openssl_lib_dir)/lib -l:libcrypto.a -l:libssl.a $(ADD_LIB_FLAGS)

lib:
	mkdir lib

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* lib/* bin/*
