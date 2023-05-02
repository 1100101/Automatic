# Minimal version of CMake
cmake_minimum_required (VERSION 3.0)

# Build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif ()

# Define project name
project (Automatic
   LANGUAGES C
   VERSION 0.9)

include(CTest)
include(GNUInstallDirs)
if(EXISTS ${CMAKE_SOURCE_DIR}/.git)
    find_package(Git)
endif()

if(GIT_FOUND)
  execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --max-count=1 --abbrev-commit HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  configure_file(include/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)
endif()

# include custom Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(automatic_srcs
        src/automatic.c
        src/base64.c
        src/config_parser.c
        src/downloads.c
        src/feed_item.c
        src/file.c
        src/filters.c
        src/json.c
        src/list.c
        src/memwatch.c
        src/output.c
        src/prowl.c
        src/pushover.c
        src/regex.c
        src/rss_feed.c
        src/state.c
        src/torrent.c
        src/transmission.c
        src/urlcode.c
        src/utils.c
        src/web.c
        src/xml_parser.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

option(AUTOMATIC_BUILD_TESTS "Build the test programs" OFF)

find_package(CURL 7.15.0 REQUIRED)
find_package(LibXml2 2.6.31 REQUIRED)
find_package(PCRE 7.4 REQUIRED)

add_executable(automatic ${automatic_srcs})
target_include_directories(automatic PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(automatic PRIVATE CURL::libcurl LibXml2::LibXml2 PCRE::PCRE)

# installation
include(systemdservice)

install(TARGETS automatic DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES src/automatic.conf-sample DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})

# Install service if we have systemd
if (SYSTEMD_FOUND)
  if(NOT EXISTS "${SYSTEMD_SERVICES_INSTALL_DIR}/automatic.service")
  	configure_file(src/automatic.service.in ${CMAKE_CURRENT_BINARY_DIR}/automatic.service @ONLY)
	  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/automatic.service
		        DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR}
		        COMPONENT data
	         )
  else()
    message(STATUS "Note: Not installing automatic.service, file already exists")
  endif()
endif (SYSTEMD_FOUND)

if(AUTOMATIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(src/tests)
endif()
