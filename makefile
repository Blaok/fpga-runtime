.PHONY: all packages install uninstall
PREFIX ?= /usr/local
all:
	$(MAKE) -C src

packages:
	$(MAKE) -C packages

INSTALLED_FILES = include/fpga-runtime.h include/opencl-errors.h lib/libfpga-runtime.a

install: all
	$(foreach FILE,$(INSTALLED_FILES),install $(FILE) $(PREFIX)/$(FILE);)

uninstall:
	rm $(foreach FILE,$(INSTALLED_FILES),$(PREFIX)/$(FILE))
