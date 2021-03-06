project (QuaZip)
cmake_minimum_required(VERSION 2.6)

# diem: we don't want to see this check box from the nomacs cmake
# option(BUILD_WITH_QT4 "Build QuaZip with Qt4 no matter if Qt5 was found" OFF)

if( NOT BUILD_WITH_QT4 )
    # try Qt5 first, and prefer that if found
    find_package(Qt5Core QUIET)
endif()

if (Qt5Core_FOUND)
    set(QTCORE_LIBRARIES ${Qt5Core_LIBRARIES})
    # if there is no QT_ROOT, try to deduce it from Qt QtCore include
    if ("${QT_ROOT}" STREQUAL "")
        set(QT_ROOT ${QT_QTCORE_INCLUDE_DIR}/../..)
    endif()    
    include_directories(${Qt5Core_INCLUDE_DIRS})

    macro(qt_wrap_cpp)
        qt5_wrap_cpp(${ARGN})
    endmacro()
else()
	unset(Qt5Core_DIR CACHE)
    set(qt_min_version "4.5.0")
    find_package(Qt4 REQUIRED)
    set(QT_USE_QTGUI false)
    include(${QT_USE_FILE})
    include_directories(${QT_INCLUDES})
    set(QTCORE_LIBRARIES ${QT_QTCORE_LIBRARY})

    macro(qt_wrap_cpp)
        qt4_wrap_cpp(${ARGN})
    endmacro()
endif()

#added by stefan for nomacs
SET(CMAKE_DEBUG_POSTFIX "d")
unset(ZLIB_INCLUDE_DIRS CACHE)
unset(ZLIB_LIBRARIES CACHE)

# Use system zlib on unix and Qt ZLIB on Windows
IF(UNIX)
	find_package(ZLIB REQUIRED)
ELSE(UNIX)
	SET(ZLIB_INCLUDE_DIRS "${QT_ROOT}/src/3rdparty/zlib" CACHE STRING "Path to ZLIB headers of Qt")
	SET(ZLIB_LIBRARIES "")
	IF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
		MESSAGE("Please specify a valid zlib include dir")
	ENDIF(NOT EXISTS "${ZLIB_INCLUDE_DIRS}/zlib.h")
ENDIF(UNIX)

# All build libraries are moved to this directory
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(LIB_DESTINATION "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Library directory name" FORCE)

add_subdirectory(quazip)

install(FILES FindQuaZip.cmake DESTINATION ${CMAKE_ROOT}/Modules)

# added by stefan for nomacs

# write configuration file
get_property(QUAZIP_DEBUG_NAME TARGET quazip PROPERTY DEBUG_OUTPUT_NAME)
get_property(QUAZIP_RELEASE_NAME TARGET quazip PROPERTY RELEASE_OUTPUT_NAME)
SET(QUAZIP_LIBS optimized ${QUAZIP_RELEASE_NAME}.lib debug ${QUAZIP_DEBUG_NAME}.lib)
SET(QUAZIP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(QUAZIP_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
SET(QUAZIP_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
configure_file(${QUAZIP_SOURCE_DIR}/QuaZip.cmake.in ${QUAZIP_BUILD_DIRECTORY}/QuaZipConfig.cmake)

