# Set up project
cmake_minimum_required(VERSION 2.6)
project(UnrandomEarthstar)
message(STATUS "==== Running cmake ====")

# Source files
set(SRC_FILES
	image.cpp
	main.cpp
	tile.cpp
	util/args_parser.cpp
	util/unittest.cpp
)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Libraries
set(PNG_FIND_VERSION "1.6.0")
find_package(ZLIB REQUIRED) # For PNG
find_package(PNG REQUIRED)

# Compiler specific configurations
if (MSVC)

else ()
	# -pg -g -O0
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wabi")
endif ()

# Include Directories
include_directories(
	${PROJECT_SOURCE_DIR}
	${PNG_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(
	${PROJECT_NAME}
	${PNG_LIBRARIES}
)
