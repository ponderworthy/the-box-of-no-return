# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_SOURCE_DIR = /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build

# Include any dependencies generated for this target.
include CMakeFiles/sfizz_pugixml.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/sfizz_pugixml.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/sfizz_pugixml.dir/flags.make

CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o: CMakeFiles/sfizz_pugixml.dir/flags.make
CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o: ../src/external/pugixml/src/pugixml.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o -c /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/src/external/pugixml/src/pugixml.cpp

CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/src/external/pugixml/src/pugixml.cpp > CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.i

CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/src/external/pugixml/src/pugixml.cpp -o CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.s

# Object files for target sfizz_pugixml
sfizz_pugixml_OBJECTS = \
"CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o"

# External object files for target sfizz_pugixml
sfizz_pugixml_EXTERNAL_OBJECTS =

libsfizz_pugixml.a: CMakeFiles/sfizz_pugixml.dir/src/external/pugixml/src/pugixml.cpp.o
libsfizz_pugixml.a: CMakeFiles/sfizz_pugixml.dir/build.make
libsfizz_pugixml.a: CMakeFiles/sfizz_pugixml.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libsfizz_pugixml.a"
	$(CMAKE_COMMAND) -P CMakeFiles/sfizz_pugixml.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sfizz_pugixml.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/sfizz_pugixml.dir/build: libsfizz_pugixml.a

.PHONY : CMakeFiles/sfizz_pugixml.dir/build

CMakeFiles/sfizz_pugixml.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sfizz_pugixml.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sfizz_pugixml.dir/clean

CMakeFiles/sfizz_pugixml.dir/depend:
	cd /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1 /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1 /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build /home/jeb/BNR/BUILD/sfizz/sfizz-1.0.0-0+nmu1/build/CMakeFiles/sfizz_pugixml.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sfizz_pugixml.dir/depend
