# This is used to identify your application
APP ?= vadd

# This is the kernel name in your HLS code
KERNEL_NAME ?= VecAdd

# Only 1 kernel file is allowed and it cannot include any customized header
KERNEL_SRC ?= test-fpga-kernel.cpp

# Multiple files allowed; headers will be filtered out for compilation
HOST_SRCS ?= test-fpga-runtime.cpp

# Platform name
PLATFORM ?= xilinx_u200_xdma_201830_1

# Externally set CXXFLAGS and LDFLAGS are honored
CXXFLAGS += -std=c++11 -O3
LDFLAGS += -lfpga-runtime -ltinyxml -lxilinxopencl

# DRAM map mappings
DRAM_MAPS ?= gmem0:DDR[0] gmem1:DDR[1] gmem2:DDR[2]

# Host arguments
HOST_ARGS ?= 1000

# DO NOT MODIFY UNLESS YOU KNOW WHAT YOU ARE DOING
MAKE_NO_PRINT_DIR := $(MAKE) --no-print-directory
XOCC ?= xocc
TARGET ?= sw_emu
XO_NAME := $(APP).$(PLATFORM).$(subst hw_emu,hw,$(TARGET)).xo
XCLBIN_NAME := $(APP).$(PLATFORM).$(TARGET).xclbin
XOCC_FLAGS := --platform $(PLATFORM)
XOCC_CFLAGS := -t $(subst hw_emu,hw,$(TARGET)) --kernel $(KERNEL_NAME)
XOCC_CFLAGS += --xp prop:kernel.$(KERNEL_NAME).kernel_flags="-std=c++11"
XOCC_LDFLAGS := -t $(TARGET) --max_memory_ports $(KERNEL_NAME)
XOCC_LDFLAGS += --nk $(KERNEL_NAME):1:$(KERNEL_NAME)
XOCC_LDFLAGS += $(foreach map,$(DRAM_MAPS),--sp $(KERNEL_NAME).m_axi_$(map))

.PHONY: csim cosim hw exe hls bitstream test
# Action aliases
csim:
	@$(MAKE_NO_PRINT_DIR) TARGET=sw_emu test

cosim:
	@$(MAKE_NO_PRINT_DIR) TARGET=hw_emu test

hw:
	@$(MAKE_NO_PRINT_DIR) TARGET=hw test

# Target aliases
exe: $(APP)

hls:
	@$(MAKE_NO_PRINT_DIR) TARGET=hw $(APP).$(PLATFORM).hw.xo

bitstream:
	@$(MAKE_NO_PRINT_DIR) TARGET=hw $(APP).$(PLATFORM).hw.xclbin

test: $(APP) $(XCLBIN_NAME)
	@touch .time
	./$(APP) $(XCLBIN_NAME) $(HOST_ARGS)
	@find *.log xocc* .run/ -newer .time -delete 2>/dev/null;exit 0
	@rm .time

$(APP): $(HOST_SRCS) $(KERNEL_SRC)
	$(CXX) $(CXXFLAGS) $(filter-out %.h,$^) -o $@ $(LDFLAGS)

$(XO_NAME): $(KERNEL_SRC)
	$(XOCC) $(XOCC_FLAGS) $(XOCC_CFLAGS) -c $< -o $@

$(XCLBIN_NAME): $(XO_NAME)
	$(XOCC) $(XOCC_FLAGS) $(XOCC_LDFLAGS) -l $< -o $@
