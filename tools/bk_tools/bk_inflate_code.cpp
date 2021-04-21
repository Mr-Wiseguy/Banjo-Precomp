#include <string>
#include <fstream>
#include <iostream>
#include "bk_asm.hpp"
#include "file_helper.hpp"

int main(int argc, char** argv){
    

    if(argc != 3){
        std::cout << "Incorrect syntex. bk_inflate_code <path/to/input.bin> <path/to/output.bin>" << std::endl;
        return 0;
    }

    std::string bin_path = argv[1];
    std::string out_path = argv[2];

    //try opening ROMS
    size_t lastindex = out_path.find_last_of("."); 
    std::string rawname = out_path.substr(0, lastindex); 

    std::ifstream in_f(bin_path, std::ios::binary | std::ios::in | std::ios::ate);
    size_t file_size = in_f.tellg();
    in_f.seekg(0);

    uint8_t* buffer = (uint8_t *) malloc(file_size);
    in_f.read((char*) buffer, file_size);
    in_f.close();
    n64_span bin(buffer, file_size);
    bk_asm file(bin);

    size_t out_size = file.code().size() + file.data().size();
    uint8_t* out_buffer = (uint8_t*) malloc(out_size);
    uint8_t* out_end = std::copy(file.code().begin(), file.code().end(), out_buffer); 
    out_end = std::copy(file.data().begin(), file.data().end(), out_end);

    free(buffer);
    std::ofstream o_f(out_path);
    o_f.write((char*) out_buffer, out_size); 
    o_f.close();

    free(out_buffer);

    return 0;
}
