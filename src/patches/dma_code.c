#include <ultra64.h>

extern u8 extRamStart[];

extern u8 _customSegmentRomStart[];
extern u8 _customSegmentSize[];
extern u8 _customSegmentNoloadStart[];
extern u8 _customSegmentNoloadEnd[];

extern u32 __osDisableInt(void);
extern void __osRestoreInt(u32);

void dma_code()
{
    u32 saveMask = __osDisableInt();
    osPiRawStartDma(OS_READ, (u32)_customSegmentRomStart, extRamStart, (u32)_customSegmentSize);
    while(osPiGetStatus() & PI_STATUS_DMA_BUSY);
    __osRestoreInt(saveMask);
    bzero(_customSegmentNoloadStart, _customSegmentNoloadEnd - _customSegmentNoloadStart);
}
