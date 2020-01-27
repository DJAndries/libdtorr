openssl_lib_dir = ../openssl


all: lib obj bin lib/libdtorr.so bin/test_bencoding bin/test_metadata

bin/test_%: obj/test_%.o bin/libdtorr.so
	gcc -lm -o $@ $< -L./lib -l:libdtorr.so

bin/libdtorr.so: lib/libdtorr.so
	cp lib/libdtorr.so bin/libdtorr.so

obj/%.o: test/%.c
	gcc -Wall -O -c $< -Iinclude -o $@

obj/%.o: src/%.c
	gcc -Wall -O -c $< -Iinclude -I$(openssl_lib_dir)/include -o $@

lib/libdtorr.so: obj/log.o obj/hashmap.o obj/metadata.o obj/dtorr.o obj/bencoding_decode.o \
	obj/bencoding_encode.o
	gcc -shared -o ./lib/libdtorr.so obj/log.o obj/hashmap.o obj/metadata.o obj/dtorr.o \
		obj/bencoding_decode.o obj/bencoding_encode.o -L$(openssl_lib_dir)/lib -l:libcrypto.a -l:libssl.a

lib:
	mkdir lib

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* lib/* bin/*
