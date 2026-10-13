cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)
FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui)
FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_DIR)
set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG v3.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(ImGui-SFML)

file(GLOB_RECURSE ALL_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
set(GENERATED_ASSETS_HPP "${CMAKE_CURRENT_BINARY_DIR}/assets.hpp")
set(XXD_COMMAND_LIST "")
set(FIRST_FILE TRUE)

foreach(ASSET_FILE ${ALL_ASSETS})
    if(IS_DIRECTORY ${ASSET_FILE})
        continue()
    endif()
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets" ${ASSET_FILE})
    if(FIRST_FILE)
        list(APPEND XXD_COMMAND_LIST COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_SOURCE_DIR}/assets" xxd -i "${REL_PATH}" > ${GENERATED_ASSETS_HPP})
        set(FIRST_FILE FALSE)
    else()
        list(APPEND XXD_COMMAND_LIST COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_CURRENT_SOURCE_DIR}/assets" xxd -i "${REL_PATH}" >> ${GENERATED_ASSETS_HPP})
    endif()
endforeach()

add_custom_command(
    OUTPUT ${GENERATED_ASSETS_HPP}
    DEPENDS ${ALL_ASSETS}
    ${XXD_COMMAND_LIST}
    COMMENT "Generating assets.hpp"
)

add_custom_target(GenerateAssets DEPENDS ${GENERATED_ASSETS_HPP})
add_executable(main src/main.cpp)
add_dependencies(main GenerateAssets)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(main PRIVATE cxx_std_23)
target_link_libraries(main PRIVATE SFML::Graphics ImGui-SFML::ImGui-SFML)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
