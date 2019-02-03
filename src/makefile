.PHONY: all clang-tidy

CXXFLAGS += -std=c++11 -O3 -fPIC -I$(XILINX_XRT)/include

all: ../lib/libfpga-runtime.a ../include/fpga-runtime.h \
  ../include/opencl-errors.h

../lib/libfpga-runtime.a: ../obj/fpga-runtime.o
	@mkdir -p $(dir $@)
	ar rcs $@ $^

../obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MM -MP -MT $@ -MF $(@:.o=.d) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include ../obj/fpga-runtime.d

../include/%.h: %.h
	cp $< $@

clang-tidy: fpga-runtime.cpp
	clang-tidy $< -- $(CXXFLAGS)
