cmake_minimum_required(VERSION 3.22)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(version)
project(ipfs-chromium
  VERSION ${version}
  DESCRIPTION "Using ipfs_client in Chromium."
  HOMEPAGE_URL https://github.com/little-bear-labs/ipfs-chromium
  LANGUAGES CXX
  )
#The cache variables in all-caps are meant for you to customize the build.
#They are all here, so you don't need to hunt.
set(DOWNLOAD_CHROMIUM FALSE CACHE BOOL
    "Whether you prefer this cmake run to fetch its own private copy of Chromium stuff. Takes a very long time."
)
set(CHROMIUM_SOURCE_TREE "${CMAKE_CURRENT_BINARY_DIR}/chromium/src" CACHE PATH
    "Path to chromium/src. If DOWNLOAD_CHROMIUM=TRUE, and it does not exist, it will be created."
)
set(CHROMIUM_PROFILE "${CMAKE_BUILD_TYPE}" CACHE STRING "The profile of the current Chromium build.")
set(IPFS_CLIENT_VERSION 0.0.1.6 CACHE STRING "The version of ipfs_client to use in-tree.")
set(CXX_VERSION "20" CACHE STRING "The numeric part (year % 100) of the version of the C++ standard to be used. Must be at least 20 (for C++20). Must be one CMake knows about.")
if(DOWNLOAD_CHROMIUM)
    set(default_dt "${CMAKE_CURRENT_BINARY_DIR}/depot_tools")
else()
    set(default_dt "DETECT_FROM_PATH")
endif()
set(DEPOT_TOOLS_DIRECTORY "${default_dt}" CACHE PATH
    "Where to find depot_tools, e.g. DEPOT_TOOLS_DIRECTORY/gn.py should exist. Setting this to DETECT_FROM_PATH implies the directory is in your PATH and always will be."
)
set(TEST_BY_DEFAULT FALSE CACHE BOOL "Update unit tests as part of the 'all' default target.")
set(USE_CLANG_TIDY FALSE CACHE BOOL "Use clang-tidy, if found, to generate additional warnings.")
set(USE_DOXYGEN TRUE CACHE BOOL "If we should attempt to use Doxygen to generate documentation, if it's available.")
set(BRANDING_DIR "${CMAKE_CURRENT_LIST_DIR}/branding" CACHE PATH "Directory conataining branding files to synchronize. If empty, don't apply any branding changes.")
set(TOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tor" CACHE PATH
    "Path to the Tor source tree (maceip/Tor).  Used to build the tor binary for onion service support."
)
set(TOR_BINARY "" CACHE FILEPATH
    "Path to a pre-built tor binary.  If empty, the build will attempt to compile Tor from TOR_SOURCE_DIR."
)
#End of user-configuration cache variables.

if(CXX_VERSION GREATER_EQUAL 20)
    message(STATUS "Building for C++${CXX_VERSION}")
else()
    message(FATAL_ERROR "Unsupported CXX_VERSION: ${CXX_VERSION}")
endif()

set(IN_WORKSPACE 1)
include(chrome)
include(setup)

enable_testing()

if(IS_DIRECTORY "${CHROMIUM_SOURCE_TREE}")
    add_subdirectory(component)
elseif(CHROMIUM_SOURCE_TREE AND DOWNLOAD_CHROMIUM)
    message(STATUS "In download mode.")
    add_subdirectory(component)
else()
    message(WARNING "CHROMIUM_SOURCE_TREE (${CHROMIUM_SOURCE_TREE}) not pointing to source tree.")
endif()

add_subdirectory(electron-spin)

# --- Tor onion service support (maceip/Tor) ---
if(IS_DIRECTORY "${TOR_SOURCE_DIR}")
  # Build tor from source using autotools.
  include(ExternalProject)
  ExternalProject_Add(tor_build
    SOURCE_DIR "${TOR_SOURCE_DIR}"
    CONFIGURE_COMMAND "${TOR_SOURCE_DIR}/configure"
        "--prefix=${CMAKE_BINARY_DIR}/tor_install"
        "--disable-asciidoc"
        "--disable-manpage"
        "--disable-html-manual"
        "--disable-module-relay"
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE FALSE
    LOG_CONFIGURE TRUE
    LOG_BUILD TRUE
  )
  set(TOR_BINARY_RESOLVED "${CMAKE_BINARY_DIR}/tor_install/bin/tor")
  message(STATUS "Tor will be built from source: ${TOR_SOURCE_DIR}")
elseif(TOR_BINARY)
  set(TOR_BINARY_RESOLVED "${TOR_BINARY}")
  message(STATUS "Using pre-built Tor binary: ${TOR_BINARY}")
else()
  set(TOR_BINARY_RESOLVED "")
  message(STATUS "Tor onion service support: tor binary will be resolved from PATH at runtime.")
endif()

include(cmake/doxy.cmake)
include(cmake/marp.cmake)
if(doc_targets)
    add_custom_target(doc
      DEPENDS ${doc_targets}
      COMMENT "Generated documentation: ${doc_targets}"
    )
endif()
