OUTPUT_ARCH (mips)

#define BEGIN_SEG(name, addr) \
    _##name##SegmentStart = ADDR(.name); \
    _##name##SegmentRomStart = __romPos; \
    .name addr : AT(__romPos)

#define END_SEG(name) \
    _##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
    _##name##SegmentRomEnd = __romPos + SIZEOF(.name); \
    _##name##SegmentSize = _##name##SegmentEnd - _##name##SegmentStart; \
    __romPos += SIZEOF(.name);

#define BEGIN_NOLOAD(name) \
    _##name##SegmentNoloadStart = ADDR(.name.noload); \
    .name.noload (NOLOAD) :

#define END_NOLOAD(name) \
    _##name##SegmentNoloadEnd = ADDR(.name.noload) + SIZEOF(.name.noload);

#define PATCH(segment, symbol, offset) \
    .segment##symbol##offset##_patch symbol + offset : AT(_##segment##SegmentRomStart + symbol - _##segment##SegmentStart + offset)
    
#define CODE_PATCH(segment, symbol, offset) \
    .segment##symbol##offset##_patch symbol + offset : AT(segment##_ROM_START + symbol - segment##_VRAM + offset)

#define JAL_HOOK(segment, symbol, offset, helper, name) \
    .segment##symbol##offset##_patch symbol + offset : AT(segment##_ROM_START + symbol - segment##_VRAM + offset) \
    { \
        name = .; \
        BYTE(0x0C); \
        BYTE((helper >> 18) & 0xFF); \
        BYTE((helper >> 10) & 0xFF); \
        BYTE((helper >> 2)  & 0xFF); \
    }
#define J_NOP_HOOK(segment, symbol, offset, helper, name) \
    .segment##symbol##offset##_patch symbol + offset : AT(segment##_ROM_START + symbol - segment##_VRAM + offset) \
    { \
        name = .; \
        BYTE(0x08); \
        BYTE((helper >> 18) & 0xFF); \
        BYTE((helper >> 10) & 0xFF); \
        BYTE((helper >> 2)  & 0xFF); \
        BYTE(0x00); \
        BYTE(0x00); \
        BYTE(0x00); \
        BYTE(0x00); \
    }

#define RET16_HOOK(segment, symbol, offset, retVal, name) \
    .segment##symbol##offset##_patch symbol + offset : AT(segment##_ROM_START + symbol - segment##_VRAM + offset) \
    { \
        name = .; \
        BYTE(0x03); \
        BYTE(0xE0); \
        BYTE(0x00); \
        BYTE(0x08); \
        BYTE(0x24); \
        BYTE(0x02); \
        BYTE((retVal >> 8) & 0xFF); \
        BYTE((retVal >> 0) & 0xFF); \
    }
    
#define ROM_PATCH(address, offset) \
    .rom##address##offset##_patch : AT(address + offset)
