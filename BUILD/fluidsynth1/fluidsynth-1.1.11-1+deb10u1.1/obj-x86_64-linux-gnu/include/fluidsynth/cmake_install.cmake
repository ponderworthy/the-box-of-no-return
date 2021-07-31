# Install script for directory: /home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "None")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fluidsynth" TYPE FILE FILES
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/audio.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/event.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/gen.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/log.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/midi.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/misc.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/mod.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/ramsfont.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/seq.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/seqbind.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/settings.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/sfont.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/shell.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/synth.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/types.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/include/fluidsynth/voice.h"
    "/home/jeb/BNR/BUILD/fluidsynth1/fluidsynth-1.1.11-1+deb10u1.1/obj-x86_64-linux-gnu/include/fluidsynth/version.h"
    )
endif()

