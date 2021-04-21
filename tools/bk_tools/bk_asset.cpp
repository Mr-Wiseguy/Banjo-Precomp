#include "bk_asset.hpp"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <cstdlib>
#include"file_helper.hpp"
#include <string>

//#include "DKRCompression.h"
//#include "libbkdeflate/libbkdeflate.h"



#pragma region BK_ASSET_META
bk_asset_meta::~bk_asset_meta(){
    if(_data != nullptr)
        free(_data);
}

bk_asset_meta& bk_asset_meta::operator=(const n64_span& other){
    offset = other(0x00);
    comp = other(0x04);
    type = other(0x06);
    size = (type == 4)? 0 : static_cast<uint32_t>(other(0x08)) - offset;
    return *this;
}

bk_asset_meta::operator n64_span() const{
    if(_data == nullptr){
        _data = (uint8_t *)malloc(8);
        if(_data == nullptr)
            throw "bad malloc";
    }
    n64_span data_s(_data,8);
    data_s.set(offset,0);
    data_s.set(comp,4);
    data_s.set(type,6);
    return data_s;
}
#pragma endregion BK_ASSET_META

#pragma region BK_ASSET
bk_asset::bk_asset(const bk_asset_meta& _meta, const n64_span& asset_bin)
:meta(_meta){
    if(meta.type == 4){ 
        _decomp_buffer = new n64_span();
        _comp_buffer = _decomp_buffer;
        return;
    }

    _comp_buffer  = new n64_span(asset_bin.slice(meta.offset, meta.size));
}

bk_asset::bk_asset(const bk_asset_meta& _meta, const std::string& path)
:meta(_meta){
    if(meta.type == 4){ 
        _decomp_buffer = new n64_span();
        _comp_buffer = _decomp_buffer;
        return;
    }
    
    std::ifstream i_asset(path, std::ios::in | std::ios::binary | std::ios::ate);
    if(!i_asset)
        std::cout << "failed to open asset " << path << std::endl;
    
    uint32_t u_size = i_asset.tellg();
    uint8_t * _decomp_data = (uint8_t *)malloc(u_size);
	if(_decomp_data == nullptr){
		return;
	}

	i_asset.seekg(0);
	i_asset.read((char*)_decomp_data, u_size); //copy asset to RAM
	i_asset.close();

    _decomp_buffer = new n64_span(_decomp_data, u_size);
    _decomp_own = true;
}

bk_asset::~bk_asset(){
    if(_comp_data != nullptr){
        free(_comp_data);
        _comp_data = nullptr;
    }
    if(_decomp_data != nullptr){
        free(_decomp_data);
        _decomp_data = nullptr;
    }
    /*if(_comp_buffer != nullptr && _comp_buffer != _decomp_buffer){
        free(_comp_buffer);
        _comp_buffer = nullptr;
    }*/
    /*if(_decomp_buffer != nullptr){
        free(_decomp_buffer);
        _decomp_buffer = nullptr;
        _comp_buffer = nullptr;
    }*/

    if(_decomper != nullptr){
        free(_decomper);
        _decomper = nullptr;
    }
}

uint32_t bk_asset::compare(const bk_asset& other) const{
    //compare decompressed
    if(decompress().size() != other.decompress().size())
        throw "decompressed assets are not the same";
    //if(decompress() != other.decompress())
        //throw "decompressed assets are not the same";

    //compare compressed
    if(compress() == other.compress())
        return 0;

    //find byte count difference
    uint32_t mismatch = std::abs(static_cast<int>(compress().size() - other.compress().size()));

    uint8_t* i = compress().begin();
    uint8_t* j = other.compress().begin();

    for(;i != compress().end() && j != other.compress().end(); i++, j++){
        if(*i != *j) mismatch++;
    }
    return mismatch;
}

const n64_span& bk_asset::compress(int lvl, bool force) const{
    if(force){
        if(_comp_data != nullptr){
            free(_comp_data);
            _comp_data = nullptr;
        }
        if(_comp_buffer != nullptr && _comp_buffer != _decomp_buffer){
            delete _comp_buffer;
            _comp_buffer = nullptr;
        }
    }
    
    if (_comp_buffer == nullptr) {
		if (_decomp_buffer == nullptr)
			throw "N64 File data never initialized";
        //_comp_method();
        if(lvl < 10)
            _gzip_comp_method(lvl,"./gzip-1.2.4/gzip");
            //_gzip_comp_method(lvl,"./libbkdeflate/gzip");
            //_bk_deflate_method(lvl);
        else
            _deflate_comp_method(lvl);
        
        if (_comp_buffer == nullptr)
		    throw "Unable to compress file";
	}
    meta.size = _comp_buffer->size();
	return *_comp_buffer;
}

const n64_span& bk_asset::decompress(void) const{
    if (_decomp_buffer == nullptr) {
		if (_comp_buffer == nullptr)
			throw "N64 File data never initialized";

        _decomp_method();
        
        if (_decomp_buffer == nullptr)
            throw "Unable to decompress file";
	}
	return *_decomp_buffer;
}

void bk_asset::_decomp_method() const{
    if(!meta.comp){
        _decomp_buffer = _comp_buffer;
        return;
    }
    
    size_t uncompressedSize = _comp_buffer->get<uint32_t>(2);

    if(_decomper == nullptr)
        _decomper = libdeflate_alloc_decompressor();
    _decomp_data = (uint8_t *) malloc(uncompressedSize); //N64 memory can not be larger than 8 MB
    libdeflate_deflate_decompress(_decomper,_comp_buffer->begin() + 6,_comp_buffer->size() - 6, _decomp_data, uncompressedSize, &uncompressedSize);
    _decomp_buffer = new n64_span(_decomp_data, uncompressedSize);
    _decomp_own = true;
}

void bk_asset::_gzip_comp_method(int lvl, const std::string& version) const{
    if(!meta.comp){
        _comp_buffer = _decomp_buffer;
        return;
    }

   //write decomp buffer to tmp file
    std::string if_name = tempnam(nullptr, "bk_");
    span_write(decompress(), if_name);

    //write decomp buffer to tmp file
    std::string of_name = tempnam(nullptr, "bk_");
    std::string command = version + " -c --no-name -";
    command += std::to_string(lvl);
    command += " ";
    command += if_name;
    command += " | head --bytes=-8 | tail --bytes=+11 > ";
    command += of_name;
    //std::cout << command << std::endl;
    system(command.data());

    //read comp buffer from file
    std::ifstream of_s(of_name, std::ios::in | std::ios::binary | std::ios::ate);
    uint32_t o_size = of_s.tellg();

    size_t finalSize = o_size + 6;
    while(finalSize & 0x7) finalSize++;

    _comp_data = (uint8_t*) malloc(finalSize);

    of_s.seekg(0);
	of_s.read((char*)_comp_data + 6, o_size); //copy ROM to RAM
	of_s.close();

    _comp_buffer = new n64_span(_comp_data, finalSize);
    _comp_buffer->set<uint16_t>(0x1172);
    _comp_buffer->set<uint32_t>(decompress().size(), 2);
    _comp_own = true;

    for(int i = o_size + 6; i < finalSize; i++){
        _comp_buffer->set<uint8_t>(0xAA,i);
    }

    //remove temp files
    std::remove(if_name.data());
    std::remove(of_name.data());
}

/*
void bk_asset::_bk_deflate_method(int lvl) const{
    if(!meta.comp){
        _comp_buffer = _decomp_buffer;
        return;
    }
    int compressedSize = 0;
    std::cout << "1" << std::endl << std::flush;
    uint8_t *_buffer = bkdeflate(_decomp_buffer->begin(),_decomp_buffer->size(), &compressedSize, lvl);
     std::cout << "2" << std::endl << std::flush;
    
    size_t finalSize = compressedSize;
    while(finalSize & 0x7) finalSize++;

     std::cout << "3" << std::endl << std::flush;
    _comp_data = (uint8_t*)malloc(finalSize);
    _comp_buffer = new n64_span(_comp_data, finalSize);
    _comp_own = true;

     std::cout << "4" << std::endl << std::flush;
    memcpy(_comp_data,_buffer, compressedSize);
    for(int i = compressedSize; i < finalSize; i++){
        _comp_buffer->set<uint8_t>(0xAA,i);
    }
     std::cout << "5" << std::endl << std::flush;

}
*/

void bk_asset::_deflate_comp_method(int lvl) const{
    if(!meta.comp){
        _comp_buffer = _decomp_buffer;
        return;
    }

    libdeflate_compressor* comper = libdeflate_alloc_compressor(lvl);

    uint8_t* _buffer = (uint8_t*)malloc(_decomp_buffer->size());

	size_t o_size = libdeflate_deflate_compress(comper, _decomp_buffer->begin(), _decomp_buffer->size(), _buffer, _decomp_buffer->size());

    size_t finalSize = o_size + 6;
    while(finalSize & 0x7) finalSize++;

	//add bk_header
    _comp_data = (uint8_t *)realloc(_buffer, finalSize);
    _comp_buffer = new n64_span(_comp_data, finalSize);
    _comp_buffer->set<uint16_t>(0x1172);
    _comp_buffer->set<uint32_t>(decompress().size(), 2);
    _comp_own = true;

    for(int i = o_size + 6; i < finalSize; i++){
        _comp_buffer->set<uint8_t>(0xAA,i);
    }

    free(comper);

	return;
}

#pragma endregion BK_ASSET

#pragma region BK_ASSET_FS
bk_asset_fs::bk_asset_fs(const n64_span& buffer){
    size = buffer;
    const n64_span& tbl = buffer.slice(8, (size_t) size*8);
    uint32_t assets_start = 8 + size*8;
    uint32_t assets_end = tbl(size*8);
    const n64_span& a_span = buffer.slice(assets_start, (size_t)(assets_end- assets_start));
    for(int i = 0; i < size; i++){
        bk_asset_meta iMeta(tbl(i*8));
        assets.insert_or_assign(i, bk_asset(iMeta, a_span));
    }
}

bk_asset_fs::bk_asset_fs(const std::string& path){
    std::ifstream i_tbl(path + "/assets.tbl");
    if(!i_tbl.is_open()){
        std::cout << "failed to open " << path << "/assets.tbl" << std::endl;
        return;
    }

    while(!i_tbl.eof()){
        uint32_t index;
        bool comp;
        uint32_t type;
        i_tbl >> std::hex >> index >> comp >> type;
        if(type == 4){
            assets.insert_or_assign(index, bk_asset(bk_asset_meta(comp, type), n64_span()));
        }
        else
        {
            //open associated asset file
            std::stringstream f_name_s;
            f_name_s << path << "/";
            f_name_s << std::hex << std::setw(4) << std::setfill('0') << index;
            f_name_s << ".bin";
            const std::string f_name(f_name_s.str());

            assets.insert_or_assign(index, bk_asset(bk_asset_meta(comp, type), f_name));
        }
    }
    i_tbl.close();
}

void bk_asset_fs::extract(std::string path){
    std::cout << "extracting assets..." << std::endl;
    std::ofstream o_tbl(path + "/assets.tbl");
    std::for_each(assets.begin(), assets.end(),
        [&](const auto& x){
            const int key = x.first;
            const bk_asset& asset = x.second;
            std::stringstream index;
            index << std::hex << std::setw(4) << std::setfill('0') << key;
            const std::string i_str(index.str());
            
            o_tbl << std::hex << std::setfill('0');
            o_tbl << i_str << ' ';
            //o_tbl << std::setw(8) << asset.meta.offset << ' ';
            o_tbl << std::setw(4) << asset.meta.comp << ' ';
            o_tbl << std::setw(4) << asset.meta.type << std::endl;

            if(asset.meta.type != 4)
                span_write(asset.decompress(), path + "/" + i_str + ".bin");
        }
    );
    o_tbl.close();
    std::cout << "asset extraction complete" << std::endl;
}

void bk_asset_fs::build(std::string path){
    std::ofstream o_bin(path + "/assets.bin", std::ios::out | std::ios::binary);
    
    //asset_size
    uint8_t* tmp_be = (uint8_t *) malloc(8);
    n64_span tmp_s(tmp_be, (size_t) 8);
    uint32_t max_id = assets.rbegin()->first + 1;
    tmp_s.set<uint32_t>(max_id);
    tmp_s.set<int32_t>(-1,4);
    o_bin.write((char *) tmp_be, 8);
    free(tmp_be);

    _update_offsets();

    //write asset table entries
    for(const auto& [key, asset] : assets){
        n64_span tbl_span = asset.meta;
        o_bin.write((char *) tbl_span.begin(), 8);
    }

    //write asset_data
    for(const auto& [key, asset] : assets){
        if(asset.meta.type != 4){
            const n64_span& bin_span = (asset.meta.comp ? asset.compress() : asset.decompress());
            o_bin.write((char *) bin_span.begin(), bin_span.size());
        }
    }

    //pad
    uint32_t o_size = o_bin.tellp();
    const char zeros[0x10] = {0};
    if(o_size % 0x10){
        o_bin.write(zeros, 0x10 - (o_size % 0x10));
    }
    o_bin.close();
}

//updates offsets and fills table
void bk_asset_fs::_update_offsets(void){
    //update offsets and fill in blank entries
    uint32_t offset = 0;
    uint32_t max_index = assets.rbegin()->first;
    for(int i = 0; i < max_index + 1; i++){
        if(!assets.contains(i)){//missing i = treat as null;
            assets.insert_or_assign(i, bk_asset(bk_asset_meta(0, 4), n64_span()));
            assets.at(i).meta.offset = offset;
        }
        else{
            assets.at(i).meta.offset = offset;
            if( assets.at(i).meta.type != 4){
                try{
                    assets.at(i).compress();
                }catch(char const* e){
                    std::cout << e << std::endl << std::flush;
                } //force compression to get compressed size;
            }
            offset += assets.at(i).meta.size;
            
        }
    }
}

#pragma endregion BK_ASSET_FS
