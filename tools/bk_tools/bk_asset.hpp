#pragma once
//array of bk_assets
#ifndef _BK_ASSET_
#define _BK_ASSET_

#include "n64_span.h"
#include<map>
#include<string>

#include "libdeflate/libdeflate.h"

/*bk_asset*/
//bk_asset(span bin)

enum bk_comp_method{
        dkr_gzip,
        pd_gzip,
        gzip,
        //zlib,
        libdeflate,
        
};

class bk_asset_meta{
public:
    bk_asset_meta(bool _comp, uint16_t _type): comp(_comp), type(_type){};
    bk_asset_meta(const n64_span& span){
        *this = span;
    }
    ~bk_asset_meta();

    bk_asset_meta& operator=(const n64_span& other);
    operator n64_span() const;

    uint32_t offset;
    size_t size = 0;
    uint16_t comp;
    uint16_t type;

private:
    mutable uint8_t* _data = nullptr;
};

class bk_asset{
    public:
        /*bk_asset(const n64_span& meta, const n64_span& asset_bin)
        : bk_asset(bk_asset_meta(meta), asset_bin) {}
*/
        bk_asset(const bk_asset_meta& _meta, const n64_span& asset_bin);
        bk_asset(const bk_asset_meta& _meta, const std::string& path);

        ~bk_asset();

        uint32_t compare(const bk_asset& other) const;

        inline const n64_span& compress(void) const {return compress(6);}
        const n64_span& compress(int lvl, bool force = false) const;
        const n64_span& decompress(void) const;

        mutable bk_asset_meta meta;

    private:
        
        mutable n64_span* _comp_buffer = nullptr;
        mutable uint8_t* _comp_data = nullptr;
        mutable bool _comp_own = false;

        mutable n64_span* _decomp_buffer = nullptr;
        mutable uint8_t* _decomp_data = nullptr;
        mutable bool _decomp_own = false;

        mutable libdeflate_decompressor* _decomper = nullptr;

        void _decomp_method() const;
        void _comp_method() const;
        void _gzip_comp_method(int lvl, const std::string& version) const;
        void _deflate_comp_method(int lvl) const;
        //void _bk_deflate_method(int lvl) const;
};

class bk_asset_fs{
    public:
        bk_asset_fs(const n64_span& buffer);
        bk_asset_fs(const std::string& path); //init from directory (must contain)
        
        //TODO: void add(std::string path); //add assets from directory
        //void add(/*meta_info*/, /*uncompressed data*/);

        void extract(std::string path);
        void build(std::string path);
        
        uint32_t size;
        std::map<int, bk_asset> assets;
    
    private:
        void _update_offsets(void);
};
#endif