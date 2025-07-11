
# MemTrackify++

**MemTrackify++** is a very simple, lightweight, header-only C++ library designed to track memory allocations and deallocations throughout the lifetime of a program.  
It helps developers detect memory leaks by automatically cleaning up leaked memory upon program termination, ensuring optimal memory management with minimal overhead.    
Ideal for both single-threaded and multi-threaded applications, with optional debug and console reporting features.


## 📦 Features
- **Memory Tracking:** Automatically tracks all memory allocations and deallocations.
- **Automatic Cleanup:** Frees any leaked memory on program termination.
- **Leak Detection:** Identifies and reports memory leaks at anytime or on program termination (optional console report).
- **Minimalistic & Lightweight:** Simple integration with no dependencies and minimal performance overhead.
- **Thread-Safety:** Optional thread-safety via `std::recursive_mutex` to protect memory operations in multi-threaded environments..
- **Debug Mode:** Tracks file/line information for allocations in debug mode, making it easier to spot issues.
- **Global `new`/`delete` Override:** Overrides global `new` and `delete` for consistent memory tracking across the project.
- **Custom Allocators:** Supports custom template functions like `smartNew`, `smartNewArray`, `smartDelete`, and `smartDeleteArray` for better memory management.
- **Macros for Simplicity:** Optional macros like `track_new`, `debug_new`, `track_delete` and `debug_delete` for quick integration into your codebase.

## 🌟 Why use MemTrackify++?
- **🧠 Memory Management Simplified:** Track allocations and deallocations effortlessly.
- **⚡ Minimal Overhead:** Designed to be as lightweight as possible while ensuring full tracking capabilities.
- **🔧 Easy to Use:** Integrates with just a single header file, with no external dependencies.
- **🛡️ Leak-Free Programs:** Automatically handles memory cleanup on termination to prevent leaks.  

Perfect for developers who need better control over memory management in C++ with minimal setup!


## 🚀 Getting Started

### 1. Include the Header

Add the `"mem_trackify.h"` header file to your project.  
Include it wherever you want (preferably in your top-most or precompiled header file).

```cpp
#include "mem_trackify.h"
```

> ⚠️ **Note:** 
>   If you're using a version of C++ **before C++17** _(C++ 98/03/11/14)_, you must also add the `"mem_trackify.cpp"` file to your project.  
>   Alternatively, you can add this definition in **one** of your translation units/source files (`.cpp`):

```cpp
MemTrackifyPlus GlobalMemTracker::globalTracker_;
```


### 2. Enable Features (optional)
Before including the header file, define any desired macros:

```cpp
#define _MTP_DEBUG
#define _MTP_THREADSAFETY
#define _MTP_CONSOLE_REPORT_ON_TERMINATION
#include "mem_trackify.h"
```

> ⚠️ **Note:** 
>   It's best to define these macros in the **Preprocessor Definitions** section of your project settings.


## 🛠 Macros & Modes

| Macro                                 | Description                                                       |
|---------------------------------------|-------------------------------------------------------------------|
| `_MTP_DEBUG`                          | Enable debug mode (tracking filename and line number).            |
| `_MTP_THREADSAFETY`                   | Ensure thread-safety during memory allocations and deallocations. |
| `_MTP_CONSOLE_REPORT_ON_TERMINATION`  | Show leak report at program exit (for console application only).  |
| `_MTP_NO_OVERRIDE_GLOBAL_OPERATORS`   | Do **not** override global `new`/`delete` operators.              |


## 🔧 Usage Examples

### You can forget the manual `delete` operator when using this library

```cpp
#include "mem_trackify.h"

class MyClass {
    int data[1000000];
};

int main()
{
    MyClass* ptr = new MyClass;
    int* arr = new int[100000];

    // delete ptr;
    // delete[] arr;

    // The allocated memory will be automatically freed
    // before the program terminates
}
```

> ⚠️ **Important Notes:** 
>   - This library **only** helps in cases where you forget **manual deletions**, it is not intended to eliminate the use of `delete` operators.  
>   - You should still manage memory carefully, but **MemTrackify++** will assist in cleaning up any missed deletions and prevent memory leaks on termination.  
>   - **MemTrackify++** is **not a garbage collector**. Any leaked memory will **only** be automatically freed **before the program terminates, not during runtime**.  
>   - If you need a pointer that **automatically frees when it goes out of scope**, consider using `STL Smart Pointers` (like `std::unique_ptr` or `std::shared_ptr`) for better practice and automatic memory management.  

### Basic allocation with overridden global `new`/`delete` operators (without definition of `_MTP_NO_OVERRIDE_GLOBAL_OPERATORS`)

```cpp
int* data = new int[100000];
delete[] data;
```

### Basic allocation with smart tracking macros

```cpp
int* data = track_new int[100000];
track_delete[] data;
```

### Debug allocation (with `_MTP_DEBUG`)

```cpp
int* debugData = debug_new int[100000];
debug_delete[] debugData;
```

### Using `smartNew` and `smartDelete`

```cpp
MyClass *obj = smartNew<MyClass>(constructor_args);
int *num = smartNew<int>(30);      // Allocates memory and assigns value of 30 for 'num'
smartDelete(obj);
smartDelete(num);
```

### Using `smartNewArray` and `smartDeleteArray`

```cpp
MyClass *arr_obj = smartNewArray<MyClass>(3);  // Creates an array of MyClass with 3 elements
int *nums = smartNewArray<int>(15);            // Creates an array of int with 15 elements
smartDeleteArray(obj);
smartDeleteArray(num);
```


## 🔍 Querying for Leaks and Tracked Information
You can use the global memory tracker instance to query for memory leaks and retrieve tracked information during or after program execution.  
This allows you to inspect the allocations and ensure everything has been properly cleaned up.  

```cpp
auto* tracker = getGlobalMemTracker();
if (tracker->isMemoryLeak()) {
    size_t trackerSize = tracker->getTrackerSize();
    std::cout << "Memory tracker size: " << trackerSize << " bytes.\n";
    size_t ptrCount = tracker->getPtrCount();
    std::cout << "Number of allocated pointers: " << ptrCount << ".\n";
    size_t allocMemSize = tracker->getMemorySize();
    std::cout << "Memory allocated size: " << allocMemSize << " bytes.\n";
    tracker->printLeakInfo(std::cout);
}
```


## ⚙️ Thread-Safety
If you're working in a multi-threaded environment, enable the `_MTP_THREADSAFETY` flag.  
This will provide an internal thread-locking mechanism to ensure thread-safety during memory allocations and deallocations, particularly when dealing with recursive memory management or parallel operations.  


## 🤝 Contributing
We welcome contributions to **MemTrackify++**!  
If you'd like to improve the library, fix bugs, or add new features, please follow these guidelines:  

### How to Contribute
 1. Fork the repository
 2. Create a branch and make your changes
 3. Create a Pull Request

### Code of Conduct
By participating in this project, you agree to follow our [Code of Conduct](CODE_OF_CONDUCT.md). Please treat others with respect and kindness.

### Issues and Bug Reports
If you find a bug or have suggestions for improvements, feel free to open an issue in the **Issues** tab.  
Be sure to provide as much detail as possible, including:
- Steps to reproduce the issue.
- The expected and actual behavior.
- Any relevant logs or error messages.


## 📄 License
Copyright © 2025 Anthony Lee Stark. All rights reserved.  
This repository is open-source and available under the [MIT License](https://opensource.org/license/mit).  


## 🤖 Author
Developed by **Anthony Lee Stark** `(@anthonyleestark)`.  