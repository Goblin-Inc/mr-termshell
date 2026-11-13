cmake_minimum_required(VERSION 3.14)
project(mr-termshell CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

add_executable(mr-termshell
    src/main.cpp
    src/fs_utils.cpp
    src/ui_components.cpp
)

target_include_directories(mr-termshell PRIVATE src)
target_link_libraries(mr-termshell PRIVATE ftxui::screen ftxui::dom ftxui::component)

install(TARGETS mr-termshell DESTINATION /usr/local/bin)
