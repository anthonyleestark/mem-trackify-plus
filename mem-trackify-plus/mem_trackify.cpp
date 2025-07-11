// MemTrackify++
// Copyright (c) 2025 Anthony Lee Stark
// SPDX-License-Identifier: MIT


// ================================================================================
// Important Note: 
// No need to include this file to your project if you are using C++ 17 or later
// ================================================================================

#include "mem_trackify.h"


// ================================================================================
// Global tracker definition
// ================================================================================

#if !_HAS_CXX17
	// Global tracker to handle memory allocations
	MemTrackifyPlus GlobalMemTracker::globalTracker_;
#endif
