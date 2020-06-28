# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tqnwhz/compiler

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tqnwhz/compiler

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tqnwhz/compiler/CMakeFiles /home/tqnwhz/compiler/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/tqnwhz/compiler/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named compiler

# Build rule for target.
compiler: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 compiler
.PHONY : compiler

# fast build rule for target.
compiler/fast:
	$(MAKE) -f CMakeFiles/compiler.dir/build.make CMakeFiles/compiler.dir/build
.PHONY : compiler/fast

#=============================================================================
# Target rules for targets named backend

# Build rule for target.
backend: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 backend
.PHONY : backend

# fast build rule for target.
backend/fast:
	$(MAKE) -f CMakeFiles/backend.dir/build.make CMakeFiles/backend.dir/build
.PHONY : backend/fast

#=============================================================================
# Target rules for targets named arm

# Build rule for target.
arm: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 arm
.PHONY : arm

# fast build rule for target.
arm/fast:
	$(MAKE) -f CMakeFiles/arm.dir/build.make CMakeFiles/arm.dir/build
.PHONY : arm/fast

#=============================================================================
# Target rules for targets named mir

# Build rule for target.
mir: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 mir
.PHONY : mir

# fast build rule for target.
mir/fast:
	$(MAKE) -f CMakeFiles/mir.dir/build.make CMakeFiles/mir.dir/build
.PHONY : mir/fast

arm_code/arm.o: arm_code/arm.cpp.o

.PHONY : arm_code/arm.o

# target to build an object file
arm_code/arm.cpp.o:
	$(MAKE) -f CMakeFiles/arm.dir/build.make CMakeFiles/arm.dir/arm_code/arm.cpp.o
.PHONY : arm_code/arm.cpp.o

arm_code/arm.i: arm_code/arm.cpp.i

.PHONY : arm_code/arm.i

# target to preprocess a source file
arm_code/arm.cpp.i:
	$(MAKE) -f CMakeFiles/arm.dir/build.make CMakeFiles/arm.dir/arm_code/arm.cpp.i
.PHONY : arm_code/arm.cpp.i

arm_code/arm.s: arm_code/arm.cpp.s

.PHONY : arm_code/arm.s

# target to generate assembly for a file
arm_code/arm.cpp.s:
	$(MAKE) -f CMakeFiles/arm.dir/build.make CMakeFiles/arm.dir/arm_code/arm.cpp.s
.PHONY : arm_code/arm.cpp.s

backend/codegen/codegen.o: backend/codegen/codegen.cpp.o

.PHONY : backend/codegen/codegen.o

# target to build an object file
backend/codegen/codegen.cpp.o:
	$(MAKE) -f CMakeFiles/backend.dir/build.make CMakeFiles/backend.dir/backend/codegen/codegen.cpp.o
.PHONY : backend/codegen/codegen.cpp.o

backend/codegen/codegen.i: backend/codegen/codegen.cpp.i

.PHONY : backend/codegen/codegen.i

# target to preprocess a source file
backend/codegen/codegen.cpp.i:
	$(MAKE) -f CMakeFiles/backend.dir/build.make CMakeFiles/backend.dir/backend/codegen/codegen.cpp.i
.PHONY : backend/codegen/codegen.cpp.i

backend/codegen/codegen.s: backend/codegen/codegen.cpp.s

.PHONY : backend/codegen/codegen.s

# target to generate assembly for a file
backend/codegen/codegen.cpp.s:
	$(MAKE) -f CMakeFiles/backend.dir/build.make CMakeFiles/backend.dir/backend/codegen/codegen.cpp.s
.PHONY : backend/codegen/codegen.cpp.s

main.o: main.cpp.o

.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/compiler.dir/build.make CMakeFiles/compiler.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i

.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/compiler.dir/build.make CMakeFiles/compiler.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s

.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/compiler.dir/build.make CMakeFiles/compiler.dir/main.cpp.s
.PHONY : main.cpp.s

mir/mir.o: mir/mir.cpp.o

.PHONY : mir/mir.o

# target to build an object file
mir/mir.cpp.o:
	$(MAKE) -f CMakeFiles/mir.dir/build.make CMakeFiles/mir.dir/mir/mir.cpp.o
.PHONY : mir/mir.cpp.o

mir/mir.i: mir/mir.cpp.i

.PHONY : mir/mir.i

# target to preprocess a source file
mir/mir.cpp.i:
	$(MAKE) -f CMakeFiles/mir.dir/build.make CMakeFiles/mir.dir/mir/mir.cpp.i
.PHONY : mir/mir.cpp.i

mir/mir.s: mir/mir.cpp.s

.PHONY : mir/mir.s

# target to generate assembly for a file
mir/mir.cpp.s:
	$(MAKE) -f CMakeFiles/mir.dir/build.make CMakeFiles/mir.dir/mir/mir.cpp.s
.PHONY : mir/mir.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... compiler"
	@echo "... backend"
	@echo "... arm"
	@echo "... mir"
	@echo "... arm_code/arm.o"
	@echo "... arm_code/arm.i"
	@echo "... arm_code/arm.s"
	@echo "... backend/codegen/codegen.o"
	@echo "... backend/codegen/codegen.i"
	@echo "... backend/codegen/codegen.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... mir/mir.o"
	@echo "... mir/mir.i"
	@echo "... mir/mir.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

