
OBJS=main.o compute.o task.o pparse.o planner.o
TARGET=planner
CXXFLAGS=-Isrc -g -lyaml-cpp -lboost_program_options -Wall -Werror
HEADERS=compute.h task.h pparse.h identity.h planner.h
TEST_DIR=../test
INPUT_DIR=$(TEST_DIR)/input

.PHONY: test clean vg all_tests test_one_comp test_insufficient_comp test_circular_task_dep test_small_task_input test_med_task_input test_large_task_input 

# development-mode target
test: $(TARGET)
	./$(TARGET) --analyze

# test targets
all_tests: test test_one_comp test_insufficient_comp test_circular_task_dep test_small_task_input test_med_task_input test_large_task_input  test_no_deps_tasks

test_small_task_input: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/small_tasks.yaml --compute $(INPUT_DIR)/small_compute.yaml --analyze

test_med_task_input: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/med_tasks.yaml --compute $(INPUT_DIR)/med_compute.yaml --analyze

test_large_task_input: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/large_tasks.yaml --compute $(INPUT_DIR)/large_compute.yaml --analyze

test_circular_task_dep: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/circ_dep.yaml --compute $(INPUT_DIR)/compute01.yaml || exit 0

test_insufficient_comp: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/tasks01.yaml --compute $(INPUT_DIR)/insufficient_compute.yaml || exit 0

test_one_comp: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/single_compute_tasks.yaml --compute $(INPUT_DIR)/single_compute.yaml --analyze

test_no_deps_tasks:
	./$(TARGET) --tasks $(INPUT_DIR)/no_dep_tasks.yaml --compute $(INPUT_DIR)/compute01.yaml

test_large_task_compare: $(TARGET)
	./$(TARGET) --tasks $(INPUT_DIR)/large_tasks.yaml --compute $(INPUT_DIR)/large_compute.yaml --analyze > large_task_compare_`git rev-parse --verify HEAD`.log

# utility targets

vg: $(TARGET)
	valgrind --track-origins=yes --log-file=./vg.log ./$(TARGET)

tags: $(HEADERS)
	ctags --sort=yes -f tags --language-force=C++ --c++-kinds=+p --fields=+iaS --extra=+q *.cc *.h

clean:
	rm -rf $(TARGET) $(OBJS) tags vg.log

../report.pdf: ../doc/report.md
	gimli -file ../doc/report.md -outputdir ..

# main targets

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(CXXFLAGS)

$(OBJS): $(HEADERS)
