#pragma once

//------------------------------------------------------------------------------
// targetver.h
// Defines the highest available Windows platform for this build.
//
// By including SDKDDKVer.h, this project will target the latest Windows SDK
// installed on your system (recommended for DirectX 12 projects).
//
// If you need to target an older Windows version, include WinSDKVer.h first
// and define _WIN32_WINNT manually before including SDKDDKVer.h.
//------------------------------------------------------------------------------

// Example of targeting Windows 10 (0x0A00):
// #define _WIN32_WINNT 0x0A00
// #define WINVER _WIN32_WINNT
// #include <WinSDKVer.h>

#include <SDKDDKVer.h>
