include ../../../makefile.prefix
CFLAGS += -g -O0 -Wall -fPIC

INC += -I. -I../../../core/include -I../../../proto -I./Processer -I../../common
LIB += -L../../../libs ../../../libs/libreactor.a ../../../libs/libcommon.a ../../../libs/libproto.a /usr/local/lib/libprotobuf.a -lssl -lcrypto
LIB += /usr/local/lib/libhiredis.a

Svrd = WSSConnectSvrd
#Prefix = Trunk
TARGET = ${Prefix}${Svrd} 

CPP_FILES = $(wildcard *.cpp) $(wildcard Processer/*.cpp)
C_FILES = $(wildcard *.c)
OBJ = $(patsubst %.cpp,%.o,$(CPP_FILES))
OBJ += $(patsubst %.c,%.o,$(C_FILES))

all :   $(TARGET)
include makefile.share