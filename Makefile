SHELL=/bin/bash
OPENMP=yes
.SUFFIXES:
default:

.PHONY: create-labs

STUDENT_EDITABLE_FILES=markov_solution.hpp config.make
PRIVATE_FILES=Assignment.key.ipynb admin .git solution bad-solution

COMPILER=$(CXX) 
MICROBENCH_OPTIMIZE= -DHAVE_LINUX_PERF_EVENT_H -I$(PWD) -g $(C_OPTS)
LIBS= -lm -pthread -lboost_program_options -L/usr/lib/ -lboost_system -ldl
BUILD=build/

OPTIMIZE+=-march=x86-64
COMPILER=g++-9
include config.make
OPENMP?=no
ifeq ($(OPENMP),yes)
OPENMP_OPTS=-fopenmp
OPENMP_LIBS=-lgomp
else
OPENMP_OPTS=
OPENMP_LIBS=
endif
.PHONY: autograde
autograde: markov.exe 
	./markov.exe -M 4000 -o bench.csv -scale 8192 -days 128 -i 1 -f markov_reference_c markov_solution_c
	./markov.exe -M 4000 -o correctness.csv -v -scale 1024 -days 1 4 25  -i 1 -f markov_solution_c

.PRECIOUS: $(BUILD)%.cpp
.PRECIOUS: $(BUILD)%.hpp

$(BUILD)perfstats.o: perfstats.c perfstats.h
	mkdir -p $(BUILD) 
	cp  $< $(BUILD)$<
	$(COMPILER) -DHAVE_LINUX_PERF_EVENT_H -O3 -I$(PWD) $(LIBS) -o $(BUILD)perfstats.o -c $(BUILD)perfstats.c


$(BUILD)%.s: $(BUILD)%.cpp
	mkdir -p $(BUILD) 
#	cp  $< $(BUILD)$<
	$(COMPILER) $(MICROBENCH_OPTIMIZE) $(LIBS) -S $(BUILD)$*.cpp

$(BUILD)%.so: $(BUILD)%.cpp
	mkdir -p $(BUILD) 
	cp *.hpp $(BUILD)
	cp *.h   $(BUILD)
	$(COMPILER)  -DHAVE_LINUX_PERF_EVENT_H $(MICROBENCH_OPTIMIZE) $(OPENMP_OPTS) $(OPENMP_LIBS) $(LIBS) -rdynamic -fPIC -shared -o $(BUILD)$*.so $(BUILD)$*.cpp
#	$(COMPILER) $(MICROBENCH_OPTIMIZE) $(LIBS) $(OPENMP_OPTS) $(OPENMP_LIBS) -c -fPIC -o $(BUILD)$*.o $(BUILD)$*.cpp

$(BUILD)%.cpp: %.cpp
	cp $< $(BUILD)
	cp *.hpp $(BUILD)

markov.exe: $(BUILD)markov_main.o  $(BUILD)markov.o $(BUILD)perfstats.o
	$(COMPILER) $(markov_OPTIMIZE) $(OPENMP_OPTS) $(OPENMP_LIBS) $(MICROBENCH_OPTIMIZE) $(BUILD)markov_main.o  $(BUILD)perfstats.o $(BUILD)markov.o -o markov.exe

$(BUILD)run_tests.o : OPTIMIZE=-O3

$(BUILD)%.o: %.cpp
	mkdir -p $(BUILD) 
	cp  $< $(BUILD)$<
	$(COMPILER)  -DHAVE_LINUX_PERF_EVENT_H $(OPENMP_OPTS) $(OPENMP_LIBS) $(MICROBENCH_OPTIMIZE) $(LIBS) -o $(BUILD)$*.o -c $(BUILD)$*.cpp

$(BUILD)markov.o : OPTIMIZE=$(markov_OPTIMIZE)
$(BUILD)markov.s : OPTIMIZE=$(markov_OPTIMIZE)
$(BUILD)markov_main.o : OPTIMIZE=$(markov_OPTIMIZE)

fiddle.exe:  $(BUILD)fiddle.o $(FIDDLE_OBJS) $(BUILD)perfstats.o
	$(COMPILER) $(MICROBENCH_OPTIMIZE) $(OPENMP_OPTS) $(OPENMP_LIBS) -DHAVE_LINUX_PERF_EVENT_H $(BUILD)fiddle.o $(BUILD)perfstats.o $(FIDDLE_OBJS) $(LIBS) -o fiddle.exe
#fiddle.exe: EXTRA_LDFLAGS=-pg
#$(BUILD)fiddle.o : OPTIMIZE=-O3 -pg


#-include $(DJR_JOB_ROOT)/$(LAB_SUBMISSION_DIR)/config.make
clean: 
	rm -f *.exe $(BUILD)*
