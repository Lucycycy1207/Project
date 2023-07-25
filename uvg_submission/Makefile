EXTRA_CXXFLAGS=-Wno-unused-variable
CXXFLAGS=-O3 -Wall -std=c++20 -I./eigen-3.4.0 $(EXTRA_CXXFLAGS)

all: uvg_compress uvg_decompress

uvg_compress: uvg_compress.o
	$(CXX) $(CXXFLAGS) -o $@ $^

uvg_decompress: uvg_decompress.o
	g++ -o $@ $^

clean:
	rm -f uvg_compress uvg_decompress *.o
