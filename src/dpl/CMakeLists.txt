# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2018-2025, The OpenROAD Authors
set(OPENSTA_INCS ${PROJECT_SOURCE_DIR}/sta/include)
set(DBSTA_INCS ${PROJECT_SOURCE_DIR}/dbSta/include)
get_target_property(OPENSTA_INCS OpenSTA INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(DBSTA_INCS dbSta INTERFACE_INCLUDE_DIRECTORIES)

include("openroad")
find_package(TCL)
find_package(Boost CONFIG)
find_package(LEMON NAMES LEMON lemon REQUIRED)
swig_lib(NAME         dpl
         NAMESPACE    dpl
         I_FILE       src/Opendp.i
         SCRIPTS      src/Opendp.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

add_library(dpl_lib
  src/Opendp.cpp
  src/dbToOpendp.cpp
  src/infrastructure/Grid.cpp
  src/CheckPlacement.cpp
  src/infrastructure/Objects.cpp
  src/infrastructure/Padding.cpp
  src/infrastructure/Tile.cpp
  src/Place.cpp
  src/FillerPlacement.cpp
  src/DecapPlacement.cpp
  src/OptMirror.cpp
  src/PlacementDRC.cpp

  src/Optdp.cpp
  src/infrastructure/architecture.cxx
  src/util/color.cxx
  src/infrastructure/network.cxx
  src/util/utility.cxx
  src/legalize_shift.cxx
  src/optimization/detailed.cxx
  src/optimization/detailed_manager.cxx
  src/optimization/detailed_mis.cxx
  src/optimization/detailed_reorder.cxx
  src/optimization/detailed_global.cxx
  src/optimization/detailed_vertical.cxx
  src/objective/detailed_displacement.cxx
  src/objective/detailed_hpwl.cxx
  src/objective/detailed_abu.cxx
  src/optimization/detailed_random.cxx
  src/optimization/detailed_orient.cxx
  src/util/journal.cxx

  src/mbff.cpp
  src/gridMap.cpp
)

target_link_libraries(dpl_lib
  PUBLIC
    odb
  PRIVATE
    utl_lib
    OpenSTA
)

target_include_directories(dpl_lib
  PUBLIC
    include
    ${LEMON_INCLUDE_DIRS}
    ${OPENSTA_INCS}
    ${DBSTA_INCS}
  PRIVATE
    src
)

target_sources(dpl
  PRIVATE
    src/MakeOpendp.cpp
    src/graphics/Graphics.cpp
    src/graphics/DplObserver.h

)

target_include_directories(dpl
  PUBLIC
    include
    ${OPENSTA_INCS}
    ${DBSTA_INCS}
  PRIVATE
    src
)

target_link_libraries(dpl
  PRIVATE
    ${TCL_LIBRARY}
    dpl_lib
    gui
    OpenSTA
)

messages(
  TARGET dpl
)

################################################################

add_custom_target(opendp_tags etags -o TAGS
  ${OPENDP_SRC}
  ${OPENDP_HOME}/include/dpl/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dpl_py
           NAMESPACE     dpl
           LANGUAGE      python
           I_FILE        src/Opendp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dpl_py.py
  )

  target_link_libraries(dpl_py
    PUBLIC
      ${Boost_LIBRARIES}
      dpl_lib
      gui
      odb
      OpenSTA
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
