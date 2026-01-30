cmake_minimum_required(VERSION 3.16)
project(legacy-notepad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX /permissive- /utf-8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")
else()
    add_compile_options(-Wall -Wextra -Werror -municode -Wno-error=missing-field-initializers)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()

add_executable(legacy-notepad WIN32
    src/main.cpp
    src/notepad.rc
)

target_link_libraries(legacy-notepad PRIVATE
    comctl32
    shlwapi
    comdlg32
    shell32
    gdiplus
    msimg32
    user32
    gdi32
    kernel32
)

if(NOT MSVC)
    target_link_options(legacy-notepad PRIVATE -mwindows -municode)
    target_link_libraries(legacy-notepad PRIVATE -static-libgcc -static-libstdc++)
    add_custom_command(TARGET legacy-notepad POST_BUILD COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:legacy-notepad>)
endif()

target_compile_definitions(legacy-notepad PRIVATE UNICODE _UNICODE)
