DEP_DIR := ./objdep
$(shell mkdir -p $(DEP_DIR))

PROJECT_DIR := ..
LIB_DIR 	:= $(PROJECT_DIR)/libs
TOOL_PROTOC   := /usr/local/bin/protoc

LIB = $(LIB_DIR)/libproto.a

CXXFLAGS += -std=c++0x -Wall -g -pipe -rdynamic -fno-strict-aliasing -Wno-unused-function -Wno-sign-compare -fpermissive 

PB_OBJ = \
		$(DEP_DIR)/poker_common.o	\
		$(DEP_DIR)/poker_data.o	\
		$(DEP_DIR)/poker_config.o \
		$(DEP_DIR)/poker_msg_basic.o	\
		$(DEP_DIR)/poker_msg_cs.o	\
		$(DEP_DIR)/poker_msg_ss.o	\
		$(DEP_DIR)/poker_msg_log.o	\
		$(DEP_DIR)/poker_msg.o	\

INC_FLAGS =  -I$(PROJECT_DIR)/proto \

$(LIB) :  $(PB_OBJ) 
	ar rcs $@ $^
	
$(PB_OBJ) :$(DEP_DIR)/%.o : %.proto
	$(TOOL_PROTOC) $< --cpp_out=./
	g++ $(CXXFLAGS) $(INC_FLAGS) -c -o $@ $(patsubst %.proto, %.pb.cc, $<)

clean:
	$(RM) $(LIB) $(DEP_DIR) -rf 

.PHONY: protbuf

# vim:ts=4:sw=4:ft=make:
