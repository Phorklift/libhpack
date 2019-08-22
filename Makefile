CFLAGS = -g -Wall -O2

libhpack.a: hpack.o hpack_dynamic.o hpack_decode.o hpack_encode.o huffman.o
	ar cr $@ $^

clean:
	rm *.o libhpack.a
