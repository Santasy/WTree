CPP=g++ -std=c++17 -Wno-cpp -fmax-errors=1
FLAGS=-O3 -Wall -DVERBOSE
PROFILEFLAGS=-g -O0 -Wall -DVERBOSE
LWTREE=../wtree/*.cpp
WTO=WTree*.o

all: clean objects clean_o remote_all

objects: $(LWTREE)
	@$(CPP) $(FLAGS) -c $(LWTREE)
	@ar rc WTree.a $(WTO)
	@printf "\t[OK] WTree.a was created.\n\n"

remote_all:
	@cd Rules_Test && make -s remote && ./rules_test.out | grep "working correctly"
	@cd Search_Test && make -s remote && ./search_test.out | grep "working correctly"
	@cd Remove_Test && make -s remote && ./remove_test.out | grep "working correctly"

singleoperations: SingleOperations.cpp
	@$(CPP) $(FLAGS) SingleOperations.cpp $(LWTREE) -o singleoperations.out
	@echo "Compilacion terminada"
	@echo

dbg_singleoperations: SingleOperations.cpp
	$(CPP) $(FLAGS) -ggdb SingleOperations.cpp $(LWTREE) -o singleoperations.out
	@echo "Compilacion para debug terminada"
	@echo

fullprof_singleoperations: SingleOperations.cpp
	(valgrind -s --leak-check=full --show-reachable=yes --show-possibly-lost=yes \
	--undef-value-errors=yes --partial-loads-ok=yes --log-file="_prof_output.txt" \
	./prof_singleoperations.out $(I) $(S))

prof_singleoperations: SingleOperations.cpp
	(valgrind -s --leak-check=no --show-reachable=no --show-possibly-lost=no \
	--undef-value-errors=no --partial-loads-ok=no --log-file="_prof_output.txt" \
	./prof_singleoperations.out $(I) $(S)) && \
	(python3 ProfParser.py $(I) $(S)) && \
	(rm _prof_output.txt)

clean:
	@rm -f *.out *.o *.a

clean_o:
	@rm -f *.o