#include <ultra64.h>
// #include <debug.h>

void drawSmallString(Gfx **dl, int x, int y, const char* string);
void drawSmallStringCol(Gfx **dl, int x, int y, const char* string, unsigned char r, unsigned char g, unsigned char b);

s32 hex_char_from_nibble(s32 val)
{
    if (val >= 0x0A)
    {
        return val + ('A' - 0x0A);
    }
    else
    {
        return val + '0';
    }
}

void schedule_gfx_task();

void custom_drawing(Gfx **gdl)
{
    Gfx *dlHead = *gdl;
    // char text[9];
    // text[0] = 'S';
    // text[1] = 'o';
    // text[2] = 'u';
    // text[3] = 'n';
    // text[4] = 'd';
    // text[5] = ':';
    // text[6] = hex_char_from_nibble((sound >> 4) & 0xF);
    // text[7] = hex_char_from_nibble((sound >> 0) & 0xF);
    // text[8] = '\0'; 
    
    gDPSetCycleType(dlHead++, G_CYC_1CYCLE);
    gDPSetRenderMode(dlHead++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
    gDPSetTexturePersp(dlHead++, G_TP_NONE);
    gDPSetTextureFilter(dlHead++, G_TF_POINT);
    gDPSetTextureLUT(dlHead++, G_TT_NONE);
    drawSmallStringCol(&dlHead, 80, 30, "Rom hecking ban joe", 255, 255, 255);
    gDPFullSync(dlHead++);
    gSPEndDisplayList(dlHead++);

    *gdl = dlHead;

    // debug_pollcommands();

    // nuGfxTaskStart(gfxList_ptr, (s32)(dlHead - gfxList_ptr) * sizeof(Gfx), ucode, flag);
}
