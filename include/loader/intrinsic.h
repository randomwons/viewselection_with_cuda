#ifndef __INTRINSIC_H__
#define __INTRINSIC_H__

#include <cuda_runtime.h>

#include <fstream>
#include <sstream>

class Intrinsic {
    
    static constexpr size_t NUM_ELEMENTS = 9;

public:
    double data[NUM_ELEMENTS];

    __host__ Intrinsic(const std::string& filename) {
        
        std::ifstream file(filename);
        if(!file.is_open()) {
            std::string errorMsg = "[Intrinsic] Can't open file" + filename;
            std::runtime_error(errorMsg.c_str());
            return;
        }

        std::string line;
        std::string cell;
        int index = 0;

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            while(iss >> cell) {
                if(index < NUM_ELEMENTS) {
                    data[index++] = std::stod(cell);
                }
            }
        }
        file.close();

    }

    __device__ double fx() { return data[0]; }
    __device__ double fy() { return data[4]; }
    __device__ double cx() { return data[2]; }
    __device__ double cy() { return data[5]; }

};


#endif // __INTRINSIC_H__