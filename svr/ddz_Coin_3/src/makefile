include ../../../makefile.prefix
CFLAGS += -g -O0 -Wall -fPIC

CXXFLAGS += -std=c++0x -Wall -g -pipe -rdynamic -fno-strict-aliasing -Wno-unused-function -Wno-sign-compare -fpermissive -Wno-invalid-offsetof

INC += -I. -I../../../core/include -I../../../proto -I./Processer -I../../common
LIB += -L../../../libs ../../../libs/libcommon.a ../../../libs/libproto.a ../../../libs/libreactor.a /usr/local/lib/libprotobuf.a

LIB += /usr/local/lib/libhiredis.a

Svrd = DdzChuangGuanSvrd
#Prefix = Trunk
TARGET = ${Prefix}${Svrd} 

CPP_FILES = $(wildcard *.cpp) $(wildcard Processer/*.cpp)
C_FILES = $(wildcard *.c)
OBJ = $(patsubst %.cpp,%.o,$(CPP_FILES))
OBJ += $(patsubst %.c,%.o,$(C_FILES))

all :   $(TARGET)
include makefile.share
