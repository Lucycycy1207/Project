/* uvg_common.hpp

   B. Bird - 2023-07-03

   V00907022
   Xinlu Chen
*/

#ifndef UVG_COMMON_HPP
#define UVG_COMMON_HPP

#include <vector>
#include <string>
#include <Eigen/Dense>
#include <iostream>
#include <array>
#include <cassert>
#include <cstdint>
#include "output_stream.hpp"

// Create an 8x8 matrix of doubles, DCT matrix
Eigen::Matrix<float, 8, 8> DCT_matrix = (Eigen::Matrix<float, 8, 8>() << 
0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,   0.3536,
0.4904,   0.4157,   0.2778,   0.0975,  -0.0975,  -0.2778,  -0.4157,  -0.4904,
0.4619,   0.1913,  -0.1913,  -0.4619,  -0.4619,  -0.1913,   0.1913,   0.4619,
0.4157,  -0.0975,  -0.4904,  -0.2778,   0.2778,   0.4904,   0.0975,  -0.4157,
0.3536,  -0.3536,  -0.3536,   0.3536,   0.3536,  -0.3536,  -0.3536,   0.3536,
0.2778,  -0.4904,   0.0975,   0.4157,  -0.4157,  -0.0975,   0.4904,  -0.2778,
0.1913,  -0.4619,   0.4619,  -0.1913,  -0.1913,   0.4619,  -0.4619,   0.1913,
0.0975,  -0.2778,   0.4157,  -0.4904,   0.4904,  -0.4157,   0.2778,  -0.0975).finished();

//correct
Eigen::Matrix<float, 8, 8> luminanceQuantizationTable = (Eigen::Matrix<float, 8, 8>() <<
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99).finished();

//correct
Eigen::Matrix<float, 8, 8> chrominanceQuantizationTable = (Eigen::Matrix<float, 8, 8>() <<
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99).finished();


//Convenience function to wrap around the nasty notation for 2d vectors
template<typename T>
std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
    std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
    return V;
}

//for testing
template<typename T>
void Data_to_csv(std::vector<std::vector<T> > image_compoment,unsigned int height, unsigned int width, std::string filename){
    // Open the output file
    std::ofstream outputFile(filename);

    // Check if the file is successfully opened
    if (!outputFile) {
        std::cerr << "Failed to open the output file." << std::endl;
        return;
    }

    // Set the delimiter
    char delimiter = ',';

    // Output the image data as a spreadsheet (CSV) file
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            outputFile << static_cast<int>(image_compoment.at(y).at(x));
            if (x < width - 1) {
                outputFile << delimiter;
            }
        }
        outputFile << "\n";
    }

    // Close the output file
    outputFile.close();
}
//for testing
void print1DPairsToFile(const std::vector<std::pair<int, int>>& pairs, const std::string& filename) {
    std::ofstream outFile(filename);
    
    if (!outFile) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (const auto& pair : pairs) {
        outFile << "Pair: (" << pair.first << ", " << pair.second << ")" << std::endl;
    }

    outFile.close();
}
//for testing

//decompressed process both used in compressor and decompressor
std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> reverseDeltaEncode(const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& encoded) {
    std::vector<std::vector<Eigen::Matrix<int, 1, 64>>> decoded;

    for (const auto& row : encoded) {
        std::vector<Eigen::Matrix<int, 1, 64>> decodedRow;

        for (const auto& matrix : row) {
            Eigen::Matrix<int, 1, 64> decodedMatrix;
            decodedMatrix(0) = matrix(0); // First element remains unchanged
            decodedMatrix(1) = matrix(1); // second element remains unchanged

            for (int i = 2; i < 64; ++i) {
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

// Convert an integer to a vector of bits
std::vector<bool> intToBitVector(int value) {
    std::vector<bool> bits;
    while (value > 0) {
        bits.push_back(value & 1); // Add least significant bit to vector
        value >>= 1; // Shift value right by 1 bit
    }
    return bits;
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
//for testing
template <typename T>
void printMatrix(const Eigen::Matrix<T, 8, 8>& matrix) {
    for (int i = 0; i < matrix.rows(); ++i) {
        for (int j = 0; j < matrix.cols(); ++j) {
            std::cout << matrix(i, j) << " ";
        }
        std::cout << std::endl;
    }
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

std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> convertMatricesIntToFloat(const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& intMatrices) {
    std::vector<std::vector<Eigen::Matrix<float, 8, 8>>> floatMatrices;
    floatMatrices.resize(intMatrices.size());
    
    for (std::size_t i = 0; i < intMatrices.size(); ++i) {
        floatMatrices[i].resize(intMatrices[i].size());
        for (std::size_t j = 0; j < intMatrices[i].size(); ++j) {
            floatMatrices[i][j] = intMatrices[i][j].cast<float>();
        }
    }
    
    return floatMatrices;
}

//convert float to int matrices 2d list
std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> convertMatricesFloatToInt(
    const std::vector<std::vector<Eigen::Matrix<float, 8, 8>>>& matricesY) {
    
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> prevMatricesY;
    prevMatricesY.resize(matricesY.size());

    for (size_t i = 0; i < matricesY.size(); ++i) {
        prevMatricesY[i].resize(matricesY[i].size());
        for (size_t j = 0; j < matricesY[i].size(); ++j) {
            // Create a temporary integer matrix
            Eigen::Matrix<int, 8, 8> temp = matricesY[i][j].array().array().round().cast<int>();
            // Assign the temporary matrix to prevMatricesY
            prevMatricesY[i][j] = temp;
        }
    }
    return prevMatricesY;
}
bool isPairInList(const std::vector<std::pair<int, int>>& list, const std::pair<int, int>& targetPair) {
    for (const auto& pair : list) {
        if (pair == targetPair) {
            return true; // Pair found in the list
        }
    }
    return false; // Pair not found in the list
}
int isIntInList(const std::vector<int>& vec, int target) {
    for (int value : vec) {
        if (value == target) {
            return 1; // Element found
        }
    }
    return 0; // Element not found
}

void writeIntVectorToFile(const std::vector<int>& vec, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        for (int value : vec) {
            file << value << std::endl;
        }
        file.close();
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }
}

Eigen::Matrix<int, 8, 8> replaceNegativeValuesWithZero(const Eigen::Matrix<int, 8, 8>& inputMatrix) {
    Eigen::Matrix<int, 8, 8> result = inputMatrix;

    for (int i = 0; i < result.rows(); ++i) {
        for (int j = 0; j < result.cols(); ++j) {
            if (result(i, j) < 0) {
                result(i, j) = 0;
            }
        }
    }

    return result;
}
//P_Frame is the diff, orimatrix = previous(+motion) - diff.
std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> reverse_motion(
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& P_frame, 
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& prev_matrices,
    std::vector<std::vector<std::pair<int, int>>>& motion_vector,
    std::vector<std::pair<int, int>>& large_index_list
    ){
    // std::cout << "P_frame("<<P_frame.size() <<","<< P_frame[0].size() << ")" << std::endl;
    // std::cout << "prev_matrices("<<prev_matrices.size() <<","<< prev_matrices[0].size() << ")" << std::endl;
     
    // for (const auto& pair : large_index_list) {
    // std::cout << "(" << pair.first << ", " << pair.second << ")" << std::endl;
    // }

    //std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> P_frame = convertMatricesFloatToInt(Pf);
    std::vector<std::vector<Eigen::Matrix<int, 8, 8>>> matrix(P_frame.size(), std::vector<Eigen::Matrix<int, 8, 8>>(P_frame[0].size()));

    for (int x = 0; x < static_cast<int>(P_frame.size()); x++) {
        for (int y = 0; y < static_cast<int>(P_frame[0].size()); y++) {
            

            int mv_Row = x/2;
            int mv_Col = y/2;
            std::pair<int, int> targetPair(mv_Row, mv_Col);
            if (isPairInList(large_index_list, targetPair)){
                //std::cout << "(" <<mv_Row <<","<<mv_Col<<")"<<std::endl;
                matrix[x][y] = P_frame[x][y];
            }
            else{
                std::pair<int, int> curr_motion = motion_vector[mv_Row][mv_Col];
                //in 1 block matrix
                //the following line cause segment fault
                //faulty bug temp solution
                if ((x + curr_motion.first) >= 0 && (x + curr_motion.first) < static_cast<int>(prev_matrices.size()) &&
                    (y + curr_motion.second) >= 0 && (y + curr_motion.second) < static_cast<int>(prev_matrices[0].size())) {
                    matrix[x][y] = prev_matrices[x + curr_motion.first][y + curr_motion.second] + P_frame[x][y];
                    
                        
                } else {
                    // Handle the case when indices are out of bounds
                    int new_first = x + curr_motion.first;
                    int new_second = y + curr_motion.second;
                    
                    //first element out of bound; 2 situations
                    if ((x + curr_motion.first) < 0){
                        new_first = 0;
                    }
                    if ((x + curr_motion.first) > static_cast<int>(prev_matrices.size())-1){
                        new_first = static_cast<int>(P_frame.size())-1;
                    }
                    //second element out of bound; 2 situations
                    if (y + curr_motion.second < 0){
                        new_second = 0;
                    }
                    if (y + curr_motion.second > static_cast<int>(prev_matrices[0].size()) - 1){
                        new_second = static_cast<int>(prev_matrices[0].size()) - 1;
                    }
                    // std::cout << "outbound matrices("<<x+curr_motion.first<<","<< y+curr_motion.second << ")" << std::endl;
                    // std::cout << "changed matrices("<<new_first<<","<< new_second << ")" << std::endl;

                    matrix[x][y] = prev_matrices[new_first][new_second] + P_frame[x][y];
                    
                    
                }
            }
            //check if smaller than 0
            for (int i = 0; i < matrix[x][y].rows(); ++i) {
                for (int j = 0; j < matrix[x][y].cols(); ++j) {
                    if (matrix[x][y](i, j) < 0) {
                        matrix[x][y](i, j) = 0;
                    }
                }
            }
            
        }
    }
        
    //printMatricesToFile(matrix, "result.txt");
    return matrix;
}
//for testing
void printListToFile(const std::vector<int>& myList, const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }
    outputFile<<myList.size()<<std::endl;
    for (int value : myList) {
        outputFile << value << " ";
    }

    outputFile.close();
}
//for testing
void outputEncodedCrToFile(const std::vector<std::vector<Eigen::Matrix<int, 1, 64>>>& encoded_Cr, const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }

    for (const auto& row : encoded_Cr) {
        for (const auto& block : row) {
            for (int i = 0; i < block.size(); ++i) {
                outputFile << block[i] << " ";
            }
            outputFile << std::endl;
        }
    }

    outputFile.close();
}

//fortesting
void writeMotionVectorToFile(const std::vector<std::vector<std::pair<int, int>>>& motion_vector_Y, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }
    outfile << motion_vector_Y.size() << " " << motion_vector_Y[0].size() << std::endl;
    int r = -1;
    int c = -1;
    for (const auto& row : motion_vector_Y) {
        r += 1;
        c = -1;
        for (const auto& pair : row) {
            c +=1;
            outfile << "(" <<r<<","<<c<<") ";
            outfile << pair.first << " " << pair.second << "\n";
        }
        
    }

    outfile.close();
}
//for testing
void writeFlattenedArrayToFile(const std::vector<std::vector<int>>& flattened_1d, const std::string& filename) {
    std::ofstream output_file(filename);
    if (!output_file.is_open()) {
        std::cerr << "Error opening output file." << std::endl;
        return;
    }

    // Write the flattened 1D array to the file
    for (const auto& row : flattened_1d) {
        for (int value : row) {
            output_file << value << " ";
        }
        output_file << std::endl;
    }

    output_file.close();
}
//for testing
void printLinearMatrix(const Eigen::Matrix<int, 1, 64>& zigzagMatrix) {
    for (int i = 0; i < 64; ++i) {
        std::cout << zigzagMatrix(0, i) << " ";
    }
    std::cout << std::endl;
}

//for testing
// Function to print the matrices to a file
template <typename T>
void printMatricesToFile(const std::vector<std::vector<Eigen::Matrix<T, 8, 8>>>& matrices, const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }
    unsigned count = 0;
    int row_num = -1;
    int col_num = -1;
    outputFile << matrices.size() << " " << matrices[0].size() << std::endl;
    for (const auto& row : matrices) {
        row_num+=1;
        col_num =-1;
        for (const auto& matrix : row) {
            col_num +=1;
            outputFile << count << ", (" << row_num << "," <<col_num <<")"<<std::endl;
            for (int i = 0; i < matrix.rows(); ++i) {
                for (int j = 0; j < matrix.cols(); ++j) {
                    outputFile << matrix(i, j) << " ";
                }
                outputFile << std::endl;
            }
            count = count +1;
            outputFile << "=====================" << std::endl;
        }
    }

    outputFile.close();
}
//for testing
template <typename T>
void printLinearMatricesToFile(const std::vector<std::vector<Eigen::Matrix<T, 1, 64>>>& matrices, const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }
    unsigned count = 0;
    int row_num = -1;
    int col_num = -1;
    outputFile << matrices.size() << " " << matrices[0].size() << std::endl;
    for (const auto& row : matrices) {
        row_num+=1;
        col_num =-1;
        for (const auto& matrix : row) {
            col_num +=1;
            outputFile << count << ", (" << row_num << "," <<col_num <<")"<<std::endl;
            for (int i = 0; i < matrix.rows(); ++i) {
                for (int j = 0; j < matrix.cols(); ++j) {
                    outputFile << matrix(i, j) << " ";
                }
                outputFile << std::endl;
            }
            count = count +1;
            outputFile << "=====================" << std::endl;
        }
    }

    outputFile.close();
}
//for testing
template <typename T>
void printRowLinearMatricesToFile(const std::vector<std::vector<Eigen::Matrix<T, 1, 64>>>& matrices, const std::string& filename) {
    std::ofstream outputFile(filename);

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }
    unsigned count = 0;
    int row_num = -1;
    int col_num = -1;
    //outputFile << matrices.size() << " " << matrices[0].size() << std::endl;
    for (const auto& row : matrices) {
        row_num+=1;
        col_num =-1;
        for (const auto& matrix : row) {
            col_num +=1;
            //outputFile << count << ", (" << row_num << "," <<col_num <<")"<<std::endl;
            for (int i = 0; i < matrix.rows(); ++i) {
                for (int j = 0; j < matrix.cols(); ++j) {
                    outputFile << static_cast<unsigned char>(matrix(i, j));
                }
                //outputFile << std::endl;
            }
            count = count +1;
            //outputFile << "=====================" << std::endl;
        }
    }

    outputFile.close();
}

template <typename T>
bool readRowLinearMatricesFromFile(const std::string& filename, std::vector<std::vector<Eigen::Matrix<T, 1, 64>>>& matrices) {
    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open input file: " << filename << std::endl;
        return false;
    }

    matrices.clear();
    std::string line;
    std::vector<Eigen::Matrix<T, 1, 64>> currentRow;
    int col_num = -1;

    while (getline(inputFile, line)) {
        if (line.empty()) {
            if (!currentRow.empty()) {
                matrices.push_back(currentRow);
                currentRow.clear();
            }
        } else {
            if (col_num == -1) {
                currentRow.push_back(Eigen::Matrix<T, 1, 64>::Zero());
                col_num = 0;
            }
            
            for (char c : line) {
                currentRow.back()(0, col_num) = static_cast<T>(c);
                col_num++;
            }
        }
    }

    if (!currentRow.empty()) {
        matrices.push_back(currentRow);
    }

    inputFile.close();
    return true;
}


std::vector<std::vector<unsigned char>> convertToUnsignedChar(const std::vector<std::vector<int>>& input) {
    std::vector<std::vector<unsigned char>> output;

    for (const auto& row : input) {
        std::vector<unsigned char> output_row;
        for (int value : row) {
            output_row.push_back(static_cast<unsigned char>(std::max(0, std::min(255, value))));
        }
        output.push_back(output_row);
    }

    return output;
}
int findIndex(const std::vector<int>& vec, int value) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == value) {
            return static_cast<int>(i); // Return the index as an integer
        }
    }
    return -1; // Value not found
}
void flatten2DTo1D(const std::vector<std::vector<Eigen::Matrix<int, 8, 8>>>& matrices_2d,
                   std::vector<std::vector<int>>& flattened_1d) {
    int numRows = matrices_2d.size() * 8;
    int numCols = matrices_2d[0].size() * 8;

    flattened_1d.resize(numRows, std::vector<int>(numCols));

    for (int i = 0; i < static_cast<int>(matrices_2d.size()); ++i) {
        for (int j = 0; j < static_cast<int>(matrices_2d[i].size()); ++j) {
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    flattened_1d[i * 8 + row][j * 8 + col] = matrices_2d[i][j](row, col);
                }
            }
        }
    }
}

//The floating point calculations we use while converting between 
//RGB and YCbCr can occasionally yield values slightly out of range
//for an unsigned char (e.g. -1 or 255.9).
//Furthermore, we want to ensure that any conversion uses rounding
//and not truncation (to improve accuracy).
inline unsigned char round_and_clamp_to_char(double v){
    //Round to int 
    int i = (int)(v+0.5);
    //Clamp to the range [0,255]
    if (i < 0)
        return 0;
    else if (i > 255)
        return 255;
    return i;
}

/* The exact RGB <-> YCbCr conversion formula used here is the "JPEG style"
   conversion (there is some debate over the best conversion formula) */
struct PixelYCbCr;
struct PixelRGB{
    unsigned char r, g, b;
    PixelYCbCr to_ycbcr(); //Implementation is below (since the PixelYCbCr type has to exist before we can fully define this function)
};

struct PixelYCbCr{
    unsigned char Y, Cb, Cr;
    inline PixelRGB to_rgb(){
        return {
            round_and_clamp_to_char(Y + 1.402*(Cr-128.0)),
            round_and_clamp_to_char(Y-0.344136*(Cb-128.0)-0.714136*(Cr-128.0)),
            round_and_clamp_to_char(Y+1.772*(Cb-128.0))
        };
    }
};


inline PixelYCbCr PixelRGB::to_ycbcr(){
    return {
        round_and_clamp_to_char(0.299*r + 0.587*g + 0.114*b),
        round_and_clamp_to_char(128 + -0.168736*r + -0.331264*g + 0.5*b),
        round_and_clamp_to_char(128 + 0.5*r + -0.418688*g + -0.081312*b)
    };
}


#endif