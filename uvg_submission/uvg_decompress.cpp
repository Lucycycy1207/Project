/* uvg_decompress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads a height/width value from the input file
    - Reads YCbCr data from the file, with the Y plane
      in full w x h resolution and the other two planes
      in half resolution.
    - Upscales the Cb and Cr planes to full resolution and
      transforms them to RGB.
    - Writes the result as a BMP image
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)

   B. Bird - 2023-07-03

   -The compressor and decompressor use a block-based, quantized DCT to represent
    the image data
    -the encoding scheme used for DCT coefficients achieves compression. For full marks,
    the compressed representation of an image must be smaller than its PNG represen-
    tation 
    -The three quality settings ‘low’, ‘medium’ and ‘high’ are supported by the compres-
    sor, and the file size is appreciably different between the three setting
    
    modified by Xinlu Chen V00907022 -2023-07-23
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include "input_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"
#include <Eigen/Dense>

Eigen::Matrix<float, 8, 8> DCT_matrix;
Eigen::Matrix<float, 8, 8> luminanceQuantizationTable;
Eigen::Matrix<float, 8, 8> chrominanceQuantizationTable;

std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> computeEncodedBlocks(unsigned int numBlocksRows, unsigned int numBlocksCols, InputBitStream& input_stream) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_blocks(numBlocksRows, std::vector<Eigen::Matrix<int, 1, 64>>(numBlocksCols));
    unsigned int sign;
    for (unsigned int x = 0; x < numBlocksRows; x++) {
        for (unsigned int y = 0; y < numBlocksCols; y++) {
            Eigen::Matrix<int, 1, 64> matrix;
            for (unsigned int a = 0; a < 64; a++) {
                //get sign first then the 1 byte data
                sign = static_cast<int>(input_stream.read_bit());
                if (sign == 0)//it pos
                    matrix(0, a) = static_cast<int>(input_stream.read_byte());
                else
                    matrix(0, a) = -static_cast<int>(input_stream.read_byte());
            }
            decoded_blocks[x][y] = matrix;
        }
    }

    return decoded_blocks;
}

std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> reverseDeltaEncode(const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& encoded) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded;

    for (const auto& row : encoded) {
        std::vector<Eigen::Matrix<int, 1, 64>> decodedRow;

        for (const auto& matrix : row) {
            Eigen::Matrix<int, 1, 64> decodedMatrix;
            decodedMatrix(0) = matrix(0); // First element remains unchanged

            for (int i = 1; i < 64; ++i) {
                decodedMatrix(i) = decodedMatrix(i - 1) + matrix(i); // Reverse delta encoding
            }

            decodedRow.push_back(decodedMatrix);
        }

        decoded.push_back(decodedRow);
    }

    return decoded;
}


Eigen::Matrix<float, 8, 8> reverseZigzagScan(const Eigen::Matrix<int, 1, 64>& zigzag) {
    const std::vector<int> zigzagIndices {
    0,  1,  8,  16, 9,  2,  3,  10,
    17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
    };
    Eigen::Matrix<float, 8, 8> quantizedDCT;

    int row = 0;
    int col = 0;
    int index = 0;

    while (index < 64) {
        row = zigzagIndices[index]/8;
        col = zigzagIndices[index]%8;
        quantizedDCT(row, col) = zigzag(index);
        index++;
    }
    return quantizedDCT;
}


Eigen::Matrix<float, 8, 8> reverseQuantization(
    const Eigen::Matrix<float, 8, 8>& quantizedDCT, 
    const Eigen::Matrix<float, 8, 8>& quantizationMatrix
) {
   
   Eigen::Matrix<float, 8, 8> transformedCoefficients = (quantizedDCT.array() * quantizationMatrix.array());
    
    return transformedCoefficients;
}

//quantized dct coefficient T
//get D' = C(DCT matrix) * Q(quantization vector)
//inverse DCT of D'
//rounding value get S'



Eigen::Matrix<int, 8, 8> reverseDCT(const Eigen::Matrix<float, 8, 8>& transformedCoefficients) {
    Eigen::Matrix<int, 8, 8> block;
    //block = transformedCoefficients * DCT_matrix.inverse();
    //DCT−1(D) = C(T)DC
    block = (DCT_matrix.transpose() * transformedCoefficients * DCT_matrix).array().round().cast<int>();
    return block;
}

Eigen::Matrix<int, 8, 8> reverseProcessBlock(
    const Eigen::Matrix<int, 1, 64>& decoded, 
    const Eigen::Matrix<float, 8, 8>& quantizationMatrix
) {
    Eigen::Matrix<float, 8, 8> quantizedDCT = reverseZigzagScan(decoded);
    //std::cout<< "after zigzag: " << std::endl;
    //printMatrix(quantizedDCT);
    Eigen::Matrix<float, 8, 8> transformedCoefficients = reverseQuantization(quantizedDCT, quantizationMatrix);
    //std::cout<< "after reverse quantization: " << std::endl;
    //printMatrix(transformedCoefficients);
    Eigen::Matrix<int, 8, 8> block = reverseDCT(transformedCoefficients);
    //std::cout<< "after reverse DCT: " << std::endl;
    //printMatrix(block);
    return block;
}

std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> reverseProcessMatrices(
    const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& zigzagCoefficients,
    const Eigen::Matrix<float, 8, 8>& quantizationTable
) {
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> reversedMatrices;

    for (const auto& row : zigzagCoefficients) {
        std::vector<Eigen::Matrix<int, 8, 8>> reversedRow;
        
        for (const auto& zigzag : row) {
            Eigen::Matrix<int, 8, 8> block = reverseProcessBlock(zigzag, quantizationTable);
         
            reversedRow.push_back(block);
        }
       
        reversedMatrices.push_back(reversedRow);
    }

    return reversedMatrices;
}



int main(int argc, char** argv){
    // Create an 8x8 matrix of doubles, DCT matrix
    DCT_matrix << 
    0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,
    0.4904,   0.4157,   0.2778,   0.0975,  -0.0975,  -0.2778,  -0.4157,  -0.4904,
    0.4619,   0.1913,  -0.1913,  -0.4619,  -0.4619,  -0.1913,   0.1913,   0.4619,
    0.4157,  -0.0975,  -0.4904,  -0.2778,   0.2778,   0.4904,   0.0975,  -0.4157,
    0.3536,  -0.3536,  -0.3536,   0.3536,   0.3536,  -0.3536,  -0.3536,   0.3536,
    0.2778,  -0.4904,   0.0975,   0.4157,  -0.4157,  -0.0975,   0.4904,  -0.2778,
    0.1913,  -0.4619,   0.4619,  -0.1913,  -0.1913,   0.4619,  -0.4619,   0.1913,
    0.0975,  -0.2778,   0.4157,  -0.4904,   0.4904,  -0.4157,   0.2778,  -0.0975;

    luminanceQuantizationTable <<
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99;
    
    chrominanceQuantizationTable <<
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99;


    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << " <input file> <output BMP>" << std::endl;
        return 1;
    }
    std::string input_filename {argv[1]};
    std::string output_filename {argv[2]};


    std::ifstream input_file{input_filename,std::ios::binary};
    InputBitStream input_stream {input_file};

    unsigned int height = input_stream.read_u32();
    unsigned int width = input_stream.read_u32();
    
    //type
    unsigned int quality = static_cast<unsigned int>(input_stream.read_byte());

    // get delta encoding data for Y
    size_t numElements_Y = ((height + 7) / 8) * ((width + 7) / 8) * 64;
    unsigned int numBlocksRows_Y = (height + 7) / 8; // Calculate the number of rows (rounded up)
    unsigned int numBlocksCols_Y = (width + 7) / 8; // Calculate the number of columns (rounded up)
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Y = computeEncodedBlocks(numBlocksRows_Y, numBlocksCols_Y, input_stream);
    
    // get delta encoding data for Cb
    size_t numElements_Cb = ((height / 2 + 7) / 8) * ((width / 2 + 7) / 8) * 64;
    unsigned int numBlocksRows_Cb = ((height + 1) / 2 + 7) / 8; // Calculate the number of rows (rounded up)
    unsigned int numBlocksCols_Cb = ((width + 1) / 2 + 7) / 8; // Calculate the number of columns (rounded up)
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cb = computeEncodedBlocks(numBlocksRows_Cb, numBlocksCols_Cb, input_stream);

    // get delta encoding data for Cr
    size_t numElements_Cr = ((height / 2 + 7) / 8) * ((width / 2 + 7) / 8) * 64;
    unsigned int numBlocksRows_Cr = ((height + 1) / 2 + 7) / 8; // Calculate the number of rows (rounded up)
    unsigned int numBlocksCols_Cr = ((width + 1) / 2 + 7) / 8; // Calculate the number of columns (rounded up)
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cr = computeEncodedBlocks(numBlocksRows_Cr, numBlocksCols_Cr, input_stream);
    //encoded has error toooo!! it seems that the negative value is not sloved,sth wrong with encoding of zigzag? No it is normal to have negative

    //no error above, enocded_Y2 is same as encoded_Y1
    //reverse delta encoding,zigzagCoefficients
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Y = reverseDeltaEncode(encoded_Y);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cb = reverseDeltaEncode(encoded_Cb);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cr = reverseDeltaEncode(encoded_Cr);
    //no error above, deocded_Y2 is same as decoded_Y1
    //error down here
    float scalarl;//larger, lower quality
    float scalarc;
    if (quality == 0){//0 for low, 1 for medium,2 for high
        scalarl = 1.2;
        scalarc = 1.5;
    }
    else if (quality == 1){
        scalarl = 1;
        scalarc = 1;
    }
    else{//high
        scalarl = 0.7;
        scalarc = 0.5;
    }
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Y = reverseProcessMatrices(decoded_Y,scalarl*luminanceQuantizationTable);
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Cb = reverseProcessMatrices(decoded_Cb,scalarc*chrominanceQuantizationTable);
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Cr = reverseProcessMatrices(decoded_Cr,scalarc*chrominanceQuantizationTable);
    
    


    
    auto Y = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    auto Cr_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    
    // Convert matrices back to unsigned char and store in Y, Cb_scaled, and Cr_scaled
    //this one don't have black color,it woooooorkkkkkkk!1!!!1
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            Y[y][x] = static_cast<unsigned char>(std::max(0, std::min(255, matrices_Y[y / 8][x / 8](y % 8, x % 8))));
        }
    }

    for (unsigned int y = 0; y < (height + 1) / 2; y++) {
        for (unsigned int x = 0; x < (width + 1) / 2; x++) {
            Cb_scaled[y][x] = static_cast<unsigned char>(std::max(0, std::min(255, matrices_Cb[y / 8][x / 8](y % 8, x % 8))));
            Cr_scaled[y][x] = static_cast<unsigned char>(std::max(0, std::min(255, matrices_Cr[y / 8][x / 8](y % 8, x % 8))));
        }
    }
    auto imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);
    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            imageYCbCr.at(y).at(x) = {
                Y.at(y).at(x),
                Cb_scaled.at(y/2).at(x/2),
                Cr_scaled.at(y/2).at(x/2)
            };
        }
    }

    
    input_stream.flush_to_byte();
    input_file.close();

    bitmap_image output_image {width,height};

    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto pixel_rgb = imageYCbCr.at(y).at(x).to_rgb();
            auto [r,g,b] = pixel_rgb;
            output_image.set_pixel(x,y,r,g,b);
        }
    }
    
    output_image.save_image(output_filename);
    
    return 0;
    
}