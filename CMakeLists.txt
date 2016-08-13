cmake_minimum_required (VERSION 2.8.11)

set (PROJECTNAME "MEMSGauge")

string (TOLOWER ${PROJECTNAME} PNAME)
project (${PNAME})
add_definitions (-DPROJECTNAME=\"${PROJECTNAME}\")

set (VER_MAJOR 0)
set (VER_MINOR 2)
set (VER_PATCH 1)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_AUTOMOC ON)

set (CPACK_SOURCE_GENERATOR "TGZ")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-src")
set (CPACK_SOURCE_IGNORE_FILES
      ".*~$"
      ".git"
      ".gitignore"
      ".*swp$"
      ".*zip$"
      ".*gz$"
      ".*deb$"
      "Makefile.*"
      "moc_"
      "qrc_"
      "^CPack.*"
      "_CPack_Packages"
      "CPackConfig.cmake"
      "CPackSourceConfig.cmake"
      "CMakeFiles"
      "CMakeCache.txt"
      "CMakeLists.txt.user*"
      "cmake_install.cmake"
      "install_manifest.txt"
      "README.TXT"
      "LICENSE.TXT"
      "build/"
      "build-win32/"
      "settings.ini"
      "debug"
      "release"
      "object_script.*")

configure_file (
  "help.html.in"
  "${CMAKE_SOURCE_DIR}/help.html"
  )

find_package (Qt5Widgets)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

include_directories ("${CMAKE_SOURCE_DIR}"
                     "${CMAKE_SOURCE_DIR}/qledindicator"
                     "${CMAKE_SOURCE_DIR}/analogwidgets"
                     "${CMAKE_CURRENT_BINARY_DIR}"
                     "${Qt5Widgets_INCLUDE_DIRS}")

qt5_wrap_ui (UI_SOURCE mainwindow.ui)
qt5_add_resources (RG_RESOURCE resources.qrc)

add_definitions ("-DVER_MAJOR=${VER_MAJOR}"
                 "-DVER_MINOR=${VER_MINOR}"
                 "-DVER_PATCH=${VER_PATCH}")

add_executable (${PNAME} main.cpp
                         memsinterface.cpp
                         helpviewer.cpp
                         logger.cpp
                         serialdevenumerator.cpp
                         mainwindow.cpp
                         aboutbox.cpp
                         optionsdialog.cpp
                         qledindicator/qledindicator.cpp
                         analogwidgets/led.cpp
                         analogwidgets/functions.cpp
                         analogwidgets/widgetwithbackground.cpp
                         analogwidgets/manometer.cpp
                         analogwidgets/abstractmeter.cpp
                         ${UI_SOURCE}
                         ${RG_RESOURCE})

if (MINGW)
  message (STATUS "Found Windows/MinGW platform.")

  # prevent the final executable from launching a terminal window in parallel with the main GUI
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")

  # -fPIC appears to be redundant when building win32 binaries, so disable that flag
  string (REGEX REPLACE "-fPIC" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

  set (LIBROSCO_PATH "$ENV{SystemDrive}/mems-rosco")
  include_directories ("${LIBROSCO_PATH}/include")

  message (STATUS "Defaulting installation path to: $ENV{SystemDrive}/${PNAME}")
  set (CMAKE_INSTALL_PREFIX "$ENV{SystemDrive}/${PNAME}")
  set (EXE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PNAME}.exe")

  # When we call find_library() below, we're only looking for DLLs, so set the suffix list appropriately
  set (CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
  message (STATUS "Limiting find_library() suffixes to ${CMAKE_FIND_LIBRARY_SUFFIXES}")

  find_library (LIBGCC NAMES libgcc_s_sjlj-1 libgcc_s_seh-1 libgcc_s_dw2-1)
  if (LIBGCC)
    message (STATUS "Found libgcc at ${LIBGCC}")
  else ()
    message (SEND_ERROR "Could not find libgcc!")
  endif ()

  find_library (LIBSTDCPP libstdc++-6)
  if (LIBSTDCPP)
    message (STATUS "Found libstdc++ at ${LIBSTDCPP}")
  else ()
    message (SEND_ERROR "Could not find libstdc++!")
  endif ()

  find_library (ZLIB zlib1)
  if (ZLIB)
    message (STATUS "Found zlib1 at ${ZLIB}")
  else ()
    message (SEND_ERROR "Could not find zlib1!")
  endif ()

  find_library (QTCORE_DLL Qt5Core)
  if (QTCORE_DLL)
    message (STATUS "Found QtCore at ${QTCORE_DLL}")
  else ()
    message (SEND_ERROR "Could not find Qt5Core.dll! Check that the Qt5 bin/ directory (or the qtbase/bin/ directory) is in your PATH.")
  endif ()

  find_library (QTGUI_DLL Qt5Gui)
  if (QTGUI_DLL)
    message (STATUS "Found QtGui at ${QTGUI_DLL}")
  else ()
    message (SEND_ERROR "Could not find Qt5Gui.dll! Check that the Qt5 bin/ directory (or the qtbase/bin/ directory) is in your PATH.")
  endif ()

  find_library (QTWIDGETS_DLL Qt5Widgets)
  if (QTGUI_DLL)
    message (STATUS "Found QtWidgets at ${QTWIDGETS_DLL}")
  else ()
    message (SEND_ERROR "Could not find Qt5Widgets.dll! Check that the Qt5 bin/ directory (or the qtbase/bin/ directory) is in your PATH.")
  endif ()

  find_library (QWINDOWS_DLL qwindows)
  if (QWINDOWS_DLL)
    message (STATUS "Found Qt qwindows platform plugin at ${QWINDOWS_DLL}")
  else ()
    message (SEND_ERROR "Could not find Qt qwindows platform plugin!")
  endif ()

  find_library(LIBROSCO_DLL librosco PATHS ${LIBROSCO_PATH} ${CMAKE_INSTALL_PREFIX})
  if (LIBROSCO_DLL)
    message (STATUS "Found librosco at ${LIBROSCO_DLL}")
  else ()
    message (SEND_ERROR "Could not find librosco.dll! Check that it exists in one of the directories in your PATH.")
  endif ()

  target_link_libraries (${PNAME} ${LIBROSCO_DLL} Qt5::Widgets)

  # convert Unix-style newline characters into Windows-style
  configure_file ("${CMAKE_SOURCE_DIR}/README" "${CMAKE_BINARY_DIR}/README.TXT" NEWLINE_STYLE WIN32)
  configure_file ("${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE.TXT" NEWLINE_STYLE WIN32)

  install (FILES "${CMAKE_BINARY_DIR}/README.TXT" "${CMAKE_BINARY_DIR}/LICENSE.TXT" DESTINATION "doc")
  install (FILES ${EXE_FILE}
                 ${LIBGCC}
                 ${LIBSTDCPP}
                 ${QTCORE_DLL}
                 ${QTGUI_DLL}
                 ${QTWIDGETS_DLL}
                 ${LIBROSCO_DLL}
                 ${ZLIB}
           DESTINATION ".")
  install (FILES ${QWINDOWS_DLL} DESTINATION "platforms")

  set (CPACK_GENERATOR "ZIP")
  set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
  set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
  set (CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-${CMAKE_SYSTEM_NAME}")
  include (CPack)

else()
  message (STATUS "Defaulting to Linux build environment.")

  target_link_libraries (${PNAME} rosco Qt5::Widgets)

  set (CMAKE_SKIP_RPATH TRUE)
  set (CMAKE_INSTALL_PREFIX "/usr")
  set (EXE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PNAME}")

  # set the installation destinations for the header files,
  # shared library binaries, and reference utility
  install (FILES ${EXE_FILE} DESTINATION "bin"
           PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)

  install (DIRECTORY DESTINATION "share/doc/${PNAME}" DIRECTORY_PERMISSIONS
            OWNER_READ OWNER_EXECUTE OWNER_WRITE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)

  install (FILES "${CMAKE_SOURCE_DIR}/README"
                 "${CMAKE_SOURCE_DIR}/LICENSE"
           DESTINATION "share/doc/${PNAME}"
           PERMISSIONS
            OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

  # set the architecture flag that will be used by the Debian package builder
  if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
  elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
  endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

  set (CPACK_GENERATOR "DEB")
  set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
  set (CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR})
  set (CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR})
  set (CPACK_PACKAGE_VERSION_PATCH ${VER_PATCH})
  set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Colin Bourassa <colin.bourassa@gmail.com>")
  set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graphical display for data read from Rover MEMS 1.6 (Modular Engine Management System)")
  set (CPACK_DEBIAN_PACKAGE_SECTION "Science")
  set (CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.13), libstdc++6 (>= 4.6.3), librosco (>= 0.1.0), libqt5core5 (>= 5.0.2) | libqt5core5a (>= 5.2.1), libqt5gui5 (>= 5.0.2), libqt5widgets5 (>= 5.0.2)")
  set (CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}-${CMAKE_SYSTEM_NAME}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
  set (CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")

  include(CPack)

endif()

