if(NOT ENABLE_PLUGINS)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if(COMPILER_SUPPORTS_CXX11)
		message(STATUS "c++11 is supported")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
		message(STATUS "c++0x is supported")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
					message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
  #create the targets
  set(BINARY_NAME ${CMAKE_PROJECT_NAME})
  link_directories(${LIBRAW_LIBRARY_DIRS} ${OpenCV_LIBRARY_DIRS} ${EXIV2_LIBRARY_DIRS})
  add_executable(${BINARY_NAME} WIN32 MACOSX_BUNDLE ${NOMACS_SOURCES} ${NOMACS_UI} ${NOMACS_MOC_SRC} ${NOMACS_RCC} ${NOMACS_HEADERS} ${NOMACS_RC} ${NOMACS_QM} ${NOMACS_TRANSLATIONS} ${LIBQPSD_SOURCES} ${LIBQPSD_HEADERS} ${LIBQPSD_MOC_SRC} ${WEBP_SOURCE} ${QUAZIP_SOURCES} ${QUAZIP_MOC_SRC})
  target_link_libraries(${BINARY_NAME} ${QT_LIBRARIES} ${EXIV2_LIBRARIES} ${LIBRAW_LIBRARIES} ${OpenCV_LIBRARIES} ${VERSION_LIB} ${TIFF_LIBRARY} ${ZLIB_LIBRARY} ${WEBP_LIBRARIES} ${QUAZIP_LIBRARIES} ${WEBP_STATIC_LIBRARIES})

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	  SET_TARGET_PROPERTIES(${BINARY_NAME} PROPERTIES LINK_FLAGS -fopenmp)
  endif()

  # installation
  #  binary
  install(TARGETS ${BINARY_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX})
  #  desktop file
  install(FILES nomacs.desktop DESTINATION share/applications)
  #  icon
  install(FILES src/img/nomacs.png DESTINATION share/pixmaps)
  #  translations
  install(FILES ${NOMACS_QM} DESTINATION share/nomacs/translations)
  #  manpage
  if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	 install(FILES Readme/nomacs.1 DESTINATION man/man1)
  else()
	 install(FILES Readme/nomacs.1 DESTINATION share/man/man1)
  endif()


  # "make dist" target
  string(TOLOWER ${CMAKE_PROJECT_NAME} CPACK_PACKAGE_NAME)
  set(CPACK_PACKAGE_VERSION "${NOMACS_VERSION}")
  set(CPACK_SOURCE_GENERATOR "TBZ2")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
  set(CPACK_IGNORE_FILES "/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;refresh-copyright-and-license.pl;build;release;")
  set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})
  include(CPack)
  # simulate autotools' "make dist"
  add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


  # generate configuration file
  set(NOMACS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(NOMACS_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
  configure_file(${NOMACS_SOURCE_DIR}/nomacs.cmake.in ${CMAKE_BINARY_DIR}/nomacsConfig.cmake)
  
  
else()
  message(WARNING "ENABLE_PLUGINS is highly experimentally for Unix/Linux")
  add_definitions(-DWITH_PLUGINS)

	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	if(COMPILER_SUPPORTS_CXX11)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	else()
					message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")

  
  set(BINARY_NAME ${CMAKE_PROJECT_NAME})
  set(DLL_NAME lib${CMAKE_PROJECT_NAME})
  #set(LIB_NAME optimized ${DLL_NAME}.lib debug ${DLL_NAME}d.lib)
  LIST(REMOVE_ITEM NOMACS_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
  link_directories(${LIBRAW_LIBRARY_DIRS} ${OpenCV_LIBRARY_DIRS} ${EXIV2_LIBRARY_DIRS} ${CMAKE_BINARY_DIR})
  add_executable(${BINARY_NAME} WIN32  MACOSX_BUNDLE src/main.cpp ${NOMACS_MOC_SRC_SU} ${NOMACS_QM} ${NOMACS_TRANSLATIONS} ${NOMACS_RC})
  target_link_libraries(${BINARY_NAME} ${QT_LIBRARIES} ${VERSION_LIB} ${DLL_NAME})

  set_target_properties(${BINARY_NAME} PROPERTIES COMPILE_FLAGS "-DDK_DLL_IMPORT -DNOMINMAX")
  set_target_properties(${BINARY_NAME} PROPERTIES IMPORTED_IMPLIB "")
		  
  add_library(${DLL_NAME} SHARED ${NOMACS_SOURCES} ${NOMACS_UI} ${NOMACS_MOC_SRC} ${NOMACS_RCC} ${NOMACS_HEADERS} ${NOMACS_RC} ${LIBQPSD_SOURCES} ${LIBQPSD_HEADERS} ${LIBQPSD_MOC_SRC} ${WEBP_SOURCE}  ${QUAZIP_SOURCES} ${QUAZIP_MOC_SRC})
  target_link_libraries(${DLL_NAME} ${QT_LIBRARIES} ${EXIV2_LIBRARIES} ${LIBRAW_LIBRARIES} ${OpenCV_LIBRARIES} ${VERSION_LIB} ${TIFF_LIBRARIES} ${HUPNP_LIBS} ${HUPNPAV_LIBS} ${WEBP_LIBRARIES} ${WEBP_STATIC_LIBRARIES})
  add_dependencies(${BINARY_NAME} ${DLL_NAME})

  if (ENABLE_QT5)
	  qt5_use_modules(${BINARY_NAME} Widgets Gui Network LinguistTools PrintSupport)
	  qt5_use_modules(${DLL_NAME} Widgets Gui Network LinguistTools PrintSupport)
  ENDIF()


  set_target_properties(${DLL_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/libs)
  set_target_properties(${DLL_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)
  set_target_properties(${DLL_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_REALLYRELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)
				  
  set_target_properties(${DLL_NAME} PROPERTIES COMPILE_FLAGS "-DDK_DLL_EXPORT -DNOMINMAX")
  set_target_properties(${DLL_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${DLL_NAME}d)
  set_target_properties(${DLL_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${DLL_NAME})

    # installation
  #  binary
  install(TARGETS ${BINARY_NAME} ${DLL_NAME} DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX})
  #  desktop file
  install(FILES nomacs.desktop DESTINATION share/applications)
  #  icon
  install(FILES src/img/nomacs.png DESTINATION share/pixmaps)
  #  translations
  install(FILES ${NOMACS_QM} DESTINATION share/nomacs/translations)
  #  manpage
  install(FILES Readme/nomacs.1 DESTINATION share/man/man1)
  #  appdata
  install(FILES nomacs.appdata.xml DESTINATION /usr/share/appdata/)


  # "make dist" target
  string(TOLOWER ${CMAKE_PROJECT_NAME} CPACK_PACKAGE_NAME)
  set(CPACK_PACKAGE_VERSION "${NOMACS_VERSION}")
  set(CPACK_SOURCE_GENERATOR "TBZ2")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
  set(CPACK_IGNORE_FILES "/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;refresh-copyright-and-license.pl;build;release;")
  set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})
  include(CPack)
  # simulate autotools' "make dist"
  add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


  # generate configuration file
  set(NOMACS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(NOMACS_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
  set(NOMACS_LIBS ${DLL_NAME})
  
  configure_file(${NOMACS_SOURCE_DIR}/nomacs.cmake.in ${CMAKE_BINARY_DIR}/nomacsConfig.cmake)
  
endif(NOT ENABLE_PLUGINS)
