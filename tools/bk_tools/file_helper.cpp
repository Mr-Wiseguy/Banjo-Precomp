#include "file_helper.hpp"
#include <iostream>
#include <fstream>
#include <iomanip> 
#include <unistd.h>
void span_write(const n64_span & s, const std::string & filename){

    std::ofstream of(filename, std::ios::out | std::ios::binary);
    of.write((char *) s.begin(), s.size());
    of.close();
}
