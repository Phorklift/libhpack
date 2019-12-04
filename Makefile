CFLAGS = -g -Wall -O2 -I../../libwuya

libhpack.a: hpack.o hpack_decode.o hpack_encode.o hpack_static.o hpack_dynamic.o huffman.o
	ar cr $@ $^

clean:
	rm *.o libhpack.a
