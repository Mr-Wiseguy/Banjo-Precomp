ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    CC = x86_64-w64-mingw32-gcc
else
    CC = gcc
endif
CXX = g++
CXXFLAGS = -std=c++2a

SRCS = bk_asm.cpp bk_asset.cpp file_helper.cpp
OBJS = $(SRCS:.cpp=.o)

LIB_DIR = lib
LIBS = -ldeflate
bk_extract bk_splat_yaml: LIBS = -lcrypto -ldeflate -lyaml-cpp
bk_inflate_code: LIBS = -lcrypto -ldeflate

TARGETS = bk_extract bk_assets_build bk_deflate_code bk_inflate_code
DEPEND = $(LIB_DIR)/libdeflate.a gzip-1.2.4/gzip $(LIB_DIR)/libyaml-cpp.a
SUBMODULES = libdeflate yaml-cpp gzip-1.2.4

default: all
all: $(TARGETS) bk_splat_yaml

$(TARGETS): $(LIB_DIR)/libdeflate.a gzip-1.2.4/gzip $(OBJS)
	g++ -o $@ $@.cpp $(OBJS) $(CXXFLAGS) -L$(LIB_DIR) $(LIBS)

bk_splat_yaml: $(DEPEND) $(OBJS) $(LIB_DIR)/libyaml-cpp.a
	g++ -o $@ $@.cpp $(OBJS) $(CXXFLAGS) -L$(LIB_DIR) $(LIBS) -Iyaml-cpp/include

clean:
	rm -f *.o
	rm -f $(TARGETS)

very_clean: $(foreach mod, $(SUBMODULES), $(mod)_clean) clean
	cd $(LIB_DIR) && rm -f *.a

$(LIB_DIR)/libdeflate.a: $(LIB_DIR) libdeflate/libdeflate.a
	cp -f libdeflate/libdeflate.a $(LIB_DIR)

libdeflate/libdeflate.a:
	cd libdeflate && make libdeflate.a CC=$(CC)

gzip-1.2.4/gzip: gzip-1.2.4/Makefile
	cd gzip-1.2.4 && make gzip

gzip-1.2.4/Makefile:
	cd gzip-1.2.4 && ./configure

$(LIB_DIR)/libyaml-cpp.a: $(LIB_DIR) yaml-cpp/build/libyaml-cppa.a
	cp -f yaml-cpp/build/libyaml-cpp.a $(LIB_DIR)

yaml-cpp/build/libyaml-cppa.a: yaml-cpp/build yaml-cpp/build/Makefile
	cd yaml-cpp/build && make

yaml-cpp/build/Makefile:
	cd yaml-cpp/build && cmake ..

yaml-cpp/build:
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  -o $@

%_clean:
	cd $* && make clean

yaml-cpp_clean:
	cd yaml-cpp && rm -rf build
