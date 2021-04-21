TARGET := banjo

OVERLAYS := core1 core2 CC MMM GV TTC MM BGS RBB FP CCW SM cutscenes lair fight

# Directories
SRC_DIR := src
BUILD_DIR := build
LD_DIR := ldscripts

SRC_DIRS := $(shell find $(SRC_DIR)/ -type d)
BUILD_SRC_DIRS := $(addprefix $(BUILD_DIR)/,$(SRC_DIRS))

# Tools
CROSS := mips-n64-

CC         := $(CROSS)gcc
AS         := $(CROSS)gcc
LD         := $(CROSS)ld
CPP        := $(CROSS)cpp
OBJCOPY    := $(CROSS)objcopy
MKDIR      := mkdir -p
RMDIR      := rm -rf
CP         := cp
CKSUM      := tools/n64crc
BANJOCRC   := tools/banjocrc
PYTHON     := python3
BANJOSPLIT := $(PYTHON) tools/banjosplit.py
BK_TOOLS   := tools/bk_tools
BK_DEFLATE := $(BK_TOOLS)/bk_deflate_code

# Inputs/outputs
ELF := $(BUILD_DIR)/$(TARGET).elf
Z64 := $(ELF:.elf=.z64)
OVL_ELFS_IN := $(addprefix elf/,$(addsuffix .elf,$(OVERLAYS)))
OVL_ELFS_OUT := $(addprefix $(BUILD_DIR)/,$(addsuffix .elf,$(OVERLAYS)))
ELF_IN := elf/$(TARGET).us.v10.elf
Z64_IN := $(BUILD_DIR)/$(TARGET)_in.z64
Z64_IN_OBJ := $(Z64_IN:.z64=.o)
LD_SCRIPT   := $(LD_DIR)/$(TARGET).ld
OVL_LD_SCRIPTS := $(filter-out $(LD_SCRIPT),$(wildcard $(LD_DIR)/*.ld))
OVL_LD_SCRIPTS_CPP := $(addprefix $(BUILD_DIR)/,$(notdir $(OVL_LD_SCRIPTS)))
CORE1CRC_BIN := $(BUILD_DIR)/core1crc.bin
CORE1CRC_OBJ := $(CORE1CRC_BIN:.bin=.o)
OVL_BINS_IN := $(addprefix $(BUILD_DIR)/,$(addsuffix _in.bin,$(OVERLAYS)))
OVL_OBJS_IN := $(addprefix $(BUILD_DIR)/,$(addsuffix _in.o,$(OVERLAYS)))
OVL_BINS  := $(addprefix $(BUILD_DIR)/,$(addsuffix .bin,$(OVERLAYS)))
OVL_CODES := $(addprefix $(BUILD_DIR)/,$(addsuffix .code,$(OVERLAYS)))
OVL_DATAS := $(addprefix $(BUILD_DIR)/,$(addsuffix .data,$(OVERLAYS)))
OVL_RZIPS     := $(addprefix $(BUILD_DIR)/,$(addsuffix .rzip,$(OVERLAYS)))
OVL_RZIP_OBJS := $(addprefix $(BUILD_DIR)/,$(addsuffix .rzip.o,$(OVERLAYS)))

C_SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
C_OBJS := $(addprefix $(BUILD_DIR)/, $(C_SRCS:.c=.o))
A_SRCS := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.s))
A_OBJS := $(addprefix $(BUILD_DIR)/, $(A_SRCS:.s=.o))

# List of overlays that need to be linked
OVLS_TO_BUILD := $(basename $(notdir $(OVL_LD_SCRIPTS)))
OVLS_TO_COPY  := $(filter-out $(OVLS_TO_BUILD),$(OVERLAYS))
ELFS_TO_BUILD := $(addprefix $(BUILD_DIR)/,$(addsuffix .elf,$(OVLS_TO_BUILD)))
ELFS_TO_COPY  := $(addprefix $(BUILD_DIR)/,$(addsuffix .elf,$(OVLS_TO_COPY)))

OBJS := $(C_OBJS) $(A_OBJS)

# Flags
CFLAGS      := -c -mabi=32 -ffreestanding -mfix4300 -G 0 -fno-zero-initialized-in-bss -Wall -Wextra -Wpedantic
CPPFLAGS    := -Iinclude -I../../bk/bk/include -I../../bk/bk/include/2.0L/ -DF3DEX_GBI -D_LANGUAGE_C
OPTFLAGS    := -Os
ASFLAGS     := -c -x assembler-with-cpp -mabi=32 -ffreestanding -mfix4300 -G 0 -O -Iinclude
LDFLAGS     := -mips3 --accept-unknown-input-arch --no-check-sections
CPP_LDFLAGS := -P -Wno-trigraphs -DBUILD_DIR=$(BUILD_DIR) -Umips -Iinclude
BINOFLAGS   := -I binary -O elf32-big
Z64OFLAGS   := -O binary

# Rules
all: $(Z64)

$(BUILD_DIR) $(BUILD_SRC_DIRS) :
	$(MKDIR) $@

$(BUILD_DIR)/%.o : %.c | $(BUILD_SRC_DIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OPTFLAGS) $< -o $@

$(BUILD_DIR)/%.o : %.s | $(BUILD_SRC_DIRS)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(notdir $(LD_SCRIPT)) :  $(LD_SCRIPT)
	$(CPP) $(CPP_LDFLAGS) -DBASEROM=$(Z64_IN_OBJ) $< -o $@

$(ELF) : $(OBJS) $(Z64_IN_OBJ) $(BUILD_DIR)/$(TARGET).ld $(ELF_IN) $(CORE1CRC_OBJ) $(OVL_RZIP_OBJS)
	$(LD) -R $(ELF_IN) $(LDFLAGS) -T $(BUILD_DIR)/$(TARGET).ld -Map $(@:.elf=.map) -o $@
	
$(Z64_IN) : $(ELF_IN) | $(BUILD_DIR)
	$(OBJCOPY) $(Z64OFLAGS) $< $@

$(Z64_IN_OBJ) : $(Z64_IN) | $(BUILD_DIR)
	$(OBJCOPY) $(BINOFLAGS) $< $@
	
$(Z64) : $(ELF) $(CKSUM)
	$(OBJCOPY) $(Z64OFLAGS) $< $@
	$(CKSUM) $@
	
$(OVL_BINS_IN) : $(BUILD_DIR)/%_in.bin : elf/%.elf | $(BUILD_DIR)
	$(OBJCOPY) $(Z64OFLAGS) $< $@

$(OVL_OBJS_IN) : $(BUILD_DIR)/%_in.o : $(BUILD_DIR)/%_in.bin | $(BUILD_DIR)
	$(OBJCOPY) $(BINOFLAGS) $< $@

$(OVL_LD_SCRIPTS_CPP) : $(BUILD_DIR)/%.ld : $(LD_DIR)/%.ld
	$(CPP) $(CPP_LDFLAGS) -DOVERLAY=$* -DBASEROM=$(BUILD_DIR)/$*_in.o $< -o $@

$(ELFS_TO_COPY) : $(BUILD_DIR)/%.elf : elf/%.elf
	$(CP) $< $@

$(ELFS_TO_BUILD) : $(BUILD_DIR)/%.elf : $(BUILD_DIR)/%_in.o $(BUILD_DIR)/%.ld $(OBJS)
	$(LD) -R elf/$*.elf $(LDFLAGS) -T $(BUILD_DIR)/$*.ld -Map $(@:.elf=.map) -o $@
	
$(OVL_BINS) : $(BUILD_DIR)/%.bin : $(BUILD_DIR)/%.elf
	$(OBJCOPY) $(Z64OFLAGS) $< $@

# This can't use objcopy because it wouldn't copy in any overwritten code
# Therefore, we objcopy the whole file and split it into code/data with a python script
$(OVL_CODES) : $(BUILD_DIR)/%.code : $(BUILD_DIR)/%.bin
	$(BANJOSPLIT) $< $(BUILD_DIR)/$*.elf $@ $(@:.code=.data) --nm mips-n64-nm

# Dummy target since banjosplit makes both code and data bins
$(OVL_DATAS) : $(BUILD_DIR)/%.data : $(BUILD_DIR)/%.code
	@printf ""

$(OVL_RZIPS) : $(BUILD_DIR)/%.rzip : $(BUILD_DIR)/%.code $(BUILD_DIR)/%.data
	@cd $(BK_TOOLS) && ../../$(BK_DEFLATE) ../../$@ ../../$(BUILD_DIR)/$*.code ../../$(BUILD_DIR)/$*.data

$(OVL_RZIP_OBJS) : $(BUILD_DIR)/%.rzip.o : $(BUILD_DIR)/%.rzip
	$(OBJCOPY) $(BINOFLAGS) $< $@

$(CORE1CRC_OBJ) : $(CORE1CRC_BIN)
	$(OBJCOPY) $(BINOFLAGS) $< $@

$(CORE1CRC_BIN) : $(BANJOCRC) $(BUILD_DIR)/core1.code $(BUILD_DIR)/core1.data
	$^ > $@

$(CKSUM) : $(CKSUM).c
	gcc -O3 $< -o $@ -Wall -Wextra -Wpedantic

$(BANJOCRC) : $(BANJOCRC).c
	gcc -O3 $< -o $@ -Wall -Wextra -Wpedantic

ovl-codes: $(OVL_CODES)
$(BUILD_DIR)/core1.ld : $(LD_DIR)/custom.inc.ld
$(BUILD_DIR)/banjo.ld : $(LD_DIR)/custom.inc.ld
$(BUILD_DIR)/banjo.ld : CPP_LDFLAGS += -DCUSTOM_LOAD

clean:
	$(RMDIR) $(BUILD_DIR)

.PHONY: all clean

print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true

