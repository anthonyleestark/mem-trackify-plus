/**
 * @file		smart-gc.cpp
 * @description A very simple C++ Smart Garbage Collection Library
 * @copyright	Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * @license		Released under MIT License
 *
 * @note		No need to add this file to your project if you are using C++ 17 or later
 *
 */


#include "smart-gc.h"


// ================================================================================
// Global tracker definition
// ================================================================================

#if !_HAS_CXX17
	// Global tracker to handle memory allocations
	SmartGarbageCollector __smart_gc_global::__g_gcSmartGarbageCollector;
#endif
