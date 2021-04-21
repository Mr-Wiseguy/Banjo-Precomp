#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <map>
#include <vector>
#include <openssl/md5.h>
#include "n64_span.h"
#include <cstdint>
#include "bk_asset.hpp"
#include "bk_asm.hpp"
#include "yaml-cpp/yaml.h"

#include "file_helper.hpp"


static const uint32_t N64_SIG = 0x40123780;
static const uint32_t V64_SIG = 0x37804012;
static const uint32_t Z64_SIG = 0x80371240;

typedef enum supported_games {
    UNKNOWN_GAME,
    BANJOKAZOOIE_NTSC,
    BANJOKAZOOIE_PAL,
    BANJOKAZOOIE_JP,
    BANJOKAZOOIE_NTSC_REV1,
}supported_games_t;

static const uint64_t BK_USA_HASH        = 0xB29599651A13F681;
static const uint64_t BK_PAL_HASH        = 0x06A43BACF5C0687F;
static const uint64_t BK_JP_HASH        = 0x3D3855A86FD5A1B4;
static const uint64_t BK_USA_REV1_HASH    = 0xB11F476D4BC8E039;

static const std::map<uint64_t, supported_games_t> hash_ID_map = {
        {BK_USA_HASH, BANJOKAZOOIE_NTSC},
        {BK_PAL_HASH, BANJOKAZOOIE_PAL},
        {BK_JP_HASH, BANJOKAZOOIE_JP},
        {BK_USA_REV1_HASH, BANJOKAZOOIE_NTSC_REV1}
    };

static const uint32_t level_asm_upper[0x0D] = {
        0x187A,//mm
        0x1872,//ttc
        0x180A,//cc
        0x1882,//bgs
        0x1892,//fp
        0x1902,//lair
        0x181A,//gv
        0x18EA,//ccw
        0x188A,//rbb
        0x1812,//mmm
        0x18F2,//sm
        0x190A,//fight
        0x18FA,//cs
};

struct rom_sec{
    std::string name;
    uint32_t offset;
    size_t size;
};

static std::map<uint32_t,uint32_t> unknown_rom_sections;
static std::vector<rom_sec> _sections;
static n64_span buffer;


namespace fs = std::filesystem;

void generate_splat(const std::string p, const fs::directory_entry& rom_bin);
n64_span section_rom(uint32_t offset, size_t size, const std::string& name);
uint32_t _from_inst(const n64_span & span, uint32_t upper, uint32_t lower);

int main(int argc, char** argv){
    std::string path = "";
    std::string rom_path = "";

    //parse arguments
    for(int i = 1; i < argc; i++){
        std::string argi= argv[i];
        if(argi == "-p")
            path = argv[++i];
        else if(argi == "-r")
            rom_path = argv[++i];
    }

    //try opening ROMS
    fs::directory_entry rom(rom_path);
    if(rom.is_directory()){
        try{
            fs::directory_iterator rom_dir = fs::directory_iterator(rom_path);

            std::for_each(begin(rom_dir), end(rom_dir),
                [& path](const fs::directory_entry& f){ generate_splat(path, f); }        
            );
        }
        catch (const std::exception& e){
            std::cout << e.what() << std::endl;
        }
    }
    else{
        generate_splat(path, rom);
    }

    return 0;
}

void generate_code_splat(const std::string& name, const std::string& ver_str, const bk_asm& asm_span, uint32_t RAM){
    auto toHex = [&](uint32_t val)->std::string{
        std::stringstream stream;
        stream << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << val;
        return stream.str();
    };
    
    fs::create_directories(name);
    YAML::Node y_Node;
    y_Node["basename"] = name;
    
    YAML::Node y_Opt;
    y_Opt["find_file_boundaries"] = true;
    y_Opt["compiler"] = YAML::Load(R"("IDO")");
    y_Opt["undefined_syms"] = "undefined_syms." + ver_str + ".txt";
    y_Node["options"] = y_Opt;

    YAML::Node y_Seg;
    y_Seg[0]["name"] = "code";
    y_Seg[0]["type"] = "code";
    y_Seg[0]["start"] = toHex(0);
    y_Seg[0]["vram"] = toHex(RAM);
    y_Seg[0]["files"][0] = YAML::Load("[" + toHex(0) + ", asm]");
    y_Seg[1]["name"] = "data";
    y_Seg[1]["type"] = ".data";
    y_Seg[1]["start"] = toHex(asm_span.code().size());
    y_Seg[1]["vram"] = toHex(RAM + asm_span.code().size());
    y_Seg[1]["files"][0] = YAML::Load("[" + toHex(asm_span.code().size()) + ", bin]");
    y_Seg[2] = "[" + toHex(asm_span.code().size() + asm_span.data().size())+ "]";

    y_Node["segments"] = y_Seg;
    std::ofstream f_yaml(name + "/" + name + "." + ver_str + ".yaml", std::ios::out);
    f_yaml << y_Node;
    f_yaml.close();

}


void generate_splat(const std::string p, const fs::directory_entry& f){
    std::cout << std::noshowbase;
    std::cout << f.path() << std::endl;

    std::ifstream rom_f(f.path().string(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!rom_f)
        return;



    size_t rom_size = rom_f.tellg();
    //TODO: make sure size is appropriate
    uint8_t * _buffer = (uint8_t *)malloc(rom_size);
    if(_buffer == nullptr){
        return;
    }
    unknown_rom_sections = std::map<uint32_t,uint32_t>();
    _sections =  std::vector<rom_sec>();
    unknown_rom_sections.insert_or_assign(0,rom_size);

    rom_f.seekg(0);
    rom_f.read((char*)_buffer, rom_size); //copy ROM to RAM
    rom_f.close();

    buffer = n64_span(_buffer, rom_size);
    auto rom_sig = buffer.get<uint32_t>();
    std::cout << "ROM Sig: " << std::hex << rom_sig << " => ";

    //check/correct endianess
    switch(rom_sig){
        case N64_SIG:
            std::cout << "*.n64 small endian" << std::endl;
            for(int i=0; i<rom_size; i+=4){
                std::swap(_buffer[i], _buffer[i+3]);
                std::swap(_buffer[i+1], _buffer[i+2]);
            }
            break;
        case V64_SIG:
            std::cout << "*.v64 mixed endian" << std::endl;
            for(int i=0; i<rom_size; i+=2){
                std::swap(_buffer[i], _buffer[i+1]);
            }
            break;
        
        case Z64_SIG:
            std::cout << "*.z64 big endian" << std::endl;
            break;
        default:
            std::cout << "NOT N64 ROM" << std::endl;
            return;    
    }

    //check MD5 Hash matches a BK version    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(_buffer, rom_size, hash);
    std::cout << "MD5:";
    for(int i = 0; i< 8; i++){
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)hash[i];
    }
    std::cout << " => ";

    supported_games_t gameID;
    uint64_t _hash = n64_span(hash,8).get<uint64_t>();

    std::string ver_str;
    switch(_hash){
        case BK_USA_HASH:
            gameID = BANJOKAZOOIE_NTSC;
            ver_str = "us.v10";
            break;
        case BK_PAL_HASH:
            gameID = BANJOKAZOOIE_PAL;
            ver_str = "pal";
            break;
        case BK_JP_HASH:
            gameID = BANJOKAZOOIE_JP;
            ver_str = "jp";
            break;
        case BK_USA_REV1_HASH:
            gameID = BANJOKAZOOIE_NTSC_REV1;
            ver_str = "us.11";
            break;
        default:
            gameID = UNKNOWN_GAME;
            std::cout << "UNKOWN N64 GAME" << std::endl;
            return;
    }    
    std::cout << "banjo." << ver_str << std::endl;

 

    YAML::Node y_Node;
    y_Node["basename"] = "banjo";
    
    YAML::Node y_Opt;
    y_Opt["find_file_boundaries"] = true;
    y_Opt["compiler"] = R"("IDO")";
    y_Opt["undefined_syms"] = "undefined_syms." + ver_str + ".txt";
    y_Node["options"] = y_Opt;

    YAML::Node y_Seg;
    int seg_indx = 0;
    
    

std::cout << "Writing files..." <<std::endl;

    uint32_t file_count = 0;

    //n64_header
    auto toHex = [&](uint32_t val)->std::string{
        std::stringstream stream;
        stream << "0x" << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << val;
        return stream.str();
    };

    y_Seg[seg_indx]["name"] =  "header";
    y_Seg[seg_indx]["type"] =  "header";
    y_Seg[seg_indx]["start"] =  toHex(0);
    y_Seg[seg_indx++]["files"][0] = YAML::Load("[" + toHex(0) +", header, header]");
    
    y_Seg[seg_indx]["name"] =  "boot";
    y_Seg[seg_indx]["type"] =  "bin";
    y_Seg[seg_indx++]["start"] =  toHex(0x40);


    y_Seg[seg_indx]["name"] =  "bk_boot";
    y_Seg[seg_indx]["type"] =  "code";
    y_Seg[seg_indx]["start"] =  toHex(0x1000);
    y_Seg[seg_indx]["vram"] =  toHex(0x80000400);
    y_Seg[seg_indx++]["files"][0] = YAML::Load("[" + toHex(0x1000) +", asm]");
    n64_span bk_boot = buffer.slice(0x1000, 0x4BE0);

    y_Seg[seg_indx]["name"] =  "assets";
    y_Seg[seg_indx]["type"] =  "bin";
    y_Seg[seg_indx]["start"] =  toHex(0x5E90);
    y_Seg[seg_indx++]["files"][0] = YAML::Load("[" + toHex(0x5E90) +", bin, assets]");

        //snd1_ctl
    bk_asset_fs ass_fs(buffer(0x5E90));
    uint32_t asset_end = buffer.get<uint32_t>(0x5E90 + 8* (ass_fs.size));
    size_t ass_span_end = asset_end + 8*ass_fs.size + 0x8;
    while(ass_span_end % 0x10) ass_span_end++;
    uint32_t snd1_ctl_offset = 0x5E90 + ass_span_end;
    uint32_t bnk_cnt = buffer.get<uint16_t>(snd1_ctl_offset + 0x02);
    uint32_t snd1_ctl_size = buffer.get<uint32_t>(snd1_ctl_offset + 0x04) + (0x10 * bnk_cnt);
    while ((snd1_ctl_size + snd1_ctl_offset) % 0x10) snd1_ctl_size++;
    n64_span snd1_ctl_span = buffer.slice(snd1_ctl_offset, snd1_ctl_size);
    uint32_t snd1_tbl_offset = snd1_ctl_offset + snd1_ctl_size;
    y_Seg[seg_indx]["name"] =  "soundfont1";
    y_Seg[seg_indx]["type"] =  "bin";
    y_Seg[seg_indx]["start"] =  toHex(snd1_ctl_offset);
    y_Seg[seg_indx]["files"][0] = YAML::Load("[" + toHex(snd1_ctl_offset) +", bin, soundfont1.ctl]");
    y_Seg[seg_indx++]["files"][1] = YAML::Load("[" + toHex(snd1_tbl_offset) +", bin, soundfont1.tbl]");

    //find wavetable size from ctl;
    uint32_t wave_size = 0;
    for(int i = 0; i < bnk_cnt; i++){
        n64_span bnk = snd1_ctl_span.slice(snd1_ctl_span.get<uint32_t>(0x04 + 0x04*i) + i*0x10, 0x18);
        uint32_t inst_cnt = bnk.get<int16_t>();        
        bnk = snd1_ctl_span.slice(snd1_ctl_span.get<uint32_t>(0x04+ 0x04*i) + i*0x10, 0x18 + (inst_cnt-1)*4);
        for(int j = 0; j < inst_cnt; j++){
            n64_span inst = snd1_ctl_span.slice(bnk.get<uint32_t>(0xC+ j*4), 0x14);
            uint32_t snd_cnt = inst.get<int16_t>(0x0E);        
            inst = snd1_ctl_span.slice(bnk.get<uint32_t>(0xC+ j*4), 0x14 + 0x04*(snd_cnt -1 ));
            for(int k = 0; k < snd_cnt; k++){
                n64_span snd = snd1_ctl_span.slice(inst.get<uint32_t>(0x10 + k*0x4), 0x10);
                n64_span wv = snd1_ctl_span.slice(snd.get<uint32_t>(0x08), 0x10);\
                uint32_t wv_base = wv.get<uint32_t>();
                uint32_t wv_len = wv.get<uint32_t>(4);
                wave_size = (wave_size < wv_base + wv_len)? wv_base + wv_len : wave_size;
            }
        }
    }
    while(wave_size % 0x10) wave_size++;

    //snd1_tbl
    n64_span snd1_tbl_span = buffer.slice(snd1_tbl_offset, wave_size);
    uint32_t snd2_ctl_offset = snd1_tbl_offset + wave_size;
    std::cout << "[" << snd1_tbl_offset << ", " << snd1_tbl_offset + wave_size << "] sound_bank_1.tbl" << std::endl;


    //snd2_ctl
    bnk_cnt = buffer.get<uint16_t>(snd2_ctl_offset + 0x02);
    uint32_t snd2_ctl_size = buffer.get<uint32_t>(snd2_ctl_offset + 0x04 + 0x04*(bnk_cnt-1)) + (0x10 * bnk_cnt);
    n64_span snd2_ctl_span = buffer.slice(snd2_ctl_offset, snd2_ctl_size);
    snd2_ctl_size = snd2_ctl_span.size();
    while ((snd2_ctl_size + snd2_ctl_offset) % 0x10) snd2_ctl_size++;
    uint32_t snd2_tbl_offset = snd2_ctl_offset + snd2_ctl_size;
    y_Seg[seg_indx]["name"] =  "soundfont2";
    y_Seg[seg_indx]["type"] =  "bin";
    y_Seg[seg_indx]["start"] =  toHex(snd2_ctl_offset);
    y_Seg[seg_indx]["files"][0] = YAML::Load("[" + toHex(snd2_ctl_offset) + ", bin, soundfont2.ctl]");
    y_Seg[seg_indx++]["files"][1] = YAML::Load("[" + toHex(snd2_tbl_offset) +", bin, soundfont2.tbl]");

    //snd2_tbl
    n64_span snd2_tbl_span = buffer.slice(snd2_tbl_offset, wave_size);
    std::cout << "[" << snd2_tbl_offset << ", " << snd2_tbl_offset + wave_size << "] sound_bank_2.tbl" << std::endl;

    try{
        //bk_core1
        uint32_t bk_core1_offset = _from_inst(bk_boot, 0x7A, 0x82); 
        uint32_t bk_core1_size = _from_inst(bk_boot, 0x7E, 0x86);
        y_Seg[seg_indx]["name"] =  "core1." + ver_str + ".rzip";
        y_Seg[seg_indx]["type"] =  "bin";
        y_Seg[seg_indx++]["start"] =  toHex(bk_core1_offset);
        n64_span bk_core1 = buffer.slice(bk_core1_offset, bk_core1_size - bk_core1_offset);
        bk_asm core1(bk_core1);
        generate_code_splat("core1", ver_str, core1, _from_inst(bk_boot, 0x5A, 0x66));

        //bk_core_2
        uint32_t bk_core2_offset = _from_inst(bk_boot, 0x17FA, 0x1822); 
        uint32_t bk_core2_size = _from_inst(bk_boot, 0x17FE, 0x1826); 
        y_Seg[seg_indx]["name"] =  "core2." + ver_str + ".rzip";
        y_Seg[seg_indx]["type"] =  "bin";
        y_Seg[seg_indx++]["start"] =  toHex(bk_core2_offset);
        n64_span bk_core2 = buffer.slice(bk_core2_offset, bk_core2_size - bk_core2_offset);
        bk_asm core2(bk_core2);
        generate_code_splat("core2", ver_str, core2, _from_inst(core1.code(), 0x0E, 0x1A));
    }
    catch(const char* s){
        std::cout << s << std::endl;
    }
    uint32_t level_ram;
    if( gameID == BANJOKAZOOIE_NTSC)
        level_ram = 0x803863F0;
    else if( gameID == BANJOKAZOOIE_PAL)
        level_ram = 0x80386DD0;
    else if( gameID == BANJOKAZOOIE_JP)
        level_ram = 0x80386F40;
        else if( gameID == BANJOKAZOOIE_NTSC_REV1)
        level_ram = 0x80385610;

    //level overlays
    std::map<uint32_t, YAML::Node> lvl_segs; //ordered map'
    uint32_t lvl_end = 0;
    for(int i = 0; i < 0x0D; i++){
        uint32_t upper = level_asm_upper[i];
        uint32_t lower = upper + 0x28;
        uint32_t bk_lvl_offset = _from_inst(bk_boot, upper, lower);
        uint32_t bk_lvl_size = _from_inst(bk_boot, (upper + 4), (lower + 4)); 
        std::string lvl_name = "level" + std::to_string(i);
        n64_span lvl_span = buffer.slice(bk_lvl_offset, bk_lvl_size - bk_lvl_offset);
        bk_asm lvl_asm(lvl_span);
        YAML::Node lvl_node;
        lvl_node["name"] =  lvl_name + "." + ver_str + ".rzip";
        lvl_node["type"] =  "bin";
        lvl_node["start"] =  toHex(bk_lvl_offset);
        lvl_segs.insert_or_assign(bk_lvl_offset, lvl_node);
        generate_code_splat(lvl_name, ver_str, lvl_asm, level_ram);
        lvl_end = std::max(lvl_end, bk_lvl_size);
    }
    //order segments
    for(const auto& [offset, n] : lvl_segs){
        y_Seg[seg_indx++] = n;
    }
    while(lvl_end % 0x10) 
        lvl_end++;    

    uint8_t* trail_start = buffer.end()-1;
    while(*trail_start == 0xFF)
        trail_start--;
    
    y_Seg[seg_indx]["name"] =  "emptyLvl." + ver_str + ".rzip";
    y_Seg[seg_indx]["type"] =  "bin";
    y_Seg[seg_indx++]["start"] =  toHex(lvl_end);
    generate_code_splat("emptyLvl", ver_str, bk_asm(buffer.slice(lvl_end, (size_t)trail_start-lvl_end)), level_ram);
    
    y_Seg[seg_indx++] = YAML::Load("[" + toHex(rom_size) + "]");

    y_Node["segments"] = y_Seg;
    std::ofstream f_yaml("banjo." + ver_str + ".yaml", std::ios::out);
    f_yaml << y_Node;
    f_yaml.close();

    free(_buffer);


}

n64_span section_rom(uint32_t offset, size_t size, const std::string& name){
    std::map<uint32_t, uint32_t>::iterator it = unknown_rom_sections.begin();
    
    if(offset != it->first){
        it = unknown_rom_sections.upper_bound(offset);
    
        if(offset != unknown_rom_sections.begin()->first && it == unknown_rom_sections.begin())
            throw "No rom section contains specified range";

        it--;
    }

    if(it->second < offset + size)
        throw "Specified range exceeds identified unknown sections";
    
    if(offset != it->first){
        if(it->second != offset+size)
            unknown_rom_sections.insert_or_assign(offset+size, it->second);
        unknown_rom_sections.insert_or_assign(it->first, offset);
    }else{
        if(it->second != offset+size)
            unknown_rom_sections.insert_or_assign(offset+size, it->second);
        unknown_rom_sections.erase(it);
    }
    rom_sec tmp_sec = {name, offset, size};
    _sections.push_back(tmp_sec);
    n64_span tmp_span = buffer.slice(offset, size);
    span_write(tmp_span, name);
    return tmp_span;
}

uint32_t _from_inst(const n64_span & s, uint32_t upper, uint32_t lower){
    uint32_t up = (uint32_t) s.get<uint16_t>(upper);
    uint32_t low = (uint32_t) s.get<uint16_t>(lower);
    if(low > 0x7FFF) up--;
    up = up << 0x10;
    return up + low;
}