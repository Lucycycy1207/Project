/* uvid_decompress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4
   
   This placeholder code reads the (basically uncompressed) data produced by
   the uvid_compress starter code and outputs it in the uncompressed 
   YCbCr (YUV) format used for the sample video input files. To play the 
   the decompressed data stream directly, you can pipe the output of this
   program to the ffplay program, with a command like 

     ffplay -f rawvideo -pixel_format yuv420p -framerate 30 -video_size 352x288 - 2>/dev/null
   (where the resolution is explicitly given as an argument to ffplay).

   B. Bird - 2023-07-08
   
   V00907022
   Xinlu Chen
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include "input_stream.hpp"
#include "yuv_stream.hpp"
#include "uvg_common.hpp"
#include <Eigen/Dense>
#include <vector>

const u32 EOF_SYMBOL = 256;

bool has_motion(InputBitStream& input_stream){
    int flag = static_cast<int>(input_stream.read_bit());
    if (flag == 1){
        //does not have motion vector
        return 0;
    }
    else{
        return 1;
    }
}


std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> computeEncodedBlocks(unsigned int numBlocksRows, unsigned int numBlocksCols, InputBitStream& input_stream) {
    std::vector<std::pair<int, int>> rle;
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_blocks(numBlocksRows, std::vector<Eigen::Matrix<int, 1, 64>>(numBlocksCols));
    int total = 0;
    //get the rle
    int bits_needed = 0;
    while(1){
        int bit = static_cast<int>(input_stream.read_bit());
        if (bit == 1){
            bits_needed += 1;
        }
        else{
            // if (bits_needed != 0)
            break;
        }
    }

    int rle_size = static_cast<int>(input_stream.read_bits(bits_needed));
    // std::cout << "bits_needed:"<<bits_needed << std::endl;
    // std::cout << "rle_size:"<<rle_size << std::endl;
    int sign;
    for (int i=0; i<rle_size;i++){
        //first element
        sign = static_cast<int>(input_stream.read_bit());
        bits_needed = 0;
        while(1){
            int bit = static_cast<int>(input_stream.read_bit());
            if (bit == 1){
                bits_needed += 1;
            }
            else{
                break;
            }
        }
        int abs_first = input_stream.read_bits(bits_needed);
        int first;
        if (sign == 1){//negative
            first = -abs_first;
        }
        else{//positve
            first = abs_first;
        }
        //second element
        sign = static_cast<int>(input_stream.read_bit());
        bits_needed = 0;
        while(1){
            int bit = static_cast<int>(input_stream.read_bit());
            if (bit == 1){
                bits_needed += 1;
            }
            else{
                break;
            }
        }
        int abs_second = input_stream.read_bits(bits_needed);
        total += abs_second;
        int second;
        if (sign == 1){//negative
            second = -abs_second;
        }
        else{//positve
            second = abs_second;
        }
        rle.emplace_back(first,second);
        


    }
    //print1DPairsToFile(rle, "rle_ha.txt");
    // input_stream.read_bit();
    //std::cout << "next:" <<input_stream.read_bit()<<std::endl;
    
    //exit(-1);
    //get the decoded_blocks
    int pos = 0;
    int count; if (rle_size != 0) count = rle[pos].second;// else exit(-1);
    int value; if (rle_size != 0) value = rle[pos].first;// else exit(-1);
    
    int curr_row = 0;
    int curr_col = 0;
    int curr_index = 0;
    // std::cout <<numBlocksCols<<"x" << numBlocksRows<< std::endl;
    // std::cout <<"total: " << total << std::endl;
    Eigen::Matrix<int, 1, 64> matrix;
    for (int x = 0; x < rle_size; x++) {
        for (int y=0; y<rle[x].second; y++){
            //std::cout <<"x:" << x << std::endl;
            matrix(curr_index) = rle[x].first;

            curr_index += 1;
            if (curr_index == 64){
                //std::cout <<"fill:"<<curr_row <<","<< curr_col << std::endl;
                
                decoded_blocks[curr_row][curr_col] = matrix;
                curr_index = 0;
                curr_col += 1;
                if (curr_col == static_cast<int>(numBlocksCols)){
                    
                    curr_row +=1;
                    // if (curr_row == static_cast<int>(numBlocksRows)){
                    //     break;
                    // }
                    // else{
                    curr_col =0;
                    //}
                }
            }
        }
        // if (curr_row == static_cast<int>(numBlocksRows) && curr_col == static_cast<int>(numBlocksCols)){
        //     std::cout << "out of bound" << std::endl;
        //     break;
        // }
    }
    //std::cout <<"end" << std::endl;
    return decoded_blocks;
}

std::pair<std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>, std::vector<std::vector<std::pair<int, int>>>>
computeEncodedBlocks_motion(unsigned int numBlocksRows, unsigned int numBlocksCols, InputBitStream& input_stream, std::vector<std::pair<int, int>>& large_index_list) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_blocks(numBlocksRows, std::vector<Eigen::Matrix<int, 1, 64>>(numBlocksCols));
    std::vector<std::vector<std::pair<int, int>>> motion_vectors(numBlocksRows / 2, std::vector<std::pair<int, int>>(numBlocksCols / 2, {0, 0})); // Initialize with zero pairs
    //large index incorrect
    unsigned int sign;
    int motion_row = -1;
    int motion_col = -1;
    
    //ERROR, why motion vector has a pair is wrong, it is because one don't have motion
    for (unsigned int x = 0; x < numBlocksRows; x++) {
        for (unsigned int y = 0; y < numBlocksCols; y++) {
            //every micrblock, decide whether having motion
            if (x % 2 == 0 && y % 2 == 0) {
                //check the flag, if 1, encode as origin, if 0, encode as predicted with motion vectors
                if (has_motion(input_stream)){
                    
                    // Read motion vectors for every 2x2 macroblock
                    motion_row = x / 2;
                    motion_col = y / 2;
                    sign = static_cast<int>(input_stream.read_bit());
                    
                    if (sign == 0) // it's positive
                        motion_vectors[motion_row][motion_col].first = static_cast<int>(input_stream.read_byte());
                    else{
                        motion_vectors[motion_row][motion_col].first = -static_cast<int>(input_stream.read_byte());
                    }
                    sign = static_cast<int>(input_stream.read_bit());

                    if (sign == 0) // it's positive
                        motion_vectors[motion_row][motion_col].second = static_cast<int>(input_stream.read_byte());
                    else{
                        
                        motion_vectors[motion_row][motion_col].second = -static_cast<int>(input_stream.read_byte());
                    }
                
                }
                //for test
                else{
                    //put microblock in largeindexlist
                    large_index_list.push_back(std::make_pair(x/2, y/2));
                    //std::cout <<"no motion at microblock: " << x/2 << "," << y/2 << std::endl;
                    
                }
            }
            
        }
    }
    //writeMotionVectorToFile(motion_vectors, "motion_vectors_en.txt");
    decoded_blocks = computeEncodedBlocks(numBlocksRows,numBlocksCols,input_stream);
    
    //printLinearMatricesToFile(decoded_blocks, "decoded_blocks.txt"); 
    //exit(-1);
    return std::make_pair(decoded_blocks, motion_vectors);
}

std::vector<std::vector<unsigned char>> convertToImageComponent(const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& matrices) {
    int numBlocksRows = matrices.size();
    int numBlocksCols = matrices[0].size();
    int height = numBlocksRows * 8;
    int width = numBlocksCols * 8;

    std::vector<std::vector<unsigned char>> image_component(height, std::vector<unsigned char>(width));

    for (int blockRow = 0; blockRow < numBlocksRows; blockRow++) {
        for (int blockCol = 0; blockCol < numBlocksCols; blockCol++) {
            int startRow = blockRow * 8;
            int startCol = blockCol * 8;

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int imageRow = startRow + i;
                    int imageCol = startCol + j;

                    if (imageRow < height && imageCol < width) {
                        image_component[imageRow][imageCol] = static_cast<unsigned char>(matrices[blockRow][blockCol](i, j));
                    }
                }
            }
        }
    }

    return image_component;
}

int main(int argc, char** argv){

    //Note: This program must not take any command line arguments. (Anything
    //      it needs to know about the data must be encoded into the bitstream)
    
    InputBitStream input_stream {std::cin};

    u32 height {input_stream.read_u32()};
    u32 width {input_stream.read_u32()};
    //type
    unsigned int quality = static_cast<unsigned int>(input_stream.read_byte());
    //height,width,2-bit quality,1 bit flag before each frame
    YUVStreamWriter writer {std::cout, width, height};
    int I_frame = 1;
    
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Y;
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Cb;
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prev_matrices_Cr;
        
    //test
    int frame_num = -1;
    std::vector<int> frame_back_to_I;
    while (unsigned int hasFrame {input_stream.read_bit()}){//loop every frame
        
        std::vector<std::vector<std::pair<int, int>>> motion_vector_Y;
        std::vector<std::vector<std::pair<int, int>>> motion_vector_Cb;
        std::vector<std::vector<std::pair<int, int>>> motion_vector_Cr;
        std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Y;
        std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Cb;
        std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrices_Cr;
        //test
        frame_num += 1;
        
        //std::cout << "hasFrame: " << hasFrame << std::endl;
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Y;
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cb;
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> encoded_Cr;
       
        // get delta encoding data for Y
        size_t numElements_Y = ((height + 7) / 8) * ((width + 7) / 8) * 64;
        unsigned int numBlocksRows_Y = (height + 7) / 8; // Calculate the number of rows (rounded up)
        unsigned int numBlocksCols_Y = (width + 7) / 8; // Calculate the number of columns (rounded up)
        std::vector<std::pair<int, int>> large_index_list_Y;
        
        
        I_frame = static_cast<int>(input_stream.read_bit());
        
        if(I_frame == 1 && frame_num != 0){
            frame_back_to_I.push_back(frame_num);
        }
        
        if (I_frame == 0){//with motion vector
            //check the flag, 1 means origin microblock, 0 means predicted microblock
            //if flag = 1, doesn't have motion vector
            //must modified inside computeEncodedBlocks_motion, it includes multiple microblocks
            //i have function hasmotion to help
            auto result = computeEncodedBlocks_motion(numBlocksRows_Y, numBlocksCols_Y, input_stream,large_index_list_Y);
            encoded_Y = result.first;
            motion_vector_Y = result.second;
        }
        else{
            //origin version
            encoded_Y = computeEncodedBlocks(numBlocksRows_Y, numBlocksCols_Y, input_stream);
      
        }
        
        std::vector<std::pair<int, int>> large_index_list_Cb;
        // get delta encoding data for Cb
        size_t numElements_Cb = ((height / 2 + 7) / 8) * ((width / 2 + 7) / 8) * 64;
        unsigned int numBlocksRows_Cb = ((height + 1) / 2 + 7) / 8; // Calculate the number of rows (rounded up)
        unsigned int numBlocksCols_Cb = ((width + 1) / 2 + 7) / 8; // Calculate the number of columns (rounded up)
        
        if (I_frame == 0){//with motion vector
            auto result = computeEncodedBlocks_motion(numBlocksRows_Cb, numBlocksCols_Cb, input_stream, large_index_list_Cb);
            encoded_Cb = result.first;
            motion_vector_Cb = result.second;
        }
        else{
            //origin version
            encoded_Cb = computeEncodedBlocks(numBlocksRows_Cb, numBlocksCols_Cb, input_stream);
        }
        
        std::vector<std::pair<int, int>> large_index_list_Cr;
        // get delta encoding data for Cr
        size_t numElements_Cr = ((height / 2 + 7) / 8) * ((width / 2 + 7) / 8) * 64;
        unsigned int numBlocksRows_Cr = ((height + 1) / 2 + 7) / 8; // Calculate the number of rows (rounded up)
        unsigned int numBlocksCols_Cr = ((width + 1) / 2 + 7) / 8; // Calculate the number of columns (rounded up)
        if (I_frame == 0){//with motion vector
            auto result = computeEncodedBlocks_motion(numBlocksRows_Cr, numBlocksCols_Cr, input_stream, large_index_list_Cr);
            encoded_Cr = result.first;
            motion_vector_Cr = result.second;
        }
        else{
            //origin version
            encoded_Cr = computeEncodedBlocks(numBlocksRows_Cr, numBlocksCols_Cr, input_stream);
        }
       
        //reverse delta encoding,zigzagCoefficientz
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Y = reverseDeltaEncode(encoded_Y);
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cb = reverseDeltaEncode(encoded_Cb);
        std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded_Cr = reverseDeltaEncode(encoded_Cr);
        
        float scalarl;//larger, lower quality
        float scalarc;
        if (quality == 0){//0 for low, 1 for medium,2 for high
            scalarl = 1.2;//1.2//2.5
            scalarc = 1.5;//1.5//2.5
        }
        else if (quality == 1){
            scalarl = 1;//1//2
            scalarc = 1;//1//2
        }
        else{//high
            scalarl = 0.7;
            scalarc = 0.5;
        }
        //if this is in Pframe, the result will be different, with many 0's matrices
        matrices_Y = reverseProcessMatrices(decoded_Y,scalarl*luminanceQuantizationTable);
        matrices_Cb = reverseProcessMatrices(decoded_Cb,scalarc*chrominanceQuantizationTable);
        matrices_Cr = reverseProcessMatrices(decoded_Cr,scalarc*chrominanceQuantizationTable);
        
        if (I_frame == 0){//the P frame
        //I have moton_vector_Y, i have matrices_Y as reverse processmatrices
            //predicted 
            //this is the diff: predicted = oricurrent + previous(+motion)
            //oricurrrent = previous(+motion) - diff. diff is in matrices_
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> P_frame_Y = matrices_Y;
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> P_frame_Cb = matrices_Cb;
            std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> P_frame_Cr = matrices_Cr;
            
            matrices_Y = reverse_motion(P_frame_Y, prev_matrices_Y,motion_vector_Y,large_index_list_Y);
            matrices_Cb = reverse_motion(P_frame_Cb, prev_matrices_Cb,motion_vector_Cb,large_index_list_Cb);
            matrices_Cr = reverse_motion(P_frame_Cr, prev_matrices_Cr,motion_vector_Cr,large_index_list_Cr); 
        }

        I_frame = 0;
        
        prev_matrices_Y = matrices_Y;
        prev_matrices_Cb = matrices_Cb;
        prev_matrices_Cr = matrices_Cr;
        
        
        std::vector<std::vector<int>> m_Y;
        std::vector<std::vector<int>> m_Cb;
        std::vector<std::vector<int>> m_Cr;
        flatten2DTo1D(matrices_Y, m_Y);
        flatten2DTo1D(matrices_Cb, m_Cb);
        flatten2DTo1D(matrices_Cr, m_Cr);
        std::vector<std::vector<unsigned char>> char_Y = convertToUnsignedChar(m_Y);
        std::vector<std::vector<unsigned char>> char_Cb = convertToUnsignedChar(m_Cb);
        std::vector<std::vector<unsigned char>> char_Cr = convertToUnsignedChar(m_Cr);
        
        YUVFrame420& frame = writer.frame();
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                frame.Y(x,y) = m_Y[y][x];
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cb(x,y) = m_Cb[y][x];
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cr(x,y) = m_Cr[y][x];
        writer.write_frame();
        
    }
    return 0;
}
//g++ -c -O3 -Wall -std=c++20 -I./eigen-3.4.0 -Wno-unused-variable -Wno-unused-but-set-variable uvid_decompress.cpp -o uvid_decompress.o
//gdb ./uvid_decompress
//run < compressed.uvi > output_video.raw
//b r eak uvid_decompress.cpp:line_number

//1.continue reverse_motion segment fault

//2.reduce the size