# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_SOURCE_DIR = /home/mica/Desktop/Vulkan/compute

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mica/Desktop/Vulkan/compute/build

# Utility rule file for shaders.

# Include the progress variables for this target.
include src/CMakeFiles/shaders.dir/progress.make

src/CMakeFiles/shaders: ../data/spirv/array_sum.comp.spv


../data/spirv/array_sum.comp.spv: ../data/glsl/array_sum.comp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/mica/Desktop/Vulkan/compute/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating ../../data/spirv/array_sum.comp.spv"
	cd /home/mica/Desktop/Vulkan/compute/build/src && /home/mica/Documents/vulkan_downloads/1.3.231.2/x86_64/bin/glslc /home/mica/Desktop/Vulkan/compute/data/glsl/array_sum.comp -o /home/mica/Desktop/Vulkan/compute/data/spirv/array_sum.comp.spv

shaders: src/CMakeFiles/shaders
shaders: ../data/spirv/array_sum.comp.spv
shaders: src/CMakeFiles/shaders.dir/build.make

.PHONY : shaders

# Rule to build all files generated by this target.
src/CMakeFiles/shaders.dir/build: shaders

.PHONY : src/CMakeFiles/shaders.dir/build

src/CMakeFiles/shaders.dir/clean:
	cd /home/mica/Desktop/Vulkan/compute/build/src && $(CMAKE_COMMAND) -P CMakeFiles/shaders.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/shaders.dir/clean

src/CMakeFiles/shaders.dir/depend:
	cd /home/mica/Desktop/Vulkan/compute/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mica/Desktop/Vulkan/compute /home/mica/Desktop/Vulkan/compute/src /home/mica/Desktop/Vulkan/compute/build /home/mica/Desktop/Vulkan/compute/build/src /home/mica/Desktop/Vulkan/compute/build/src/CMakeFiles/shaders.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/shaders.dir/depend

