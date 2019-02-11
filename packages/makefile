libfpga-runtime.deb: ../src/*.h ../src/*.cpp DEBIAN/control
	$(MAKE) -C ../src
	mkdir -p $(basename $@)/usr
	cp -r ../{include,lib} $(basename $@)/usr
	cp -r DEBIAN $(basename $@)/
	dpkg --build $(basename $@)
	rm -rf $(basename $@)
