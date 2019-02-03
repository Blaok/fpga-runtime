APP ?= vadd
KERNEL_NAME ?= VecAdd
KERNEL_SRC ?= test-fpga-kernel.cpp
HOST_SRCS ?= test-fpga-runtime.cpp
PLATFORM ?= xilinx_u200_xdma_201830_1

XOCC ?= xocc
TARGET ?= sw_emu
CXXFLAGS += -std=c++11 -O3 -I../include -I$(XILINX_XRT)/include
LDFLAGS += -L../lib -L$(XILINX_XRT)/lib
LDFLAGS += -lfpga-runtime -ltinyxml -lxilinxopencl

test: $(APP) $(APP).$(PLATFORM).$(TARGET).xclbin
	./$(APP) $(APP).$(PLATFORM).$(TARGET).xclbin 1000

$(APP): $(HOST_SRCS) $(KERNEL_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(APP).$(PLATFORM).$(TARGET).xo: $(KERNEL_SRC)
	$(XOCC) -t $(TARGET) -c $< -o $@ \
	  --platform $(PLATFORM) --kernel $(KERNEL_NAME) \
	  --xp prop:kernel.$(KERNEL_NAME).kernel_flags="-std=c++11"

$(APP).$(PLATFORM).$(TARGET).xclbin: $(APP).$(PLATFORM).$(TARGET).xo
	$(XOCC) -t $(TARGET) -l $< -o $@ --platform $(PLATFORM) \
	  --max_memory_ports $(KERNEL_NAME) \
	  --nk $(KERNEL_NAME):1:$(KERNEL_NAME) \
	  --sp $(KERNEL_NAME).m_axi_gmem0:DDR[0] \
	  --sp $(KERNEL_NAME).m_axi_gmem1:DDR[1] \
	  --sp $(KERNEL_NAME).m_axi_gmem2:DDR[2]

emconfig.json:
	emconfigutil --platform $(PLATFORM)
