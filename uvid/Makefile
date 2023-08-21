EXTRA_CXXFLAGS=-Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS=-O3 -Wall -std=c++20 -I./eigen-3.4.0 $(EXTRA_CXXFLAGS)

all: uvid_compress uvid_decompress

uvid_compress: uvid_compress.o
	$(CXX) $(CXXFLAGS) -o $@ $^

uvid_decompress: uvid_decompress.o
	g++ -o $@ $^


clean:
	rm -f uvid_compress uvid_decompress *.o
