/* uvg_compress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads an input image in BMP format
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)
    - Transforms the image from RGB to YCbCr (i.e. "YUV").
    - Downscales the Cb and Cr planes by a factor of two
      (producing the same resolution that would result
       from 4:2:0 subsampling, but using interpolation
       instead of ignoring some samples)
    - Writes each colour plane (Y, then Cb, then Cr)
      in 8 bits per sample to the output file.

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
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>
#include "output_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"
#include <Eigen/Dense>

Eigen::Matrix<float, 8, 8> DCT_matrix;
Eigen::Matrix<float, 8, 8> luminanceQuantizationTable;
Eigen::Matrix<float, 8, 8> chrominanceQuantizationTable;

//A simple downscaling algorithm using averaging.
std::vector<std::vector<unsigned char> > scale_down(std::vector<std::vector<unsigned char> > source_image, unsigned int source_width, unsigned int source_height, int factor){

    unsigned int scaled_height = (source_height+factor-1)/factor;
    unsigned int scaled_width = (source_width+factor-1)/factor;

    //Note that create_2d_vector automatically initializes the array to all-zero
    //row, column | -
    auto sums = create_2d_vector<unsigned int>(scaled_height,scaled_width);
    auto counts = create_2d_vector<unsigned int>(scaled_height,scaled_width);

    for(unsigned int y = 0; y < source_height; y++)
        for (unsigned int x = 0; x < source_width; x++){
            sums.at(y/factor).at(x/factor) += source_image.at(y).at(x);
            counts.at(y/factor).at(x/factor)++;
        }

    auto result = create_2d_vector<unsigned char>(scaled_height,scaled_width);
    for(unsigned int y = 0; y < scaled_height; y++)
        for (unsigned int x = 0; x < scaled_width; x++)
            result.at(y).at(x) = (unsigned char)((sums.at(y).at(x)+0.5)/counts.at(y).at(x));
    return result;
}
std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> convertToMatrices(const std::vector<std::vector<unsigned char>>& image_component, int height, int width) {
    int numBlocksRows = (height + 7) / 8; // Calculate the number of rows (rounded up)
    int numBlocksCols = (width + 7) / 8; // Calculate the number of columns (rounded up)
    // Create a 2D vector to store the matrices
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices(numBlocksRows, std::vector<Eigen::Matrix<float, 8, 8>>(numBlocksCols));

    // Iterate over each block (8x8 matrix)
    for (int blockRow = 0; blockRow < numBlocksRows; blockRow++) {
        for (int blockCol = 0; blockCol < numBlocksCols; blockCol++) {
            int startRow = blockRow * 8;
            int endRow = std::min(startRow + 8, static_cast<int>(height));

            int startCol = blockCol * 8;
            int endCol = std::min(startCol + 8, static_cast<int>(width));

            Eigen::Matrix<float, 8, 8> matrix;
            // Iterate over each element within the block
            for (int i = startRow; i < startRow + 8; i++) {
                for (int j = startCol; j < startCol + 8; j++) {
                    
                    // Check if the current position is within the image bounds
                    if (i < height && j < width) {
                        // Copy the image component value to the corresponding position in the matrix
                        matrix(i - startRow, j - startCol) = static_cast<float>(image_component[i][j]);
                    } else {
                        // Duplicate the last row or column if necessary
                        
                        int lastRow = std::min(i, static_cast<int>(height) - 1);
                        int lastCol = std::min(j, static_cast<int>(width) - 1);
                        matrix(i - startRow, j - startCol) = static_cast<float>(image_component[lastRow][lastCol]);
                    }
                }
            }

            // Store the matrix in the 2D vector
            matrices[blockRow][blockCol] = matrix;
        }
    }

    // Return the resulting matrices
    return matrices;
}

Eigen::Matrix<int, 1, 64> zigzagScan(const Eigen::Matrix<int, 8, 8>& matrix) {
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
    Eigen::Matrix<int, 1, 64> zigzagMatrix;

    for (int i = 0; i < 64; ++i) {
        int row = zigzagIndices[i] / 8;
        int col = zigzagIndices[i] % 8;
        zigzagMatrix(0, i) = matrix(row, col);
    }

    return zigzagMatrix;
}

// Function to perform DCT, quantization, and zigzag scan for a single block
//called by processMatrices
Eigen::Matrix<int, 1, 64> processBlock(const Eigen::Matrix<float, 8, 8>& block, const Eigen::Matrix<float, 8, 8>& quantizationMatrix) {
    
    // Apply DCT by performing matrix multiplication
    Eigen::Matrix<float, 8, 8> transformedCoefficients = DCT_matrix * block * DCT_matrix.transpose();
    // Perform quantization
    Eigen::Matrix<int, 8, 8> quantizedDCT = (transformedCoefficients.array() / quantizationMatrix.array()).array().round().cast<int>();
    // Check if the quantizedDCT matrix has any negative value
    
    // Perform zigzag scan
    Eigen::Matrix<int, 1, 64> zigzag = zigzagScan(quantizedDCT);

    return zigzag;
}

// Iterate over the given input matrix, consisting of 2D 8x8 matrices
//call processBlock
std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> processMatrices(
    const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matrices,
    const Eigen::Matrix<float, 8, 8>& quantizationTable
) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients;

    for (std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>::size_type row = 0; row < matrices.size(); ++row) {
        std::vector<Eigen::Matrix<int, 1, 64>> rowCoefficients;
        for (std::vector<Eigen::Matrix<float, 8, 8>>::size_type col = 0; col < matrices[row].size(); ++col) {
            const Eigen::Matrix<float, 8, 8>& currentBlock = matrices[row][col];
            const Eigen::Matrix<int, 1, 64> zigzag = processBlock(currentBlock, quantizationTable);
        
            rowCoefficients.push_back(zigzag);
        }
        zigzagCoefficients.push_back(rowCoefficients);
    }

    return zigzagCoefficients;
}

std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> deltaEncode(const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& input) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded;

    for (const auto& row : input) {
        std::vector<Eigen::Matrix<int, 1, 64>> encodedRow;

        for (const auto& matrix : row) {
            Eigen::Matrix<int, 1, 64> encodedMatrix;
            encodedMatrix(0) = matrix(0); // First element remains unchanged

            for (int i = 1; i < 64; ++i) {
                encodedMatrix(i) = matrix(i) - matrix(i - 1); // Delta encoding
            }

            encodedRow.push_back(encodedMatrix);
        }

        encoded.push_back(encodedRow);
    }

    return encoded;
}

void write_type(std::string quality,OutputBitStream output_stream){
    if (quality == "low")
        output_stream.push_bits(0, 2);
    else if (quality == "medium")
        output_stream.push_bits(1, 2);
    else
        output_stream.push_bits(2, 2);
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

    
    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <low/medium/high> <input BMP> <output file>" << std::endl;
        return 1;
    }
    std::string quality{argv[1]};
    std::string input_filename {argv[2]};
    std::string output_filename {argv[3]};

    bitmap_image input_image {input_filename};

    unsigned int height = input_image.height();
    unsigned int width = input_image.width();

    //Read the entire image into a 2d array of PixelRGB objects 
    //(Notice that height is the outer dimension, so the pixel at coordinates (x,y) 
    // must be accessed as imageRGB.at(y).at(x)).
    std::vector<std::vector<PixelYCbCr>> imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);


    for(unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto [r,g,b] = input_image.get_pixel(x,y);
            PixelRGB rgb_pixel {r,g,b};
            imageYCbCr.at(y).at(x) = rgb_pixel.to_ycbcr();            
        }
    }

    std::ofstream output_file{output_filename,std::ios::binary};
    OutputBitStream output_stream {output_file};

    //Placeholder: Use a simple bitstream containing the height/width (in 32 bits each)
    //followed by the entire set of values in each colour plane (in row major order).

    output_stream.push_u32(height);
    output_stream.push_u32(width);

   ///////////////////////////////////////////////////////////////
   
   //scale_down process given, Y unchanged, Cb,Cr change to 1/2
    //Extract the Y plane into its own array 
    auto Y = create_2d_vector<unsigned char>(height,width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Y.at(y).at(x) = imageYCbCr.at(y).at(x).Y;
    //Extract the Cb plane into its own array 
    auto Cb = create_2d_vector<unsigned char>(height,width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Cb.at(y).at(x) = imageYCbCr.at(y).at(x).Cb;
    auto Cb_scaled = scale_down(Cb,width,height,2);

    //Extract the Cr plane into its own array 
    auto Cr = create_2d_vector<unsigned char>(height,width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Cr.at(y).at(x) = imageYCbCr.at(y).at(x).Cr;
    auto Cr_scaled = scale_down(Cr,width,height,2);

    //4d, matrix block, pos_r,pos_c,matrix, for Y, Cb, Cr
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Y = convertToMatrices(Y,height,width);
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Cb = convertToMatrices(Cb_scaled,(height+1)/2,(width+1)/2);
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Cr = convertToMatrices(Cr_scaled,(height+1)/2,(width+1)/2);
    

    //now i got the 2d matrixes of 8*8 matrixes
    //these should in a looppppppppp
    float scalarl;//larger, lower quality
    float scalarc;
    if (quality == "low"){//0 for low, 1 for medium,2 for high
        scalarl = 1.2;
        scalarc = 1.5;
    }
    else if (quality == "medium"){
        scalarl = 1;
        scalarc = 1;
    }
    else{//high
        scalarl = 0.7;
        scalarc = 0.5;
    }
    //0.8,0.5,0.5 work
    //0.7,0.5,0.5 work
    //1.2,1.5 ,1.5 work
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Y = processMatrices(matrices_Y, scalarl*luminanceQuantizationTable);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Cb = processMatrices(matrices_Cb, scalarc*chrominanceQuantizationTable);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Cr = processMatrices(matrices_Cr, scalarc*chrominanceQuantizationTable);
    
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Y = deltaEncode(zigzagCoefficients_Y);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cb = deltaEncode(zigzagCoefficients_Cb);
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cr = deltaEncode(zigzagCoefficients_Cr);
  
    size_t numElements_Y = encoded_Y.size() * encoded_Y[0].size()*64;
    size_t numElements_Cb = encoded_Cb.size()* encoded_Cb[0].size()*64;
    size_t numElements_Cr = encoded_Cr.size()* encoded_Cr[0].size()*64;

    //push 0 for low, 1 for medium, 2 for high, 2 bit value
    write_type(quality,output_stream);
    
    int size = 0;
    //causion: this may have negative value, 1 extra bit for sign, 0 for pos, 1 for negative 
    // Write the encoded Y values
    for (const auto& row : encoded_Y) {
        for (const auto& block : row) {
            for (int i = 0; i < block.size(); ++i) {
                if (block[i] < 0){
                    output_stream.push_bit(1);
                    output_stream.push_byte(-block[i]);
                }
                else{
                    output_stream.push_bit(0);
                    output_stream.push_byte(block[i]);
                }
                
            }
        }
    }
    std::cout << size<< std::endl;
    // Write the encoded Cb values
    for (const auto& row : encoded_Cb) {
        for (const auto& block : row) {
            for (int i = 0; i < block.size(); ++i) {
                if (block[i] < 0){
                    output_stream.push_bit(1);
                    output_stream.push_byte(-block[i]);
                }
                else{
                    output_stream.push_bit(0);
                    output_stream.push_byte(block[i]);
                }
            }
        }
    }

    // Write the encoded Cr values
    for (const auto& row : encoded_Cr) {
        for (const auto& block : row) {
            for (int i = 0; i < block.size(); ++i) {
                if (block[i] < 0){
                    output_stream.push_bit(1);
                    output_stream.push_byte(-block[i]);
                }
                else{
                    output_stream.push_bit(0);
                    output_stream.push_byte(block[i]);
                }
            }
        }
    }

    output_stream.flush_to_byte();
    output_file.close();

    return 0;
}

