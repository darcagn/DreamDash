cmake_minimum_required(VERSION 3.0)
#set(CMAKE_VERBOSE_MAKEFILE ON)

include(toolchain.cmake)

#############
# ds loader
#############
project(dreamboot)
enable_language(ASM)

set(VERSION "1.1.0")

#############
# code
#############

file(GLOB SOURCES src/*.c)
set(INCLUDES src src/uthash)
set(CFLAGS -D__DB_VERSION__="${VERSION}")

file(GLOB_RECURSE CROSS_SOURCES src/dc/*.c)
file(GLOB_RECURSE DS_SOURCES src/ds/*.*)
list(APPEND SOURCES ${CROSS_SOURCES} ${DS_SOURCES})
list(APPEND INCLUDES src/ds/include src/ds/include/fatfs)
list(APPEND CFLAGS -D__DC__)
set(LDFLAGS kosext2fs ${KOS_LIBS})

#############
# executable
#############
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDES})
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})
target_link_libraries(${PROJECT_NAME} ${LDFLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE -s)

include(targets.cmake)
