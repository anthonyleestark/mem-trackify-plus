/**
 * @file		mem_trackify.cpp
 * @brief		A very simple, lightweight, header-only C++ library designed to
 *				track memory allocations and deallocations throughout the lifetime of a program.
 * @copyright	Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * @details		Released under MIT License
 *
 * @note		No need to add this file to your project if you are using C++ 17 or later
 *
 */


#include "mem_trackify.h"


// ================================================================================
// Global tracker definition
// ================================================================================

#if !_HAS_CXX17
	// Global tracker to handle memory allocations
	MemTrackifyPlus __mem_trackify_global::__g_mtpGlobalTracker;
#endif
