all: lib obj bin lib/libdtorr.so bin/test_bencoding

bin/test_%: obj/test_%.o bin/libdtorr.so
	gcc -lm -o $@ $< -L./lib -l:libdtorr.so

bin/libdtorr.so: lib/libdtorr.so
	cp lib/libdtorr.so bin/libdtorr.so

obj/%.o: test/%.c
	gcc -Wall -O -c $< -Iinclude -o $@

obj/%.o: src/%.c
	gcc -Wall -O -c $< -Iinclude -o $@

lib/libdtorr.so: obj/log.o obj/hashmap.o obj/dtorr.o obj/bencoding_decode.o \
	obj/bencoding_encode.o
	gcc -fvisibility=hidden -shared -o ./lib/libdtorr.so obj/log.o obj/hashmap.o obj/dtorr.o \
		obj/bencoding_decode.o obj/bencoding_encode.o

lib:
	mkdir lib

obj:
	mkdir obj

bin:
	mkdir bin

clean:
	rm -f obj/* lib/* bin/*
