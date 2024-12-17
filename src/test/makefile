NVER?=v000
COMPILATION_FLAGS=
OUT_SUFFIX=

ifdef TARGET_NODE_BYTES
	COMPILATION_FLAGS= -DWTREE_TARGET_NODE_BYTES $(TARGET_NODE_BYTES)
	OUT_SUFFIX=_$(TARGET_NODE_BYTES)
endif

CPP = g++ -std=c++17 -Wall -Wno-unknown-pragmas $(COMPILATION_FLAGS)
FLAGS = -fmax-errors=10 -O3 -DNDEBUG -Wno-array-bounds -Wno-unused-variable
PROFILEFLAGS = -g -O0 -DDEBUG -DDBGVERBOSE -Wno-unused-but-set-variable -Wno-unused-variable

# Folders and sources
WTFOLDER = ../wtree/

# Base WTree sources
SHARED_SRCS = WTree.cpp WTree_node.cpp WTree_base_insert.cpp \
              WTree_remove.cpp WTree_print.cpp WTree_utils.cpp
BASEWTREE_SRCS = $(SHARED_SRCS) WTree_search.cpp
BASEWTREE_SRCS_BS = $(SHARED_SRCS) WTree_search_bs.cpp

# Full and alternative implementations
LIBWTREE = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert.cpp)

WTO=WTree.o WTree_print.o WTree_node.o \
	WTree_insert.o WTree_base_insert.o WTree_search.o WTree_utils.o WTree_remove.o
BWTO = $(addprefix "./build/", $(WTO))

all: clean objects clean_o remote_all

objects: $(WTFOLDER)
	@$(CPP) $(FLAGS) -c $(LIBWTREE)
	@mv *.o ./build/
	@ar rc ./build/WTree.a $(BWTO)
	@printf "\t[OK] WTree.a was created.\n\n"

remote_all:
	@cd Rules_Test && make -s remote && ./rules_test.out | grep "working correctly"
# @cd Search_Test && make -s remote && ./search_test.out | grep "working correctly"
# @cd Remove_Test && make -s remote && ./remove_test.out | grep "working correctly"

clean:
	@rm -f *.out ./build/*.o ./build/*.a

clean_o:
	@rm -f ./build/*.o