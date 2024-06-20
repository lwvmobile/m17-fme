cmake_minimum_required(VERSION 3.10.2)
project(m17-fme)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

#Set curses to ncurses, and wide true for ascii
set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)

#use cmake option -DCOLORS=OFF to disable color output
option(COLORS
    "Build with Colors Enabled" ON)
if (COLORS)
    add_definitions(-DPRETTY_COLORS) 
endif ()

#use cmake option -DOTAKD=OFF to disable OTA Key Delivery
option(OTAKD
    "Build with Over the Air Encryption Key Delivery" ON)
if (OTAKD)
    add_definitions(-DOTA_KEY_DELIVERY) 
endif ()

include(git_revision)
git_describe(GIT_TAG)

find_package(LibSndFile REQUIRED) #required packaged need the REQUIRED argument
find_package(PulseAudio)
find_package(Curses)
find_package(CODEC2)

include_directories(SYSTEM ${LIBSNDFILE_INCLUDE_DIR})
set(LIBS ${LIBSNDFILE_LIBRARY})

# append optional libraries and directories and set definitions for optional function calls
if(PULSEAUDIO_FOUND)
    include_directories(SYSTEM ${PULSEAUDIO_INCLUDE_DIRS})
    list(APPEND LIBS ${PULSEAUDIO_SIMPLE_LIBRARY} ${PULSEAUDIO_LIBRARY})
    add_definitions(-DUSE_PULSEAUDIO)
endif(PULSEAUDIO_FOUND)

if(CODEC2_FOUND)
    include_directories(SYSTEM ${CODEC2_INCLUDE_DIRS})
    list(APPEND LIBS ${CODEC2_LIBRARIES})
    add_definitions(-DUSE_CODEC2)
endif(CODEC2_FOUND)

if(CURSES_FOUND)
    include_directories(SYSTEM ${CURSES_INCLUDE_DIRS})
    list(APPEND LIBS ${CURSES_LIBRARIES})
    add_definitions(-DUSE_CURSES)
endif(CURSES_FOUND)

FILE(GLOB SRCS src/*.c src/decoder/*.c src/encoder/*.c src/encryption/*.c src/io/*.c src/fec/*.c src/modem/*.c src/net/*.c src/ncurses/*.c src/utils/*.c src/*.cpp src/utils/*.cpp)
FILE(GLOB HEADERS include/*.h include/*.hpp)

configure_file("src/git_ver.c.in" "${CMAKE_CURRENT_BINARY_DIR}/git_ver.c" @ONLY)
list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/git_ver.c")

# Look and see if micro-ecc is available, if so, then the append to SRCS and HEADERS
find_file(UECC NAMES uECC.h PATHS src/micro-ecc/)
if ( EXISTS ${UECC} )
    list(APPEND SRCS "src/micro-ecc/uECC.c")
    list(APPEND HEADERS "src/micro-ecc/uECC.h") #needed?
    add_definitions(-DUSE_UECC)
    message ("-- micro-ecc found")
else()
    message ("-- micro-ecc not found")
endif()

include_directories("${PROJECT_SOURCE_DIR}/include")

ADD_EXECUTABLE(m17-fme ${SRCS} ${HEADERS})
TARGET_LINK_LIBRARIES(m17-fme ${LIBS})

target_compile_options(m17-fme PRIVATE -Wunused-but-set-variable -Wall -Wextra -Wpedantic $<$<COMPILE_LANGUAGE:C>:-Wpointer-sign>)

include(GNUInstallDirs)
install(TARGETS m17-fme DESTINATION ${CMAKE_INSTALL_BINDIR})

# uninstall target
configure_file(
    "cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)