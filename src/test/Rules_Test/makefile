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
WTFOLDER = ../../wtree/

# Base WTree sources
SHARED_SRCS = WTree.cpp WTree_node.cpp WTree_base_insert.cpp \
              WTree_remove.cpp WTree_print.cpp WTree_utils.cpp
BASEWTREE_SRCS = $(SHARED_SRCS) WTree_search.cpp
BASEWTREE_SRCS_BS = $(SHARED_SRCS) WTree_search_bs.cpp

# Full and alternative implementations
LIBWTREE = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert.cpp)
all: clean rules

rules: Rules_Test.cpp
	$(CPP) $(FLAGS) Rules_Test.cpp $(LIBWTREE) -o rules_test.out
	@echo "Compilacion terminada"
	@echo

remote: clean Rules_Test.cpp
	@$(CPP) $(FLAGS) Rules_Test.cpp ../build/WTree.a -o rules_test.out

dbg: Rules_Test.cpp
	$(CPP) $(PROFILEFLAGS) Rules_Test.cpp -ggdb $(LIBWTREE) -o dbg.out
	@echo "Compilacion para debug terminada"
	@echo

clean:
	@(rm *.out || true)