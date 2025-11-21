include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(ca2co_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(ca2co_setup_options)
  option(ca2co_ENABLE_HARDENING "Enable hardening" ON)
  option(ca2co_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    ca2co_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    ca2co_ENABLE_HARDENING
    OFF)

  ca2co_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR ca2co_PACKAGING_MAINTAINER_MODE)
    option(ca2co_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(ca2co_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(ca2co_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ca2co_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ca2co_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ca2co_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(ca2co_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(ca2co_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ca2co_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(ca2co_ENABLE_IPO "Enable IPO/LTO" ON)
    option(ca2co_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(ca2co_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(ca2co_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(ca2co_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(ca2co_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(ca2co_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(ca2co_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(ca2co_ENABLE_PCH "Enable precompiled headers" OFF)
    option(ca2co_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      ca2co_ENABLE_IPO
      ca2co_WARNINGS_AS_ERRORS
      ca2co_ENABLE_USER_LINKER
      ca2co_ENABLE_SANITIZER_ADDRESS
      ca2co_ENABLE_SANITIZER_LEAK
      ca2co_ENABLE_SANITIZER_UNDEFINED
      ca2co_ENABLE_SANITIZER_THREAD
      ca2co_ENABLE_SANITIZER_MEMORY
      ca2co_ENABLE_UNITY_BUILD
      ca2co_ENABLE_CLANG_TIDY
      ca2co_ENABLE_CPPCHECK
      ca2co_ENABLE_COVERAGE
      ca2co_ENABLE_PCH
      ca2co_ENABLE_CACHE)
  endif()

  ca2co_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (ca2co_ENABLE_SANITIZER_ADDRESS OR ca2co_ENABLE_SANITIZER_THREAD OR ca2co_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(ca2co_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(ca2co_global_options)
  if(ca2co_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    ca2co_enable_ipo()
  endif()

  ca2co_supports_sanitizers()

  if(ca2co_ENABLE_HARDENING AND ca2co_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR ca2co_ENABLE_SANITIZER_UNDEFINED
       OR ca2co_ENABLE_SANITIZER_ADDRESS
       OR ca2co_ENABLE_SANITIZER_THREAD
       OR ca2co_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${ca2co_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${ca2co_ENABLE_SANITIZER_UNDEFINED}")
    ca2co_enable_hardening(ca2co_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(ca2co_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(ca2co_warnings INTERFACE)
  add_library(ca2co_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  ca2co_set_project_warnings(
    ca2co_warnings
    ${ca2co_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(ca2co_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    ca2co_configure_linker(ca2co_options)
  endif()

  include(cmake/Sanitizers.cmake)
  ca2co_enable_sanitizers(
    ca2co_options
    ${ca2co_ENABLE_SANITIZER_ADDRESS}
    ${ca2co_ENABLE_SANITIZER_LEAK}
    ${ca2co_ENABLE_SANITIZER_UNDEFINED}
    ${ca2co_ENABLE_SANITIZER_THREAD}
    ${ca2co_ENABLE_SANITIZER_MEMORY})

  set_target_properties(ca2co_options PROPERTIES UNITY_BUILD ${ca2co_ENABLE_UNITY_BUILD})

  if(ca2co_ENABLE_PCH)
    target_precompile_headers(
      ca2co_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(ca2co_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    ca2co_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(ca2co_ENABLE_CLANG_TIDY)
    ca2co_enable_clang_tidy(ca2co_options ${ca2co_WARNINGS_AS_ERRORS})
  endif()

  if(ca2co_ENABLE_CPPCHECK)
    ca2co_enable_cppcheck(${ca2co_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(ca2co_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    ca2co_enable_coverage(ca2co_options)
  endif()

  if(ca2co_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(ca2co_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(ca2co_ENABLE_HARDENING AND NOT ca2co_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR ca2co_ENABLE_SANITIZER_UNDEFINED
       OR ca2co_ENABLE_SANITIZER_ADDRESS
       OR ca2co_ENABLE_SANITIZER_THREAD
       OR ca2co_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    ca2co_enable_hardening(ca2co_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
