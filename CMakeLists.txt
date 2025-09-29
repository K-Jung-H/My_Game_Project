cmake_minimum_required(VERSION 3.16)

project(MyGame LANGUAGES CXX)

# ── 전체 타겟 공통: C++ 미리 보기(Preview) 표준
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
add_compile_definitions(UNICODE _UNICODE)

# Visual Studio : ZERO_CHECK 숨기기
set(CMAKE_SUPPRESS_REGENERATION ON)


# 하위 모듈 추가 
add_subdirectory(My_Game_Engine/My_Game_Engine/Engine)
add_subdirectory(My_Game_Engine/Editor_Source)


# 실행 파일
add_executable(MyGame WIN32
    My_Game_Engine/My_Game_Engine/My_Game_Project.cpp
    My_Game_Engine/My_Game_Engine/pch.cpp
    # --- 윈도우 파일 추가 ---
    My_Game_Engine/My_Game_Engine/My_Game_Engine.rc
    My_Game_Engine/My_Game_Engine/Resource.h
    My_Game_Engine/My_Game_Engine/My_Game_Engine.ico
    My_Game_Engine/My_Game_Engine/small.ico
)

# 디버그 작업 디렉터리
set_target_properties(MyGame PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>"
)

# 디버깅 용도 PDB 생성
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE    "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE    "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

#  Visual Studio 시작 프로젝트 지정
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MyGame)

# 공통 pch.h 
target_precompile_headers(MyGame PRIVATE
    "${CMAKE_SOURCE_DIR}/My_Game_Engine/My_Game_Engine/pch.h"
)


# Engine, Editor 링크
target_link_libraries(MyGame PRIVATE
    Engine
    Editor
)


# 리소스 복사: Assets & Shaders 
set(GAME_ASSET_DIR  "${CMAKE_SOURCE_DIR}/My_Game_Engine/My_Game_Engine/Assets")
set(GAME_SHADER_DIR "${CMAKE_SOURCE_DIR}/My_Game_Engine/My_Game_Engine/Shaders")

add_custom_command(
    TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${GAME_ASSET_DIR}  $<TARGET_FILE_DIR:MyGame>/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${GAME_SHADER_DIR} $<TARGET_FILE_DIR:MyGame>/Shaders
)