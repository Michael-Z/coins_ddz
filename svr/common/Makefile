
#memory check flag
#MEMCHECK=1

#gcc version
CC  = gcc
CXX = g++
GCCVER := $(shell $(CC) -dumpversion | awk -F. '{ print $$1"."$$2}' )

COMPILE_LIB_HOME=../../libs

INC     = -I. -I../../core/include -I../../proto -I../../libs
OPT     = -pipe -shared -fPIC -Wno-invalid-offsetof
LINK    = -lpthread /usr/local/lib/libhiredis.a ../../libs/json/libjson.a

CFLAGS += $(OPT) -g -pg -Wall -MMD 

CXXFLAGS := $(CFLAGS)

SRCS = $(wildcard *.cpp)
			
DYNAMIC_NAME = libcommon.so
STATIC_NAME = libcommon.a
DYNAMIC_LIB	= $(COMPILE_LIB_HOME)/$(DYNAMIC_NAME)
STATIC_LIB = $(COMPILE_LIB_HOME)/$(STATIC_NAME)

all: $(DYNAMIC_LIB) $(STATIC_LIB)

$(DYNAMIC_LIB): $(SRCS:.cpp=.o) 
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LINK) $(LIB)
	cp $(DYNAMIC_LIB) .
#	@-ln -sf $(DYNAMIC_NAME) $(COMPILE_LIB_HOME)/libcore.so

$(STATIC_LIB): $(SRCS:.cpp=.o)
	@ar cr $@ $^ $(LIB)
	cp $(STATIC_LIB) .	

%.o: %.c Makefile
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
#	@-mv -f $*.d .dep.$@
%.o: %.cc Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<
#	@-mv -f $*.d .dep.$@
%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<
#	@-mv -f $*.d .dep.$@
clean:
	rm -f *.o .po *.so *.d .dep.*  $(SRCS:.cpp=.o) $(SRCS:.cpp=.d) $(DYNAMIC_LIB) $(STATIC_LIB) $(DYNAMIC_NAME) $(STATIC_NAME)
-include /dev/null $(wildcard .dep.*) 
