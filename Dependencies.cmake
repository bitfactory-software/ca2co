include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(cogoproject_setup_dependencies)

  # For each dependency, see if it's
  # already been provided to us by a parent project

  if(NOT cogoproject_is_top)
    message("co_go -> no dependecies required")
    return()
  endif()

  message("cpm add packages...") 

#  if(NOT TARGET fmtlib::fmtlib)
#    cpmaddpackage("gh:fmtlib/fmt#11.1.4")
#  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage("gh:catchorg/Catch2@3.8.1")
  endif()

  if(NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage(
      NAME Boost
	  VERSION 1.85.0
	  URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.7z
	  OPTIONS "BOOST_INCLUDE_LIBRARIES asio"
      )
  endif()

  set(BOOST_INCLUDE_LIBRARIES asio)
  set(BOOST_ENABLE_CMAKE ON)
  include(FetchContent)
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_PROGRESS TRUE
    GIT_TAG boost-1.82.0
 )
 FetchContent_MakeAvailable(Boost)

  if(NOT TARGET tools::tools)
    cpmaddpackage("gh:lefticus/tools#update_build_system")
  endif()

endfunction()
