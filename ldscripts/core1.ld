#include <linking.h>

SECTIONS
{
    __romPos = 0;

    BEGIN_SEG(baserom, 0)
    {
        BASEROM;
    }
    END_SEG(baserom)
    baseromEnd = __romPos;
    extRamStart = 0xFFFFFFFF80400000;
    . = extRamStart;

    /* CODE_PATCH(code, __osLeoInterrupt, 0x0)
    {
        BUILD_DIR/src/patches/dma_code.o(.text);
    } */

    J_NOP_HOOK(code, func_80253DE0, 0x0, custom_drawing, custom_drawing_hook)

#include "custom.inc.ld"

    /* Discard everything not specifically mentioned above. */
    /DISCARD/ :
    {
        *(*);
    }
}
