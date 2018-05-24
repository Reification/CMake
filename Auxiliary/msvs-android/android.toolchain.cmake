#toolchain file supporting generation of MSVS projects configured for the Mobile C++ Development for Android workflow
#specify this file directly as toolchain file or include from your project's toolchain file

#for android.toolchain.cmake innards
if(NOT "$ENV{ANDROID_NDK_HOME}" STREQUAL "")
  set(_ANDROID_NDK "$ENV{ANDROID_NDK_HOME}")
elseif(NOT "$ENV{NDK_ROOT}" STREQUAL "")
  set(_ANDROID_NDK "$ENV{NDK_ROOT}")
else()
  message(FATAL_ERROR "environment variable ANDROID_NDK_HOME or NDK_ROOT must be set.")
endif()

if(NOT EXISTS "${_ANDROID_NDK}/")
  message(FATAL_ERROR "'${_ANDROID_NDK}' does not exist")
endif()

if("${ANDROID_ABI}" STREQUAL "")
  set(ANDROID_ABI arm64-v8a)
endif()

if("${ANDROID_NATIVE_API_LEVEL}" STREQUAL "")
  set(ANDROID_NATIVE_API_LEVEL 26)
endif()

if("${ANDROID_STL}" STREQUAL "")
  set(ANDROID_STL "c++_static")
endif()

if("${ANDROID_TOOLCHAIN}" STREQUAL "")
  set(ANDROID_TOOLCHAIN clang)
endif()

#NDK toolchain file does the heavy lifting
include("${_ANDROID_NDK}/build/cmake/android.toolchain.cmake")

unset(_ANDROID_NDK)

#default is executable - can't build native console executable under NDK, so use STATIC_LIBRARY (only other valid option)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#set up cmake variables for visual studio vcxproj generation
if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
  #CRITICAL! if not set we don't get an MSVS Android workflow vcxproj
  set(VS_GLOBAL_KEYWORD "Android")

  #utility value for testing if building for android under visual studio mobile workflow
  set(VS_ANDROID true) 

  #cmake will pick most recent supported clang toolchain by default.
  #for gcc, specify the last version that will ever be supported on android.
  #seriously, who's using this any more?
  if(${ANDROID_TOOLCHAIN} STREQUAL "gcc")
    if(NOT CMAKE_GENERATOR_TOOLSET)
      set(CMAKE_GENERATOR_TOOLSET "GCC_4_9")
    endif()
  endif()

  #Platform specification for MSVS is with these strings
  if("${ANDROID_ABI}" MATCHES "arm64")
    set(CMAKE_GENERATOR_PLATFORM "ARM64")
  elseif("${ANDROID_ABI}" MATCHES "arm")
    set(CMAKE_GENERATOR_PLATFORM "ARM")
  elseif("${ANDROID_ABI}" MATCHES "x86_64")
    set(CMAKE_GENERATOR_PLATFORM "x86_64")
  elseif("${ANDROID_ABI}" MATCHES "x86")
    set(CMAKE_GENERATOR_PLATFORM "x86")
  else()
    message(FATAL "Only arm64, arm, x86, x86_64 platforms supported.")
  endif()
endif()
