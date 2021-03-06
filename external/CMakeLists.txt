################################################################################
### CmdLine library
################################################################################

add_library(
  ${IResearch_TARGET_NAME}-cmdline
  INTERFACE
)

target_include_directories(
  ${IResearch_TARGET_NAME}-cmdline
  SYSTEM
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cmdline
)

################################################################################
### MurMurHash library
################################################################################

set(MurMurHash_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/MurmurHash/MurmurHash3.cpp ${CMAKE_CURRENT_SOURCE_DIR}/MurmurHash/MurmurHash3.h
)

add_library(
  ${IResearch_TARGET_NAME}-murmurhash
  OBJECT
  ${MurMurHash_sources}
)

set_ipo(${IResearch_TARGET_NAME}-murmurhash)


target_compile_features(
  ${IResearch_TARGET_NAME}-murmurhash
  PRIVATE
  cxx_final
  cxx_variadic_templates
)


if (NOT MSVC)
  target_compile_options(
    ${IResearch_TARGET_NAME}-murmurhash
    PRIVATE "-Wno-all"
    PRIVATE "-fPIC"
  )
endif()

target_include_directories(
  ${IResearch_TARGET_NAME}-murmurhash
  SYSTEM
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/murmurhash
)

################################################################################
### OpenFST + Kaldi library
################################################################################

# We build OpenFST extensions provided by Kaldi as a part of OpenFST
set(Kaldi_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/kaldi/src/base/io-funcs.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/kaldi/src/base/kaldi-error.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/kaldi/src/base/kaldi-utils.cc
)

set(OpenFST_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/compat.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/flags.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/fst.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/fst-types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/weight.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/mapped-file.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/properties.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/symbol-table.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/symbol-table-ops.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/openfst/util.cc
  ${Kaldi_sources}
)

add_library(
  ${IResearch_TARGET_NAME}-ofst
  OBJECT
  ${OpenFST_sources}
)

set_ipo(${IResearch_TARGET_NAME}-ofst)


target_compile_definitions(
  ${IResearch_TARGET_NAME}-ofst
  PUBLIC "FST_NO_DYNAMIC_LINKING"
)

target_compile_features(
  ${IResearch_TARGET_NAME}-ofst
  PRIVATE
  cxx_final
  cxx_variadic_templates
)

# disable warnings for 3rd-party libs for a cleaner build
if (MSVC)
  target_compile_options(
    ${IResearch_TARGET_NAME}-ofst
    # MSVC2015-2017 require "/bigobj" to compile debug build
    PRIVATE "$<$<CONFIG:Debug>:/bigobj>"
    # MSVC requires listing each warning code individually
    # /W0 and /w cannot be used because the produce a single warning themselves
    PRIVATE "/wd4018"
    PRIVATE "/wd4100"
    PRIVATE "/wd4127"
    PRIVATE "/wd4244"
    PRIVATE "/wd4245"
    PRIVATE "/wd4267"
    PRIVATE "/wd4291"
    PRIVATE "/wd4305"
    PRIVATE "/wd4389"
    PRIVATE "/wd4510"
    PRIVATE "/wd4512"
    PRIVATE "/wd4610"
    PRIVATE "/wd4706"
  )
else()
  target_compile_options(
    ${IResearch_TARGET_NAME}-ofst
    PRIVATE "-Wno-all"
    PRIVATE "-fPIC"
  )
endif()

target_include_directories(
  ${IResearch_TARGET_NAME}-ofst
  SYSTEM
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/openfst
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/kaldi/src
)

################################################################################
### RapidJSON library
################################################################################

add_library(
  ${IResearch_TARGET_NAME}-rapidjson
  INTERFACE
)

target_include_directories(
  ${IResearch_TARGET_NAME}-rapidjson
  SYSTEM
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson
)

################################################################################
### SIMDCOMP library
################################################################################

if (USE_SIMDCOMP)
  add_subdirectory(simdcomp)
endif()

################################################################################
### utfcpp library
################################################################################

add_library(
  ${IResearch_TARGET_NAME}-utfcpp
  INTERFACE
)

target_include_directories(
  ${IResearch_TARGET_NAME}-utfcpp
  SYSTEM
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/utfcpp
)

################################################################################
### frozen library
################################################################################

set(FROZEN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/frozen/include PARENT_SCOPE)
