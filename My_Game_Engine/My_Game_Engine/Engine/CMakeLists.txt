# Engine 라이브러리
add_library(Engine STATIC)

# Windows 실행 파일 아님
set_target_properties(Engine PROPERTIES
    WIN32_EXECUTABLE OFF     
    MACOSX_BUNDLE    OFF
)

# 소스/헤더 수집 및 
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# VS 필터
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SOURCES})

target_sources(Engine PRIVATE ${ENGINE_SOURCES})
target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 공통 pch.h 
target_precompile_headers(Engine PRIVATE
    "${CMAKE_SOURCE_DIR}/My_Game_Engine/My_Game_Engine/pch.h"
)

# HLSL 파일 
file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Shader/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shader/*.hlsli"
)

source_group("Shaders" FILES ${SHADER_FILES})



# 엔진 타겟에 포함 (빌드 제외)
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY ON)
target_sources(Engine PRIVATE ${SHADER_FILES})
