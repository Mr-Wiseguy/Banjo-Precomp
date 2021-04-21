#include <string>
#include <fstream>
#include <iostream>
#include "bk_asm.hpp"
#include "file_helper.hpp"

int main(int argc, char** argv){
 //parse arguments
    if(argc != 4){
        std::cout << "Incorrect syntex. bk_deflate_code <output.bin> <code.bin> <data.bin>" << std::endl;
        return 0;
    }
    std::string o_path = argv[1];
    std::string c_path = argv[2];
    std::string d_path = argv[3];

    //try opening ROMS
    std::ifstream c_s(c_path, std::ios::in | std::ios::binary | std::ios::ate);
    uint32_t c_size = c_s.tellg();
    size_t final_c = c_size;
    //while(final_c & 0xF){final_c++;};
    uint8_t *c_buffer = (uint8_t *)malloc(final_c);
    
    n64_span c_span(c_buffer, final_c); 
    c_s.seekg(0);
	c_s.read((char*)c_buffer, c_size);
	c_s.close(); 

    std::ifstream d_s(d_path, std::ios::in | std::ios::binary | std::ios::ate);
    uint32_t d_size = d_s.tellg();
    size_t final_d = d_size;
    while(final_d & 0xF)
        final_d++;
    uint8_t *d_buffer = (uint8_t *)malloc(final_d);
    n64_span d_span(d_buffer, final_d); 
    d_s.seekg(0);
	d_s.read((char*)d_buffer, d_size);
	d_s.close(); 

    bk_asm assembly(c_span, d_span);
    span_write(assembly.compressed(), o_path);

    return 0;
}