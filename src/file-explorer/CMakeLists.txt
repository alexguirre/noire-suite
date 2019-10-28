cmake_minimum_required(VERSION 3.12)

set(FILE_EXPLORER_SOURCES
    "App.cpp"
    "App.h"
    "DirectoryContentsListCtrl.cpp"
    "DirectoryContentsListCtrl.h"
    "FileTreeCtrl.cpp"
    "FileTreeCtrl.h"
    "Identifiers.h"
    "MainWindow.cpp"
    "MainWindow.h"
)

add_executable(noire-file-explorer WIN32
    ${FILE_EXPLORER_SOURCES}
)

find_path(WX_INCLUDE_DIR wx/wx.h)
if (WX_INCLUDE_DIR STREQUAL WX_INCLUDE_DIR-NOTFOUND)
    message(FATAL_ERROR "wxWidgets not found")
endif()

target_include_directories(noire-file-explorer PRIVATE ${MSGSL_INCLUDE_DIR})
target_include_directories(noire-file-explorer PRIVATE ${WX_INCLUDE_DIR})

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
)

set(WX_LIB_DEBUG_DIR ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib)
set(WX_LIB_DIR ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib)

set(WX_LIBS
    optimized ${WX_LIB_DIR}/wxbase31u.lib debug ${WX_LIB_DEBUG_DIR}/wxbase31ud.lib
    optimized ${WX_LIB_DIR}/wxmsw31u_core.lib debug ${WX_LIB_DEBUG_DIR}/wxmsw31ud_core.lib
    optimized ${WX_LIB_DIR}/libpng16.lib debug ${WX_LIB_DEBUG_DIR}/libpng16d.lib
    optimized ${WX_LIB_DIR}/zlib.lib debug ${WX_LIB_DEBUG_DIR}/zlibd.lib
    comctl32
    Rpcrt4
)

target_link_libraries(noire-file-explorer PRIVATE
    noire-formats
    ${WX_LIBS}
)