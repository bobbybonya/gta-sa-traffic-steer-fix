cmake_minimum_required(VERSION 3.20)
project(TrafficFix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PLUGIN_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/plugin-sdk")

# SDK-shared .cpp — исключаем Audio.cpp
file(GLOB PLUGIN_SDK_SHARED_CPP "${PLUGIN_SDK_DIR}/shared/*.cpp")
list(FILTER PLUGIN_SDK_SHARED_CPP EXCLUDE REGEX ".*Audio\\.cpp$")

# SDK-shared/game .cpp — CRGBA, CVector и другие базовые типы
file(GLOB PLUGIN_SDK_SHARED_GAME_CPP "${PLUGIN_SDK_DIR}/shared/game/*.cpp")

# SDK-game_sa .cpp — методы CVehicle, CPools, CTimer и др.
file(GLOB PLUGIN_SDK_GAMESA_CPP "${PLUGIN_SDK_DIR}/plugin_sa/game_sa/*.cpp")

add_library(TrafficFix SHARED
    src/main.cpp
    ${PLUGIN_SDK_SHARED_CPP}
    ${PLUGIN_SDK_SHARED_GAME_CPP}
    ${PLUGIN_SDK_GAMESA_CPP}
)

# Автоматически подключаем все директории с заголовочными файлами SDK
file(GLOB_RECURSE PLUGIN_SDK_HEADERS "${PLUGIN_SDK_DIR}/plugin_sa/*.h" "${PLUGIN_SDK_DIR}/shared/*.h")
set(PLUGIN_SDK_INC_DIRS "${PLUGIN_SDK_DIR}/plugin_sa" "${PLUGIN_SDK_DIR}/plugin_sa/game_sa" "${PLUGIN_SDK_DIR}/shared" "${PLUGIN_SDK_DIR}/shared/game")
foreach(_header ${PLUGIN_SDK_HEADERS})
    get_filename_component(_dir ${_header} DIRECTORY)
    list(APPEND PLUGIN_SDK_INC_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES PLUGIN_SDK_INC_DIRS)

target_include_directories(TrafficFix PRIVATE
    ${PLUGIN_SDK_INC_DIRS}
)

set_target_properties(TrafficFix PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
)

target_compile_options(TrafficFix PRIVATE /std:c++latest /permissive-)
target_compile_definitions(TrafficFix PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
    GTASA
    RW
)

# Директории с .lib-файлами SDK
target_link_directories(TrafficFix PRIVATE
    "${PLUGIN_SDK_DIR}/shared/dxsdk"
    "${PLUGIN_SDK_DIR}/shared/rwd3d9"
)

# Необходимые библиотеки
target_link_libraries(TrafficFix PRIVATE
    d3d9.lib
    dinput8.lib
    dxguid.lib
)