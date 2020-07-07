CFLAGS = -g -Wall -O2

libhpack.a: hpack.o hpack_decode.o hpack_encode.o hpack_static.o hpack_dynamic.o huffman.o
	ar cr $@ $^

clean:
	rm -f *.o libhpack.a
