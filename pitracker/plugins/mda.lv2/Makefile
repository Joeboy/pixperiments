export CXX=arm-none-eabi-g++
export CXXFLAGS = -Wall -O2 -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfloat-abi=hard -mfpu=vfp -ffast-math


buildit :
	./waf configure --lv2dir=$(INSTALL_DIR)
	./waf build

clean :
	./waf clean

install : buildit
	./waf install

