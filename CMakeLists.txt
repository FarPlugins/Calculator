cmake_minimum_required(VERSION 3.15)
project(Calculator)

#====================================================
# Enable policy
#====================================================
# enable CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

#====================================================
# Set default build to release
#====================================================
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif ()
message("Build type for ${PROJECT_NAME}: ${CMAKE_BUILD_TYPE}")

#====================================================
# Set configuration types
#====================================================
if(NOT MSVC_IDE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
  #target_compile_options cannot set parameters for all configurations in MSVC
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif()
message("Configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")

set(CALC_BUILD_ARCH x64 CACHE STRING "Build architecture: x86, x64, arm64")

# check platform for build
set(IS_x64_PLATFORM FALSE CACHE BOOL "" FORCE)
if ("$ENV{PLATFORM}" MATCHES "x64")
  set(IS_x64_PLATFORM TRUE CACHE BOOL "" FORCE)
endif ()

include(cmake/utils.cmake)
plugin_extract_version(${CMAKE_CURRENT_LIST_DIR}/src/plugcalc/version.h)
set(PACKAGE_FILE_NAME "${PROJECT_NAME}.${PLUGIN_VERSION}.${CALC_BUILD_ARCH}")

#====================================================
# build plugin
#====================================================
add_subdirectory(src)

#====================================================
# build resource files
#====================================================
# used @@ESCAPE and ESCAPE@@ for escape @ in files
configure_file(res/file_id.diz.in res/file_id.diz)
configure_file(res/calce.hlf.in res/calce.hlf)
configure_file(res/calcr.hlf.in res/calcr.hlf)

#====================================================
# install
#====================================================
install(TARGETS Calculator RUNTIME DESTINATION .)
install(DIRECTORY res/
    DESTINATION .
    PATTERN "*.in" EXCLUDE)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/src/calc.map
    ${CMAKE_CURRENT_BINARY_DIR}/res/file_id.diz
    ${CMAKE_CURRENT_BINARY_DIR}/res/calce.hlf
    ${CMAKE_CURRENT_BINARY_DIR}/res/calcr.hlf
    whatsnew_en.txt
    whatsnew_ru.txt
    DESTINATION .)

#====================================================
# cpack
#====================================================
set(CPACK_GENERATOR "ZIP" CACHE STRING "Generators to support. semi-colon delimited list")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}")

include(CPack)