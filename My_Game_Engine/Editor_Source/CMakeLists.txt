# Editor 라이브러리
add_library(Editor STATIC)

# 소스/헤더 수집 및 
file(GLOB_RECURSE EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# VS 솔루션 필터
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_SOURCES})

target_sources(Editor PRIVATE ${EDITOR_SOURCES})
target_include_directories(Editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 공통 pch.h
target_precompile_headers(Editor PRIVATE
    "${CMAKE_SOURCE_DIR}/My_Game_Engine/My_Game_Engine/pch.h"
)

target_compile_definitions(Editor PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
