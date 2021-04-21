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

    /* RET16_HOOK(code, func_80386780, 0x0, 1, APHook1)
    RET16_HOOK(code, func_803866CC, 0x0, 1, APHook2) */

    /* Discard everything not specifically mentioned above. */
    /DISCARD/ :
    {
        *(*);
    }
}
