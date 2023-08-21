/* uvid_compress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4

   Reads video data from stdin in uncompresed YCbCr (YUV) format 
   (With 4:2:0 subsampling). To produce this format from 
   arbitrary video data in a popular format, use the ffmpeg
   tool and a command like 

     ffmpeg -i videofile.mp4 -f rawvideo -pixel_format yuv420p - 2>/dev/null | ./this_program <width> height>

   Note that since the width/height of each frame is not encoded into the raw
   video stream, those values must be provided to the program as arguments.

   B. Bird - 2023-07-08

   V00907022
   Xinlu Chen

   
*/
//support temporal compression (that is, P-frames).
//support motion compensation (at least at a basic level).
// The average bitrate of the compressed stream should be at most 1 bit per pixel (i.e. a com-
// pression ratio of 24 compared to a raw RGB representation, or a ratio of 12 compared to
// the input 4:2:0 data). As we have seen, this is the expected bitrate for a stream consisting
// entirely of I-frames, so it should not be di cult to improve on this requirement.

//
#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include "output_stream.hpp"
#include "yuv_stream.hpp"
#include "uvg_common.hpp"
#include <Eigen/Dense>
#include <cstdlib>
#include <vector>
#include <cmath>

//for use of determine whether to use motion vector in P frame if its value is too large
int customized_largest_motion = 100;
const u32 EOF_SYMBOL = 256;
std::vector<int> high_value_motion;//.clear()


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
            encodedMatrix(1) = matrix(1); // second element remains unchanged

            for (int i = 2; i < 64; ++i) {
                encodedMatrix(i) = matrix(i) - matrix(i - 1); // Delta encoding
            }

            encodedRow.push_back(encodedMatrix);
        }

        encoded.push_back(encodedRow);
    }

    return encoded;
}

//relative index compared with the original current microblock
std::pair<int, int> relative_index(std::string relative_pos){
    int relative_row = 0;
    int relative_col = 0;

    if (relative_pos == "OO"){//Origin
        return std::make_pair(relative_row, relative_col);
    }
    else if (relative_pos == "UL"){//upper left
        relative_row = -2;//row -1
        relative_col = -2;//col -1
    }
    else if(relative_pos == "UM"){//upper middle
        relative_row = -2;//row -1
        relative_col = 0;//col unchanged
    }
    else if (relative_pos == "UR"){//upper right
        relative_row = -2;//row -1
        relative_col = 1;//col +1
    }
    else if(relative_pos == "LM"){//left middle
        relative_row = 0;//row unchanged
        relative_col = -2;//col -1
    }
    else if(relative_pos == "RM"){//right middle
        relative_row = 0;//row unchanged
        relative_col = 2;//col +1
    }
    else if(relative_pos == "DL"){//down left
        relative_row = 2;//row +1
        relative_col = -2;//col -1
    }
    else if(relative_pos == "DM"){//down middle
        relative_row = 2;//row +1
        relative_col = 0;//col unchanged
    }
    else if(relative_pos == "DR"){//down right
        relative_row = 2;//row +1
        relative_col = 2;//col +1
    }
    return std::make_pair(relative_row, relative_col);
}

//block level
// Function to calculate the sum of absolute difference between two blocks
//sum of absolute differences (SAD)//
//CORRECT
float block_SAD(const Eigen::Matrix<float, 8, 8>& block1, const Eigen::Matrix<float, 8, 8>& block2) {
    float sum = 0.0;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            sum += std::abs(block1(i, j) - block2(i, j));
        }
    }

    return sum;
}

//calculate relative microblock AAD result
//the microblock may have smaller size
//if relative pos out of bound, ret -1
float Microblock_AAD(const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matrices,const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& prev_matrices,int mbRow,int mbCol,std::string relative_pos
                    ,int* relative_row, int* relative_col)
{
    int totalSum = 0;
    auto result = relative_index(relative_pos);
    //plus these values
    *relative_row = result.first;
    *relative_col = result.second;

    int real_mac_size = 0;
    float avgSAD;
    // calculate the SAD of the microblock on (mbRow+relative_row), (mbCol+relative_col)
    // std::cout << "===================" << std::endl;

    // std::cout << "relative_pos: " << relative_pos << std::endl;
    // std::cout << "matrices size: " << matrices.size() << "," << matrices[0].size() << std::endl;
    // std::cout << "mbRow: " << mbRow << std::endl;
    // std::cout << "mbCol: " << mbCol << std::endl;
    // std::cout << "relative_row: " << *relative_row << std::endl;
    // std::cout << "relative_col: " << *relative_col << std::endl;
    
    if ((0 <= mbRow+(*relative_row)) && (mbRow+(*relative_row) < static_cast<int>(matrices.size())) &&
        (0 <= mbCol+(*relative_col)) && (mbCol+(*relative_col) < static_cast<int>(matrices[0].size())))
    {
        for (int innerRow = 0; innerRow < 2; innerRow++){
            for (int innerCol = 0; innerCol < 2; innerCol++){
                //need to check if the index is in range
                
                    real_mac_size += 1;
                    const auto& block1 = matrices[mbRow+innerRow][mbCol+innerCol];
                    const auto& block2 = prev_matrices[mbRow+innerRow+(*relative_row)][mbCol+innerCol+(*relative_col)].cast<float>();
                    // std::cout << "curr matrice: " << std::endl;
                    // std::cout << matrices[mbRow][mbCol] << std::endl;
                    // std::cout << "prev matrice: " << std::endl;
                    // std::cout << prev_matrices[mbRow][mbCol] << std::endl;
                    //std::cout << "SAD: " << block_SAD(block1, block2) << std::endl;
                    totalSum += block_SAD(block1, block2);
                }
            }
    //totalSum, avgSAD correct
    // std::cout << "TOTAL: "  << totalSum << std::endl;
    // std::cout << "real_mac_size: "  << real_mac_size << std::endl;
    //the avgSAD should be float value, but totalSum and mac_size is int
    avgSAD = static_cast<float>(totalSum) / (8*8*real_mac_size);//1 macroblock have 4 blocks(may smaller), 1 block have 8*8 pixels
    // std::cout << "avgSAD: "  << avgSAD << std::endl;
    }
    else{
        avgSAD = -1;
    }
    // std::cout << "avgSAD: "  << avgSAD << std::endl;
    return avgSAD;
}

//single microblock level, do local search for every surrounding microblocks, return minAAD block value
std::pair<int, int> local_search(const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matrices,const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& prev_matrices,int mbRow, int mbCol)
{ 
    float avgSAD;
    float minSAD = std::numeric_limits<float>::max();
    int relative_row = 0;
    int relative_col = 0;
    int min_row = -1;
    int min_col = -1;
    
    // std::cout << "in local_search:" << std::endl;
    // std::cout << "matrices size:" << matrices.size() << "," << matrices[0].size() << std::endl;
    
    //original pos
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"OO", &relative_row, &relative_col);
    
    // Compare avgSAD and minSAD and store the minimum in minSAD
    //-1 means the relative pos's microblock out of bound
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    
    //surrounding pos
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"UL", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"UM", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"UR", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"LM", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"RM", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"DL", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"DM", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    avgSAD = Microblock_AAD(matrices,prev_matrices,mbRow,mbCol,"DR", &relative_row, &relative_col);
    if (avgSAD < minSAD && avgSAD != -1){
        min_row = mbRow + relative_row; min_col = mbCol + relative_col;
        minSAD = avgSAD;
    }
    //improvement could
    //if avgSAD is small enough, return motion vector(0,0), it is itself
    //else, compare microblock in surrounding 8 microblocks
    //CAUSION: the range of the microblock, especially on the side or corner
    // std::cout << "back relative_row: "  << relative_row << std::endl;
    // std::cout << "back relative_col: "  << relative_col << std::endl;
    // std::cout << "min_microblock_row: "  << min_row << std::endl;
    // std::cout << "min_microblock_col: "  << min_col << std::endl;

    return std::make_pair(min_row, min_col);
}


//calculate motion vectors using local search
//motion vector = index of min AAD microblock prev - index of current microblock
std::vector<std::vector<std::pair<int, int>>> motion_vectors_calculation(const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matrices, const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& prev_matrices) 
{
   
                 
    int numMacroblocksY = matrices.size() /2;
    int numMacroblocksX = matrices[0].size() /2;
    int totalSum;

    std::vector<std::vector<std::pair<int, int>>> motion_vectors; // Vector to store motion vectors
    // std::cout << "in motion_vectors_calculation:" << std::endl;
    // std::cout << "matrices size:" << matrices.size() << "," << matrices[0].size() << std::endl;
    // std::cout << "numMacroblocksY:" << numMacroblocksY << std::endl;
    // std::cout << "numMacroblocksX:" << numMacroblocksX << std::endl;
    // std::cout << "prev_matrices size:" << prev_matrices.size() << "," << prev_matrices[0].size() << std::endl;

    //the ith macroblock in 2*2 in frame
    //for every macroblock, do the local search
    for (int mbRow = 0; mbRow < static_cast<int>(matrices.size()); mbRow+=2) {
        std::vector<std::pair<int, int>> row_vectors; // Vector to store motion vectors for the current row
        for (int mbCol = 0; mbCol < static_cast<int>(matrices[0].size()); mbCol+=2) {
            totalSum = 0.0;
            //inside 2*2 macroblock loops
            //the min AAD macroblock's index
            auto index = local_search(matrices,prev_matrices,mbRow, mbCol);
            
            // std::cout << "origin:" << mbRow << "," << mbCol << std::endl;
            // std::cout << "index:" << index.first << "," << index.second << std::endl;

            //motion vector = index of min AAD microblock - index of current microblock
            //calculate motion vector
            int motion_row = index.first - mbRow;
            int motion_col = index.second - mbCol;
            //std::cout << "motion_vector:" << motion_row << "," << motion_col << std::endl;

            // Store the motion vector in the current row vector
            row_vectors.push_back(std::make_pair(motion_row, motion_col));
        }
        // Store the row vector in the motion_vectors 2D vector
        motion_vectors.push_back(row_vectors);
    }
    //std::cout << "motion_vectors:" << motion_vectors.size() << "," << motion_vectors[0].size() << std::endl;
    return motion_vectors;
}

//Generate P frame using motion vector
//motion vector = index of min AAD microblock - index of current microblock 
//predicted macroblock(i,j) = (previous macroblock(i+motionvectorx,j+motionvectory))

//if the predicted macroblock is large, store the origin data!!!!IMPORTANT
//here, all predicted, but return the macroblock index which are large(>100)IMPORTANT, stored in customized_largest_motion
//update the previous_matrices
std::pair<std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>,std::vector<std::pair<int, int>>>  motionCompensation(
    const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matrices,
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& prev_matrices,
    std::vector<std::vector<std::pair<int, int>>>& motion_vector
) {
    
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> motionCompensatedFrame;
    // std::cout << "in motionCompensation:" << std::endl;
    
    // std::cout << "motionvector size:" << motion_vector.size() << "," << motion_vector[0].size() << std::endl;
    // std::cout << "matrices size:" << static_cast<int>(matrices.size()) << "," << static_cast<int>(matrices[0].size()) << std::endl;
    std::vector<std::pair<int, int>> large_index_list;
    for (int Row = 0; Row < static_cast<int>(matrices.size()); Row++) {
        
        std::vector<Eigen::Matrix<float, 8, 8>> row_blocks;
        for (int Col = 0; Col < static_cast<int>(matrices[0].size()); Col++) {
            
            //1 macroblock have 1 motion vector
            // std::cout << "curr motion_vector at :" << Row/2 << "," << Col/2 << std::endl;
    
            int mv_Row = motion_vector[Row/2][Col/2].first;
            int mv_Col = motion_vector[Row/2][Col/2].second;
            // std::cout << "mv_Row: " << mv_Row << std::endl; 
            // std::cout << "mv_Col: " << mv_Col << std::endl; 
            // std::cout << "Row: " << Row << std::endl; 
            // std::cout << "Col: " << Col << std::endl; 
            //- motion vector
            //the diff betwen current and previous calculated pos
            Eigen::Matrix<float, 8, 8> motionCompensatedBlock = (matrices[Row][Col] - prev_matrices[Row+mv_Row][Col+mv_Col].cast<float>()).array().round().cast<float>();
            //update previous matrices
            //prev_matrices[Row][Col] = matrices[Row][Col].array().round().cast<int>();
            
            // Find the largest element in the matrix
            int largestElement = motionCompensatedBlock.cwiseAbs().maxCoeff();
            

            //cancel doing motion in large value's microblock
          

            if (largestElement > customized_largest_motion){
                //std::cout << "in matrices(" <<Row<<","<<Col << ")"<<std::endl;
                // std::cout << "in micro(" <<Row/2<<","<<Col/2<< ")"<<std::endl;
                // printMatrix(matrices[Row][Col]);
                // std::cout << "======="<<std::endl;
                // printMatrix(prev_matrices[Row+mv_Row][Col+mv_Col]);
                // std::cout << "result"<<std::endl;
                // printMatrix(motionCompensatedBlock);
                // exit(-1);
                //add the new microblock index into list
                if (!isPairInList(large_index_list,std::make_pair(Row/2, Col/2))){
                    //std::cout << "hahahaha"<<std::endl;
                    large_index_list.push_back(std::make_pair(Row/2, Col/2));
                    //update motion vector
                    motion_vector[Row/2][Col/2].first = 0;
                    motion_vector[Row/2][Col/2].second =0;
                }
            }
            //std::cout << "============" << std::endl;
            
            row_blocks.push_back(motionCompensatedBlock);
        }
        motionCompensatedFrame.push_back(row_blocks);
    }
    //std::cout << "end of motionCompensation:" << std::endl;
    return std::make_pair(motionCompensatedFrame, large_index_list);//contains list of macroblock_blocks, order from upper left to down right, row by row
}

void write_type(std::string quality,OutputBitStream output_stream){
    if (quality == "low")
        output_stream.push_bits(0, 2);
    else if (quality == "medium")
        output_stream.push_bits(1, 2);
    else
        output_stream.push_bits(2, 2);
}

//update P frame, for large microblock after compensation, use origin data for that microblock
std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> update_Pframe(
            std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame,
            std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices,
            std::vector<std::pair<int, int>> large_index_list){
                std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> new_P_frame = P_frame;
                // Print the elements of the vector
                for (const auto& pair : large_index_list) {
                    int x = pair.first;
                    int y = pair.second;
                    //top left in microblock
                    new_P_frame[x*2][y*2] = matrices[x*2][y*2];
                    //right has space
                    if (static_cast<int>(new_P_frame[0].size()) > (y*2+1)){
                        new_P_frame[x*2][y*2+1] = matrices[x*2][y*2+1];
                        //down right has space
                        if (static_cast<int>(new_P_frame.size()) > (x*2+1)){
                        new_P_frame[x*2+1][y*2+1] = matrices[x*2+1][y*2+1];
                        }
                    }
                    //down has space
                    if (static_cast<int>(new_P_frame.size()) > (x*2+1)){
                        new_P_frame[x*2+1][y*2] = matrices[x*2+1][y*2];
                    }
                }
                return new_P_frame;
}


void find_element_in_encoded(
std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded,
std::vector<int>& element_list)
{
    int targetValue;
    for (int Row = 0; Row < static_cast<int>(encoded.size()); Row++) {
        for (int Col = 0; Col < static_cast<int>(encoded[0].size()); Col++) {
            for (int i = 0; i < 64; ++i) {
                targetValue = encoded[Row][Col][i];
                auto it = std::find(element_list.begin(), element_list.end(), targetValue);
                if (it == element_list.end()) //not present in the list
                {
                    element_list.push_back(targetValue);
                }
            }
        }
    }
    std::sort(element_list.begin(), element_list.end());
}

std::vector<std::pair<int, int>> counting(
std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded,
std::vector<int>& element_list)
{
    int targetValue;
    std::vector<std::pair<int, int>> count_list;
    //initialize the count_list
    for (int x = 0; x < static_cast<int>(element_list.size()); x++) {
        count_list.emplace_back(element_list[x],0);

    }

    for (int Row = 0; Row < static_cast<int>(encoded.size()); Row++) {
        for (int Col = 0; Col < static_cast<int>(encoded[0].size()); Col++) {
            for (int i = 0; i < 64; ++i) {
                targetValue = encoded[Row][Col][i];
                int index = findIndex(element_list, targetValue);
                count_list[index].second += 1;
            }
        }
    }
    return count_list;
}

std::vector<std::pair<int, int>> runLengthEncode(const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& input) {
    std::vector<std::pair<int, int>> encoded;
    int current = input[0][0][0];
    int count = 0;
    int total = 0;
    for (int Row = 0; Row < static_cast<int>(input.size()); Row++) {
        for (int Col = 0; Col < static_cast<int>(input[0].size()); Col++) {
            for (int i = 0; i < 64; ++i) {
                if (input[Row][Col][i] == current) {
                    ++count;
                    if (count == 256){
                        encoded.emplace_back(current, count-1);
                        total += 255;
                        current = input[Row][Col][i];
                        count = 1;
                    }
                } else {
                    encoded.emplace_back(current, count);
                    total += count;
                    current = input[Row][Col][i];
                    count = 1;
                }
            }
        }
    }
    encoded.emplace_back(current, count);
    total += count;
    // std::cout << "size:"<<static_cast<int>(input.size())<<"x"<<static_cast<int>(input[0].size())<<std::endl;
    // std::cout << "count: " << total<<std::endl;
   
    return encoded;
}


void encode_rle(std::vector<std::pair<int, int>>& rle,OutputBitStream& output_stream){
    int size = static_cast<int>(rle.size());
    //format: number of size bit in uanry + rle_Y size in bits
    int bits_needed = static_cast<int>(std::log2(size)) + 1;
    for (int i=0; i<bits_needed;i++){
        output_stream.push_bit(1);
    }
    output_stream.push_bit(0);
    output_stream.push_bits(static_cast<int>(rle.size()),bits_needed);
    
    // std::cout << "bits_needed:"<<bits_needed << std::endl;
    // std::cout << "rle_size:"<<static_cast<int>(rle.size()) << std::endl;
    
    //in rle pair list
    for (int i=0; i<static_cast<int>(rle.size());i++){
        if (rle[i].first < 0){
            output_stream.push_bit(1);
        }
        else{
            output_stream.push_bit(0);
        }
        //output the first element
        bits_needed = 0;
        unsigned int value = abs(rle[i].first);
        while (value > 0) {
            bits_needed++;
            value >>= 1; // Right-shift by 1
            output_stream.push_bit(1);
        }
        output_stream.push_bit(0);
        output_stream.push_bits(abs(rle[i].first),bits_needed);

        if (rle[i].second < 0){
            output_stream.push_bit(1);
        }
        else{
            output_stream.push_bit(0);
        }
        bits_needed = 0;
        value = abs(rle[i].second);
        while (value > 0) {
            bits_needed++;
            value >>= 1; // Right-shift by 1
            output_stream.push_bit(1);
        }
        output_stream.push_bit(0);
        //output the second element
        output_stream.push_bits(abs(rle[i].second),bits_needed);
    }
}

int main(int argc, char** argv){
    //height,width,2-bit quality,1 bit flag before each frame
    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <width> <height> <low/medium/high>" << std::endl;
        return 1;
    }
    // Create an 8x8 matrix of doubles, DCT matrix
   
    u32 w = std::stoi(argv[1]);
    u32 h = std::stoi(argv[2]);
    std::string quality{argv[3]};

    YUVStreamReader reader {std::cin, w, h};
    OutputBitStream output_stream {std::cout};

    output_stream.push_u32(h);
    output_stream.push_u32(w);
    write_type(quality,output_stream);
    int height = h;
    int width = w;
    //Extract the Y plane into its own array 
    auto Y = create_2d_vector<unsigned char>(height,width);
    //Extract the Cb plane into its own array 
    auto Cb = create_2d_vector<unsigned char>(height/2,width/2);
    //Extract the Cr plane into its own array 
    auto Cr = create_2d_vector<unsigned char>(height/2,width/2);
    // //determine whether it is first frame or not
    int I_frame = 1;
    int count = 0;
    //previous encoded block
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Y;
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cb;
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cr;
    
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Y;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Cb;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Cr;
    
    //test
    int loop = 0;
    int frame_num = -1;
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Y;
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Cb;
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Cr;

    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> new_P_frame_Y;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> new_P_frame_Cb;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> new_P_frame_Cr;
    
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Y;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Cb;
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Cr;
    
    std::vector<std::vector<std::pair<int, int>>> motion_vector_Y;
    std::vector<std::vector<std::pair<int, int>>> motion_vector_Cb;
    std::vector<std::vector<std::pair<int, int>>> motion_vector_Cr;

    std::vector<std::pair<int, int>> large_index_list_Y;
    std::vector<std::pair<int, int>> large_index_list_Cb;
    std::vector<std::pair<int, int>> large_index_list_Cr;

    std::vector<int> frame_back_to_I;

    while (reader.read_next_frame()){
        //test
        frame_num +=1;
        output_stream.push_bit(1); //Use a one byte flag to indicate whether there is a frame here
        
        

        YUVFrame420& frame = reader.frame();
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                Y.at(y).at(x) = frame.Y(x,y);
        for (int y = 0; y < height/2; y++)
            for (int x = 0; x < width/2; x++)
                Cb.at(y).at(x) = frame.Cb(x,y);
        for (int y = 0; y < height/2; y++)
            for (int x = 0; x < width/2; x++)
                Cr.at(y).at(x) = frame.Cr(x,y);
        
        //4d, matrix block, pos_r,pos_c,matrix, for Y, Cb, Cr
        std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> matrices_Y = convertToMatrices(Y,height,width);
        std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>matrices_Cb = convertToMatrices(Cb,(height+1)/2,(width+1)/2);
        std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>matrices_Cr = convertToMatrices(Cr,(height+1)/2,(width+1)/2);
        
        
        
        
        
        //now i got the 2d matrixes of 8*8 matrixes
        //these should in a looppppppppp
        float scalarl;//larger, lower quality
        float scalarc;
        if (quality == "low"){//0 for low, 1 for medium,2 for high
            scalarl = 1.2;//1.2//2.5
            scalarc = 1.5;//1.5//2.5
        }
        else if (quality == "medium"){
            scalarl = 1;//1//2
            scalarc = 1;//1//2
        }
        else{//high
            scalarl = 0.7;
            scalarc = 0.5;
        }
        
        //0.8,0.5,0.5 work//0.7,0.5,0.5 work//1.2,1.5 ,1.5 work
        
        //+++++++++++++++++++++++++++++++++++++++
        if (I_frame != 1){// First frame is I frame, other all are P frame
           
            //NOTICED: P FRAME HERE
            // the value of each pixel in block: current - decompressed prev
            //get decompressed prev
            //////////////////////////////////////////////////////
            //reverse delta encoding,zigzagCoefficients
            std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Y = reverseDeltaEncode(encoded_Y);
            std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cb = reverseDeltaEncode(encoded_Cb);
            std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cr = reverseDeltaEncode(encoded_Cr);
            
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> temp_prev_matrices_Y = reverseProcessMatrices(decoded_Y,scalarl*luminanceQuantizationTable);
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> temp_prev_matrices_Cb = reverseProcessMatrices(decoded_Cb,scalarc*chrominanceQuantizationTable);
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> temp_prev_matrices_Cr = reverseProcessMatrices(decoded_Cr,scalarc*chrominanceQuantizationTable);
           
            //get previous P matrices: situation 1:first Pframe. 2.other P frame
            if (frame_num == 1  || isIntInList(frame_back_to_I,frame_num-1)){
                //std::cout << "first Pframe -> 1st_prev_matrices_Y_en.txt" << std::endl;
                prev_matrices_Y = temp_prev_matrices_Y;
                prev_matrices_Cb = temp_prev_matrices_Cb;
                prev_matrices_Cr = temp_prev_matrices_Cr;
            }
            else{
                prev_matrices_Y = reverse_motion(temp_prev_matrices_Y, prev_matrices_Y,motion_vector_Y,large_index_list_Y);
                prev_matrices_Cb = reverse_motion(temp_prev_matrices_Cb, prev_matrices_Cb,motion_vector_Cb,large_index_list_Cb);
                prev_matrices_Cr = reverse_motion(temp_prev_matrices_Cr, prev_matrices_Cr,motion_vector_Cr,large_index_list_Cr);
            }
            

            ////////////////////////////////////////////////////////
            //microblock consists 4 Y, and 1 Cb, 1 Cr. get 1 single motion vector
            motion_vector_Y = motion_vectors_calculation(matrices_Y, prev_matrices_Y);
            motion_vector_Cb = motion_vectors_calculation(matrices_Cb, prev_matrices_Cb);
            motion_vector_Cr = motion_vectors_calculation(matrices_Cr, prev_matrices_Cr);
            
            //NEXT STEP: check large_index list, if large, high motion, make it to I frame
            auto result = motionCompensation(matrices_Y, prev_matrices_Y, motion_vector_Y);
            std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Y = result.first;
            large_index_list_Y = result.second;
            new_P_frame_Y = update_Pframe(P_frame_Y,matrices_Y, large_index_list_Y);
            
            //large_list correct for 1 microblock situation
            
            result = motionCompensation(matrices_Cb, prev_matrices_Cb, motion_vector_Cb);
            std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Cb = result.first;
            large_index_list_Cb = result.second;
            new_P_frame_Cb = update_Pframe(P_frame_Cb,matrices_Cb, large_index_list_Cb);
 
            result = motionCompensation(matrices_Cr, prev_matrices_Cr, motion_vector_Cr);
            std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> P_frame_Cr = result.first;
            large_index_list_Cr = result.second;
            new_P_frame_Cr = update_Pframe(P_frame_Cr,matrices_Cr, large_index_list_Cr);
            //if large index list is larger than half of it size, make it to Iframe
            // size_t largest_Y = (matrices_Y.size() / 2) * (matrices_Y[0].size() / 2)/4;
            // size_t largest_Cb = (matrices_Cb.size() / 2) * (matrices_Cb[0].size() / 2)/4;
            // size_t largest_Cr = (matrices_Cr.size() / 2) * (matrices_Cr[0].size() / 2)/4;
               
            // if (large_index_list_Y.size() > largest_Y || 
            //     large_index_list_Cb.size() > largest_Cb || 
            //     large_index_list_Cr.size() > largest_Cr){
            //     frame_back_to_I.push_back(frame_num);
            //     I_frame = 1;
            // }
            //else{
                matrices_Y = new_P_frame_Y;
                matrices_Cb = new_P_frame_Cb;
                matrices_Cr = new_P_frame_Cr;
            //}
            //check in P_frame if any matrices value larger than |15|, if is, store the original one
            //index_list store the microblock that use original data
            //(index_list, new_P_frame_Y)
            // Update matrices_Y with the generated P_frame
        }

        //DCT,quantization,zigzag
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Y = processMatrices(matrices_Y, scalarl*luminanceQuantizationTable);
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Cb = processMatrices(matrices_Cb, scalarc*chrominanceQuantizationTable);
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> zigzagCoefficients_Cr = processMatrices(matrices_Cr, scalarc*chrominanceQuantizationTable);

        encoded_Y = deltaEncode(zigzagCoefficients_Y);
        encoded_Cb = deltaEncode(zigzagCoefficients_Cb);
        encoded_Cr = deltaEncode(zigzagCoefficients_Cr);
        
        std::vector<std::pair<int, int>> rle_Y = runLengthEncode(encoded_Y);
        std::vector<std::pair<int, int>> rle_Cb = runLengthEncode(encoded_Cb);
        std::vector<std::pair<int, int>> rle_Cr = runLengthEncode(encoded_Cr);
        //format: sign + bitnum in unary + times bits in unary + num + times
        //e.g. 4:   1 + 1110(100) + 10 + 100 + 1 total need 11 bits
        size_t numElements_Y = encoded_Y.size() * encoded_Y[0].size()*64;
        size_t numElements_Cb = encoded_Cb.size()* encoded_Cb[0].size()*64;
        size_t numElements_Cr = encoded_Cr.size()* encoded_Cr[0].size()*64;
        
        int motion_row = -1;
        int motion_col = -1;
        
        if (I_frame == 0){
            output_stream.push_bit(0);//it is Pframe
        }
        else{
            output_stream.push_bit(1);//it is Iframe
        }
        // Write the encoded Y values
        if (I_frame == 0){
            for (int Row = 0; Row < static_cast<int>(encoded_Y.size()); Row++) {
                for (int Col = 0; Col < static_cast<int>(encoded_Y[0].size()); Col++) {
                    //i got new_P_frame_Y,large_index_list_Y
                    //output motion vector, some microblock is origin
                    if (Row%2==0 && Col%2==0)//check if new motion vector should output
                    {
                        //list is the microblock index, so /2
                        //IMPORTANT! flag 1 means origin, 0 means predicted microblock
                        if (isPairInList(large_index_list_Y,std::make_pair(Row/2,Col/2))){
                            //no motion vector needed,send flag 1
                            output_stream.push_bit(1);
                        }
                        
                        else{
                            //need motion vector, send flag 0
                            output_stream.push_bit(0);
                            //4 blocks 1 macroblock has 1 motion vector
                            //format: sign + byte
                            motion_row = Row/2; motion_col = Col/2;
                            //first
                            if (motion_vector_Y[motion_row][motion_col].first < 0){
                                output_stream.push_bit(1);
                                }
                            else{
                                output_stream.push_bit(0);
                                }
                            output_stream.push_byte(abs(motion_vector_Y[motion_row][motion_col].first));
                            //second
                            if (motion_vector_Y[motion_row][motion_col].second < 0){
                                output_stream.push_bit(1);
                                }
                            else{
                                output_stream.push_bit(0);
                                }
                            output_stream.push_byte(abs(motion_vector_Y[motion_row][motion_col].second));
                        
                        }  
                    }
                }
            }
        }
        
        encode_rle(rle_Y,output_stream);

        // Write the encoded Cb values
        if (I_frame == 0){
            for (int Row = 0; Row < static_cast<int>(encoded_Cb.size()); Row++) {
                for (int Col = 0; Col < static_cast<int>(encoded_Cb[0].size()); Col++) {
                    //output motion vector
                    if (Row%2==0 && Col%2==0){//check if new motion vector should output
                        //4 blocks 1 macroblock has 1 motion vector
                        //format: sign + bCbte
                        motion_row = Row/2; motion_col = Col/2;
                        //IMPORTANT! flag 1 means origin, 0 means predicted microblock
                        if (isPairInList(large_index_list_Cb,std::make_pair(Row/2,Col/2))){
                            //no motion vector needed,send flag 1
                            output_stream.push_bit(1);
                        }
                        else{
                            //need motion vector, send flag 0
                            output_stream.push_bit(0);
                            //first
                            if (motion_vector_Cb[motion_row][motion_col].first < 0){
                                output_stream.push_bit(1);
                            }
                            else{
                                output_stream.push_bit(0);
                            }
                            output_stream.push_byte(abs(motion_vector_Cb[motion_row][motion_col].first));

                            //second
                            if (motion_vector_Cb[motion_row][motion_col].second < 0){
                                output_stream.push_bit(1);
                            }
                            else{
                                output_stream.push_bit(0);
                            }
                            output_stream.push_byte(abs(motion_vector_Cb[motion_row][motion_col].second));
                        }
                        
                    }
                }
            }
        }
        encode_rle(rle_Cb,output_stream);

        // Write the encoded Cr values
        if (I_frame == 0){
            for (int Row = 0; Row < static_cast<int>(encoded_Cr.size()); Row++) {
                for (int Col = 0; Col < static_cast<int>(encoded_Cr[0].size()); Col++) {
                    //output motion vector
                    if (Row%2==0 && Col%2==0){//check if new motion vector should output
                        //4 blocks 1 macroblock has 1 motion vector
                        //format: sign + bCrte
                        motion_row = Row/2; motion_col = Col/2;
                        //IMPORTANT! flag 1 means origin, 0 means predicted microblock
                        if (isPairInList(large_index_list_Cr,std::make_pair(Row/2,Col/2))){
                            //no motion vector needed,send flag 1
                            output_stream.push_bit(1);
                        }
                        else{
                            //need motion vector, send flag 0
                            output_stream.push_bit(0);
                            //first
                            if (motion_vector_Cr[motion_row][motion_col].first < 0){
                                output_stream.push_bit(1);
                            }
                            else{
                                output_stream.push_bit(0);
                            }
                            output_stream.push_byte(abs(motion_vector_Cr[motion_row][motion_col].first));

                            //second
                            if (motion_vector_Cr[motion_row][motion_col].second < 0){
                                output_stream.push_bit(1);
                            }
                            else{
                                output_stream.push_bit(0);
                            }
                            output_stream.push_byte(abs(motion_vector_Cr[motion_row][motion_col].second));
                        }
                        
                    }
                }
            }
        }
        encode_rle(rle_Cr,output_stream);
        
        I_frame = 0;
        
    }
    //std::cout << "frame_num: " << frame_num << std::endl;//299
    output_stream.push_byte(0); //Flag to indicate end of data
    output_stream.flush_to_byte();
    return 0; 
    //origin compressed.uvi has size 51,733,161 before compression
    //height *weight = 352*288 = 101,376 -> 12,672 bytes, total <=3,801,600, current=1,620,477
    
}
//need to get 0.76 to pass
//ffmpeg -f rawvideo -pixel_format yuv420p -framerate 30 -video_size 352x288 -i - -f yuv4mpegpipe output_video.y4m < output_video.raw