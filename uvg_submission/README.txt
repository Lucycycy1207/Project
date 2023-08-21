CSC485B A3
XINLU CHEN V00907022

This program is about image compression. It contains compressor and decompressor. Input is .bmp file.
It uses techniques: DCT, quantization, 4:2:0 scaling for Y,Cb,Cr,delta encoding.
files:
bitmap_image.hpp 
input_stream.hpp  
output_stream.hpp  
test_images     
eigen-3.4.0             
Makefile          
README.txt         
uvg_common.hpp  
uvg_compress.cpp  
uvg_decompress.cpp

command:
tar -zxf uvg.tar.gz
cd uvg
make
./uvg_compress low input_image.bmp compressed_image.uvg
./uvg_compress medium input_image.bmp compressed_image.uvg
./uvg_compress high input_image.bmp compressed_image.uvg
./uvg_decompress compressed_image.uvg decompressed_image.bmp

Functions:
-bitmap_image.hpp, input_stream.hpp, output_stream.hpp,test_images,uvg_common.hpp
are all given files.

-eigen-3.4.0 
    contains the c++ library which downloaded online.To use #include <Eigen/Dense>

-uvg_compress.cpp  
    scale_down():
        downscaling algorithm using average.
    convertToMatrices():
        convert image component to matrix.
    zigzagScan():
        doing zigzag on matrix.
    processBlock():
        Function to perform DCT, quantization, and zigzag scan for a single block
        called by processMatrices
    processMatrices():
        Iterate over the given input matrix, consisting of 2D 8x8 matrices
        Doing the loop over matrixes and call processMatrices
    deltaEncode():
        Using delta encoding for the data.
    write_type():
        write two bits value to the output file for quality setting. 
        0 for low,1 for medium, 2 for high.

-uvg_decompress.cpp
    computeEncodedBlocks():
        reading input blocks. Get sign and store them to matrix.
    reverseDeltaEncode():
        reverse delta encoding, get decoded data.
    reverseZigzagScan():
        reverse zigzag algorithm to get 8*8 matrixes from 1*64 matrixes.
    reverseQuantization():
        reverse quantization calculation.
    reverseDCT():
        reverse DCT calculations.
    reverseProcessBlock():
        reverse the processBlock function in uvg_compressed.cpp.
    reverseProcessMatrices():
        reverse the ProcessMatrices function in uvg_compressed.cpp.
