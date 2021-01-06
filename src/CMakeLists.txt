project(Calculator CXX C)

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
  set(FLAGS_CXX_DEBUG /W4 /GS)


  set(LINK_FLAGS "/MAP /MANIFEST:NO")

  # build with debug info in pdb
  if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(LINK_FLAGS "${LINK_FLAGS} /incremental:no /OPT:REF /OPT:ICF /debug /ltcg")
  endif ()

  add_definitions(-DUNICODE -D_UNICODE)

  if (IS_x64_PLATFORM)
    set(CMAKE_RC_FLAGS -D_WIN64)
    add_definitions(-DTTMATH_NOASM -DTTMATH_PLATFORM64)
  else ()
    add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)
  endif ()

  add_definitions(-D_CRT_SECURE_NO_WARNINGS -DTTMATH_RELEASE)

endif ()

# set options for target
set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_C_DEBUG ${FLAGS_C_DEBUG} ${FLAGS_C_DEFAULT})
set(MAIN_FLAGS_C_RELEASE ${FLAGS_C_RELEASE} ${FLAGS_C_DEFAULT})

#====================================================
# build
#====================================================

set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(Calculator SHARED ${SRC_FILES})
target_include_directories(Calculator
    PRIVATE ./shared
    )

set_target_properties(Calculator PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    )

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(Calculator
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_DEBUG}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>
      )
else ()
  target_compile_options(Calculator
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_RELEASE}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>
      )
endif ()

set_target_properties(Calculator
    PROPERTIES
    LINK_FLAGS "${LINK_FLAGS}"
    OUTPUT_NAME "calc"
    )

#====================================================
# install
#====================================================
install(TARGETS Calculator RUNTIME DESTINATION .)
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/calc.map DESTINATION .)

