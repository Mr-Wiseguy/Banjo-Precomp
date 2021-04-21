#include <string>
#include <iostream>
#include <fstream>
#include "bk_asset.hpp"


int main(int argc, char** argv){

    //parse arguments
    if(argc != 3){
        std::cout << "Incorrect syntex. bk_assets_build <path/to/output/dir> <path/to/in/dir>" <<std::endl;
        return 0;
    }
    std::string o_path = argv[1];
    std::string a_path = argv[2];
    

    bk_asset_fs a_fs(a_path);
    a_fs.build(o_path);

    return 0;

}