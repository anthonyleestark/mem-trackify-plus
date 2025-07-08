/*
*
 * smart-gc.h
 * A very simple C++ Smart Garbage Collection Library
 * Copyright (c) 2025 Anthony Lee Stark. All rights reserved.
 * Released under MIT License
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
