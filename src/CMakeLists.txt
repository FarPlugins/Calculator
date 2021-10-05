project(Calculator VERSION ${PLUGIN_VERSION} LANGUAGES CXX C)

set(SRC_CPP
    plugcalc/api.cpp
    plugcalc/api.h
    plugcalc/api-far3.cpp
    plugcalc/calc.cpp
    plugcalc/calc.h
    plugcalc/calc.rc
    plugcalc/config.cpp
    plugcalc/config.h
    plugcalc/messages.h
    plugcalc/newparse.cpp
    plugcalc/newparse.h
    plugcalc/sarg.cpp
    plugcalc/sarg.h
    plugcalc/syntax.cpp
    plugcalc/syntax.h
    plugcalc/version.h
    shared/hashmap/HashMap.h
    shared/mathexpression/MathExpressionBase.h
    shared/sgml/sgml.cpp
    shared/trex/trex.c
    shared/trex/trex.h
    shared/ttmath/ttmath.h
    shared/ttmath/ttmathbig.h
    shared/ttmath/ttmathint.h
    shared/ttmath/ttmathobjects.h
    shared/ttmath/ttmathparser.h
    shared/ttmath/ttmathtypes.h
    shared/ttmath/ttmathuint.h
    shared/ttmath/ttmathuint_noasm.h
    shared/ttmath/ttmathuint_x86.h
    shared/ttmath/ttmathuint_x86_64.h
    )

set(SRC_DEF plugcalc/plugcalc.def)

#====================================================
# common flags
#====================================================
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # default for msvc project
  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  # for replace /Ob2  https://gitlab.kitware.com/cmake/cmake/-/issues/19084
  string(REGEX REPLACE "/Ob2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif ()

  set(FLAGS_CXX_DEFAULT /Zi)
  set(FLAGS_CXX_RELEASE /W3 /Oi /Ot /GL /GS- /Ob1 /GF /GR)
  set(FLAGS_CXX_DEBUG /W3 /GS)

  set(LINK_FLAGS "/MAP /MANIFEST:NO")

  # build with debug info in pdb
  if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(LINK_FLAGS "${LINK_FLAGS} /incremental:no /OPT:REF /OPT:ICF /debug /ltcg")
  endif ()

  add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DTTMATH_RELEASE)

  if (IS_x64_PLATFORM)
    set(CMAKE_RC_FLAGS -D_WIN64)
    add_definitions(-DTTMATH_NOASM -DTTMATH_PLATFORM64)
  else ()
    add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)
    # support old CPU AMD on WinXP
    set(FLAGS_CXX_DEFAULT ${FLAGS_CXX_DEFAULT} /arch:IA32)
    set(FLAGS_C_DEFAULT ${FLAGS_C_DEFAULT} /arch:IA32)
  endif ()

  # for vc_crt_fix_impl.cpp
  set(FLAGS_CXX_DEFAULT ${FLAGS_CXX_DEFAULT} /Zc:threadSafeInit-)
  set(FLAGS_C_DEFAULT ${FLAGS_C_DEFAULT} /Zc:threadSafeInit-)

endif ()

# set options for target
set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_C_DEBUG ${FLAGS_C_DEBUG} ${FLAGS_C_DEFAULT})
set(MAIN_FLAGS_C_RELEASE ${FLAGS_C_RELEASE} ${FLAGS_C_DEFAULT})

#====================================================
# support win2k, winxp
#====================================================
if (MSVC)
  set(ASM_SOURCE ${PROJECT_SOURCE_DIR}/shared/farplugin/vc_crt_fix.asm)
  set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/vc_crt_fix.obj)
  set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
  set(MASM_DEFINES "")
  if (${CMAKE_CL_64} STREQUAL "0")
    find_program(MASM_EXECUTABLE ml)
  else ()
    find_program(MASM_EXECUTABLE ml64)
    set(MASM_DEFINES "/DX64")
  endif ()

  add_custom_command(
      OUTPUT ${ASM_OBJECT}
      COMMAND ${MASM_EXECUTABLE}
      ARGS /c ${MASM_DEFINES} /Fo ${ASM_OBJECT} ${ASM_SOURCE}
      DEPENDS ${ASM_SOURCE}
  )

  set(SRC_CPP ${SRC_CPP}
      ${PROJECT_SOURCE_DIR}/shared/farplugin/vc_crt_fix_impl.cpp
      ${ASM_OBJECTS}
      )
endif ()
#====================================================
# build
#====================================================

set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_include_directories(${PROJECT_NAME}
    PRIVATE ./shared
    )

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    )

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(${PROJECT_NAME}
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_DEBUG}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>
      )
else ()
  target_compile_options(${PROJECT_NAME}
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_RELEASE}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>
      )
endif ()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    LINK_FLAGS "${LINK_FLAGS}"
    OUTPUT_NAME "calc"
    )

# create archive with pdb file
add_custom_target(
    ${PROJECT_NAME}_PDB ALL
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "../${PACKAGE_FILE_NAME}.pdb.zip" --format=zip $<TARGET_PDB_FILE:${PROJECT_NAME}>
    DEPENDS $<TARGET_PDB_FILE:${PROJECT_NAME}>
)