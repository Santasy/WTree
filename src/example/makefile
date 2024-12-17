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

ROUTINE_FILE = single_operations.cpp

# Folders and sources
WTFOLDER = ../wtree/

# Base WTree sources
SHARED_SRCS = WTree.cpp WTree_node.cpp WTree_base_insert.cpp \
              WTree_remove.cpp WTree_print.cpp WTree_utils.cpp
BASEWTREE_SRCS = $(SHARED_SRCS) WTree_search.cpp
BASEWTREE_SRCS_BS = $(SHARED_SRCS) WTree_search_bs.cpp

# WTree variants
LIBWTREE_BASIC = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert_basic.cpp)
LIBWTREE_SLIDE = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert_slide.cpp)
LIBWTREE_SPLIT = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert_split.cpp)
LIBWTREE_BOTH = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS) WTree_insert_both.cpp)
LIBWTREE_BS_SLIDE = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS_BS) WTree_insert_bs_slide.cpp)
LIBWTREE_BS_BOTH = $(addprefix $(WTFOLDER), $(BASEWTREE_SRCS_BS) WTree_insert_bs_both.cpp)

# Pattern rule for sop variants
define sop_template =
sop$(1): clean
	@printf " === Compiling Single Operations ($(1)) routine ===\n"
	$(CPP) $(FLAGS) $$(LIBWTREE$(2)) $(ROUTINE_FILE) -o versions/sop$(1)_$(NVER)$(OUT_SUFFIX).out
	@printf " === Compiled Single Operations ($(1)) routine ===\n"
endef

$(eval $(call sop_template,_basic,_BASIC))
$(eval $(call sop_template,_slide,_SLIDE))
$(eval $(call sop_template,_split,_SPLIT))
$(eval $(call sop_template,_both,_BOTH))
$(eval $(call sop_template,_bs_slide,_BS_SLIDE))
$(eval $(call sop_template,_bs_both,_BS_BOTH))

# Generic dbg target for all variants
define dbg_template
dbg$(1): clean
	@printf " === Compiling dbg full$(1) routine ===\n"
	$(CPP) $(PROFILEFLAGS) $$(LIBWTREE$(2)) $(ROUTINE_FILE) -o dbg.out
	@echo "Compiled DBG exe."
	@echo
endef

# Standard dbg and dbg_man variants
$(eval $(call dbg_template,_basic,_BASIC))
$(eval $(call dbg_template,_slide,_SLIDE))
$(eval $(call dbg_template,_split,_SPLIT))
$(eval $(call dbg_template,_both,_BOTH))
$(eval $(call dbg_template,_bs_slide,_BS_SLIDE))
$(eval $(call dbg_template,_bs_both,_BS_BOTH))

### Call commands ###
all: clean sop_bs_slide

variants: sop_basic sop_slide sop_split sop_both sop_bs_slide sop_bs_both

dbg_variants: dbg_basic dbg_slide dbg_split dbg_both dbg_bs_slide dbg_bs_both

clean:
	@rm -f *.out ./*.o ./*.a

clean_o:
	@rm -f ./*.o