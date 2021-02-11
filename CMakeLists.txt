cmake_minimum_required (VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs")  
set(CMAKE_REQUIRED_FLAGS -std=c++17 -lstdc++fs)

link_libraries(stdc++fs)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG master
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# ------------------------------------------------------------------------------

project(vortex_TUI
  LANGUAGES CXX
  VERSION 1.0.0
)

#find_package(catkin REQUIRED COMPONENTS roslib roscpp)
#catkin_package(INCLUDE_DIRS include CATKIN_DEPENDS roscpp roslib)
#include_directories(include ${catkin_INCLUDE_DIRS})

find_package( roscpp REQUIRED)
find_package( roslib REQUIRED)
include_directories( ${roslib_INCLUDE_DIRS} ${roscpp_INCLUDE_DIRS} )

add_executable(vortex_TUI src/main.cpp)
target_include_directories(vortex_TUI PRIVATE src)

target_link_libraries(vortex_TUI
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component # Not needed for this example.
)

# C++17 is used. We requires fold expressions at least.
set_target_properties(vortex_TUI PROPERTIES CXX_STANDARD 17)

install(TARGETS vortex_TUI RUNTIME DESTINATION "bin")