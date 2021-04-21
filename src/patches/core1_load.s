#include <macro.inc>
.set noat
.set noreorder
.section .text

core1_load_patch:
    lui        $v0, %hi(core1_custom_ROM_START)
    lui        $t8, %hi(core1_custom_ROM_END)
    addiu      $a1, $v0, %lo(core1_custom_ROM_START)
    addiu      $t8, $t8, %lo(core1_custom_ROM_END)
