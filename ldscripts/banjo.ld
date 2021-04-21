#include <linking.h>

#define OVERLAY(ovl) \
    ovl##_custom_ROM_START = .; \
    BUILD_DIR/ovl.rzip.o(.data); \
    ovl##_custom_ROM_END = .;

SECTIONS
{
    __romPos = 0;

    BEGIN_SEG(baserom, 0)
    {
        BASEROM;
    }
    END_SEG(baserom)

    baseromEnd = __romPos;

    CODE_PATCH(boot_bk_boot, func_80000450, 0x28)
    {
        BUILD_DIR/src/patches/core1_load.o(.text);
    }

    CODE_PATCH(boot_bk_boot, overlay_table_init, 0x0)
    {
        BUILD_DIR/src/patches/overlay_table.o(.text);
    }

    CODE_PATCH(boot_bk_boot, __osLeoInterrupt, 0x0)
    {
        BUILD_DIR/src/patches/dma_code.o(.text);
    }
    
    ROM_PATCH(assets_ROM_START, 0x8)
    {
        BUILD_DIR/core1crc.o(.data);
    }

    . = soundfont2_ROM_END;
    ROM_PATCH(soundfont2_ROM_END, 0x0)
    {
        OVERLAY(core1);
        OVERLAY(core2);
        OVERLAY(CC);
        OVERLAY(MMM);
        OVERLAY(GV);
        OVERLAY(TTC);
        OVERLAY(MM);
        OVERLAY(BGS);
        OVERLAY(RBB);
        OVERLAY(FP);
        OVERLAY(SM);
        OVERLAY(cutscenes);
        OVERLAY(lair);
        OVERLAY(fight);
        OVERLAY(CCW);
    }

    __romPos = .;

    ROM_PATCH(0x0, 0x000000)
    {
        BUILD_DIR/src/patches/header.o(.data);
    }
    
    extRamStart = 0xFFFFFFFF80400000;
    . = extRamStart;

#include "custom.inc.ld"


    /* Discard everything not specifically mentioned above. */
    /DISCARD/ :
    {
        *(*);
    }
}