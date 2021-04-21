#include "bk_asm.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include "file_helper.hpp"

bk_asm::bk_asm(const n64_span& span){
    _comp_span = new n64_span(span);
}

bk_asm::bk_asm(const n64_span& code_span, const n64_span& data_span){
    _code_span = new n64_span(code_span);
    _data_span = new n64_span(data_span);
}

bk_asm::~bk_asm(void){
    if(_comp_buff != nullptr){
        free(_comp_buff);
        _comp_buff = nullptr;
    }
    if(_code_buff != nullptr){
        free(_code_buff);
        _code_buff = nullptr;
    }
    if(_data_buff != nullptr){
        free(_data_buff);
        _data_buff = nullptr;
    }

    if(decomper != nullptr)
        free(decomper);
        decomper = nullptr;

    if(_comp_span != nullptr){
        free(_comp_span);
        _comp_span = nullptr;
    }
    if(_code_span != nullptr){
        free(_code_span);
        _code_span = nullptr;
    }
    if(_data_span != nullptr){
        free(_data_span);
        _data_span = nullptr;
    }
}

const n64_span& bk_asm::compressed(int lvl){
    if(_comp_span == nullptr){
        if(_code_span == nullptr)
            throw "code file never initialized";
        if(_data_span == nullptr)
            throw "data file never initialized";

        _comp_method(lvl);

        if(_comp_span == nullptr)
            throw "Unable to compress file";
    }
    return *_comp_span;
}

const n64_span& bk_asm::code(void) const{
    if(_code_span == nullptr){
        if(_comp_span == nullptr)
            throw "Asm file never initialized";

        _decomp_method();

        if(_code_span == nullptr)
            throw "Unable to decompress file";
    }
    return *_code_span;
}

const n64_span& bk_asm::data(void) const{
    if(_data_span == nullptr){
        if(_comp_span == nullptr)
            throw "Asm file never initialized";

        _decomp_method();

        if(_data_span == nullptr)
            throw "Unable to decompress file";
    }
    return *_data_span;
}

void bk_asm::_comp_method(int lvl) const{
   //write code buffer to tmp file
    std::string icodef_name = tempnam(nullptr, "bk_");
    span_write(code(), icodef_name);

    //write code buffer to tmp file
    std::string ocodef_name = tempnam(nullptr, "bk_");
    std::string command = "./gzip-1.2.4/gzip -c --no-name -";
    command += std::to_string(lvl);
    command += " ";
    command += icodef_name;
    command += " | head --bytes=-8 | tail --bytes=+11 > ";
    command += ocodef_name;
    system(command.data());

    //write data buffer to tmp file
    std::string idataf_name = tempnam(nullptr, "bk_");
    span_write(data(), idataf_name);

    std::string odataf_name = tempnam(nullptr, "bk_");
    command = "./gzip-1.2.4/gzip -c --no-name -";
    command += std::to_string(lvl);
    command += " ";
    command += idataf_name;
    command += " | head --bytes=-8 | tail --bytes=+11 > ";
    command += odataf_name;
    system(command.data());


    //read comp code buffers from compressed file
    std::ifstream ocodef_s(ocodef_name, std::ios::in | std::ios::binary | std::ios::ate);
    uint32_t code_comp_size = ocodef_s.tellg();

    std::ifstream odataf_s(odataf_name, std::ios::in | std::ios::binary | std::ios::ate);
    uint32_t data_comp_size = odataf_s.tellg();

    size_t finalSize = code_comp_size + 6 + data_comp_size + 6;
    while(finalSize & 0xF) finalSize++;

    _comp_buff = (uint8_t*) malloc(finalSize);

    _comp_span = new n64_span(_comp_buff, finalSize);
    _comp_span->set<uint16_t>(0x1172);
    _comp_span->set<uint32_t>(code().size(), 2);
    
    ocodef_s.seekg(0);
	ocodef_s.read((char*)_comp_buff + 6, code_comp_size);
	ocodef_s.close();

    _comp_span->set<uint16_t>(0x1172, code_comp_size + 6);
    _comp_span->set<uint32_t>(data().size(), code_comp_size + 6 + 2);

    odataf_s.seekg(0);
	odataf_s.read((char*)_comp_buff + code_comp_size + 12, data_comp_size);
	odataf_s.close();

    for(int i = code_comp_size + 6 + data_comp_size + 6; i < finalSize; i++){
        _comp_span->set<uint8_t>(0x00,i);
    }

    //remove temp files
    std::remove(icodef_name.data());
    std::remove(idataf_name.data());
    std::remove(ocodef_name.data());
    std::remove(odataf_name.data());
}

void bk_asm::_decomp_method(void) const{
    if(decomper == nullptr)
        decomper = libdeflate_alloc_decompressor();

    size_t code_comp_size = _comp_span->size();
    
    size_t tmpCompSize;
    size_t codeSize = 0;
    size_t dataSize = 0;
    size_t codeCompSize;
    size_t dataCompSize;

	//decompress Code
    size_t code_size = _comp_span->get<uint32_t>(2); 
    _code_buff = (uint8_t*)malloc(code_size);
    enum libdeflate_result decompResult = libdeflate_deflate_decompress_ex( decomper, 
        _comp_span->begin() + 6, _comp_span->size() - 6, 
        _code_buff, code_size, &codeCompSize, &codeSize
    );
    _code_span = new n64_span(_code_buff, codeSize);
    //std::cout << std::hex << "code_comp_size " << (uint32_t) codeCompSize <<std::endl;
    
    //decompress data
    codeCompSize +=6;
    const n64_span& data_cmp_span = _comp_span->slice(codeCompSize, _comp_span->size() - codeCompSize);
    size_t data_size = _comp_span->get<uint32_t>(codeCompSize + 2);
    _data_buff = (uint8_t*)malloc(data_size); 
    decompResult = libdeflate_deflate_decompress_ex( decomper, 
        data_cmp_span.begin() + 6, 
        data_cmp_span.size() - 6, 
        _data_buff, data_size, &dataCompSize, &dataSize
    );
    _data_span = new n64_span(_data_buff, dataSize);

    return;
}
