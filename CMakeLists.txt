cmake_minimum_required(VERSION 3.21)
project(fltk_demo_editor)

set(CMAKE_CXX_STANDARD 14)

find_package(FLTK REQUIRED NO_MODULE)

add_executable(fltk_demo_editor WIN32 MACOSX_BUNDLE main.cpp)

if (APPLE)
    target_link_libraries(fltk_demo_editor "-framework cocoa")
endif (APPLE)

target_include_directories(fltk_demo_editor PUBLIC ${FLTK_INCLUDE_DIRS})

target_link_libraries(fltk_demo_editor fltk fltk_images fltk_forms fltk_png fltk_jpeg fltk_gl fltk_z)