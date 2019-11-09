cmake_minimum_required(VERSION 3.12)

file(GLOB FILE_EXPLORER_SOURCES
    "App.cpp"
    "App.h"
    "Identifiers.h"
    "Images.cpp"
    "Images.h"
    "controls/DirectoryContentsListCtrl.cpp"
    "controls/DirectoryContentsListCtrl.h"
    "controls/DirectoryTreeCtrl.cpp"
    "controls/DirectoryTreeCtrl.h"
    "controls/ImagePanel.cpp"
    "controls/ImagePanel.h"
    "controls/PathToolBar.cpp"
    "controls/PathToolBar.h"
    "controls/StatusBar.cpp"
    "controls/StatusBar.h"
    "util/DirectoryHistory.cpp"
    "util/DirectoryHistory.h"
    "util/Format.cpp"
    "util/Format.h"
    "windows/ImageWindow.cpp"
    "windows/ImageWindow.h"
    "windows/MainWindow.cpp"
    "windows/MainWindow.h"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FILE_EXPLORER_SOURCES})

add_executable(noire-file-explorer WIN32
    ${FILE_EXPLORER_SOURCES}
)

find_path(WX_INCLUDE_DIR wx/wx.h)
if (WX_INCLUDE_DIR STREQUAL WX_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR "wxWidgets not found")
endif()

find_path(DEVIL_INCLUDE_DIR IL/il.h)
if (DEVIL_INCLUDE_DIR STREQUAL DEVIL_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR "DevIL not found")
endif()

target_include_directories(noire-file-explorer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MSGSL_INCLUDE_DIR}
    ${WX_INCLUDE_DIR}
    ${DEVIL_INCLUDE_DIR}
)

get_target_property(FORMATS_INCLUDE_DIR noire-formats SOURCE_DIR)
get_filename_component(FORMATS_INCLUDE_DIR ${FORMATS_INCLUDE_DIR} DIRECTORY)
if (FORMATS_INCLUDE_DIR STREQUAL FORMATS_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR "noire-formats not found")
else()
    target_include_directories(noire-file-explorer PRIVATE ${FORMATS_INCLUDE_DIR})
endif()

target_compile_definitions(noire-file-explorer PRIVATE
    wxUSE_GUI=1
    __WXMSW__
    _CRT_SECURE_NO_WARNINGS
    IL_STATIC_LIB
)

set(VCPKG_LIBS_DEBUG_DIR ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib)
set(VCPKG_LIBS_DIR ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)

set(WX_LIBS
    optimized ${VCPKG_LIBS_DIR}/wxbase31u.lib debug ${VCPKG_LIBS_DEBUG_DIR}/wxbase31ud.lib
    optimized ${VCPKG_LIBS_DIR}/wxmsw31u_core.lib debug ${VCPKG_LIBS_DEBUG_DIR}/wxmsw31ud_core.lib
    optimized ${VCPKG_LIBS_DIR}/libpng16.lib debug ${VCPKG_LIBS_DEBUG_DIR}/libpng16d.lib
    optimized ${VCPKG_LIBS_DIR}/zlib.lib debug ${VCPKG_LIBS_DEBUG_DIR}/zlibd.lib
    comctl32
    Rpcrt4
)

set(DEVIL_LIBS
    optimized ${VCPKG_LIBS_DIR}/DevIL.lib debug ${VCPKG_LIBS_DEBUG_DIR}/DevIL.lib
    optimized ${VCPKG_LIBS_DIR}/jasper.lib debug ${VCPKG_LIBS_DEBUG_DIR}/jasperd.lib
    optimized ${VCPKG_LIBS_DIR}/jpeg.lib debug ${VCPKG_LIBS_DEBUG_DIR}/jpegd.lib
    optimized ${VCPKG_LIBS_DIR}/lcms2.lib debug ${VCPKG_LIBS_DEBUG_DIR}/lcms2d.lib
    optimized ${VCPKG_LIBS_DIR}/libpng16.lib debug ${VCPKG_LIBS_DEBUG_DIR}/libpng16d.lib
    optimized ${VCPKG_LIBS_DIR}/tiff.lib debug ${VCPKG_LIBS_DEBUG_DIR}/tiffd.lib
    optimized ${VCPKG_LIBS_DIR}/tiffxx.lib debug ${VCPKG_LIBS_DEBUG_DIR}/tiffxxd.lib
    optimized ${VCPKG_LIBS_DIR}/lzma.lib debug ${VCPKG_LIBS_DEBUG_DIR}/lzmad.lib
)

add_dependencies(noire-file-explorer
    noire-formats
)

target_link_libraries(noire-file-explorer PRIVATE
    noire-formats
    ${WX_LIBS}
    ${DEVIL_LIBS}
)