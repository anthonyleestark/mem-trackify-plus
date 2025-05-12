
# Smart Garbage Collector for C++

A very simple, lightweight, header-only C++ smart garbage collection library designed to track memory allocations and detect memory leaks.  
Ideal for both single-threaded and multi-threaded applications, with optional debug and console reporting features.

---

## 📦 Features

- Automatic tracking of memory allocations and deallocations.
- Automatic freeing memory leaks on termination.
- Leak detection with detailed reporting.
- Optional thread-safety with `std::recursive_mutex`.
- Debug mode with file/line tracking.
- Console memory leak report on termination (optional).
- Can override or skip overriding global `new`/`delete`.
- Optional custom template: `gcNew/gcNewArray`, `gcDelete/gcDeleteArray`.
- Optional macros: `smart_new`, and `debug_new`.

---

## 🚀 Getting Started

### 1. Include the Header

```cpp
#include "smart-gc.h"
```

### 2. Enable Features (optional)

Before including the header, define any desired macros:

```cpp
#define SMART_GC_ENABLED
#define SMART_GC_DEBUG
#define SMART_GC_THREADSAFETY
#define SMART_GC_CONSOLE_REPORT_ON_TERMINATION
#include "smart-gc.h"
```

---

## 🛠 Macros & Modes

| Macro                                 | Description |
|--------------------------------------|-------------|
| `SMART_GC_DEBUG`                     | Enable debug tracking (filename, line number). |
| `SMART_GC_THREADSAFETY`             | Use mutex for multi-threaded safety. |
| `SMART_GC_CONSOLE_REPORT_ON_TERMINATION` | Show leak report at program exit (console only). |
| `SMART_GC_NOTOVERRIDE_GLOBAL_NEW`   | Do **not** override global `new` operator. |
| `SMART_GC_NOTOVERRIDE_GLOBAL_DELETE`| Do **not** override global `delete` operator. |

---

## 🔧 Usage Examples

### You can forget the manual `delete` operator when using this library

```cpp
#include "smart-gc.h"

class MyClass {
    int data[100000];
};

int main()
{
    MyClass* ptr = new MyClass;
    int* data = new int[100];

    // delete ptr;
    // delete[] data;

    // The allocated memory will be automatically freed
    // before the program terminates
}
```
> ⚠️ **Note:** 
>     The allocated memory will **only** be automatically freed before the program terminates.
>     If you want a pointer which automatically frees when goes out of scope, use `STL Smart Pointers` instead.

### Basic allocation with overriden global new/delete operators (without `SMART_GC_NOTOVERRIDE_GLOBAL_NEW/DELETE`)

```cpp
int* data = new int[100];
delete[] data;
```

### Basic allocation with smart macros

```cpp
int* data = smart_new int[100];
smart_delete[] data;
```

### Debug allocation (with `SMART_GC_DEBUG`)

```cpp
int* debugData = debug_new int;
debug_delete debugData;
```

### Using `gcNew` and `gcDelete`

```cpp
MyClass *obj = gcNew<MyClass>(constructor_args);
int *num = gcNew<int>(30);      // Set value = 30 for 'num'
gcDelete(obj);
gcDelete(num);
```

### Using `gcNewArray` and `gcDeleteArray`

```cpp
MyClass *arr_obj = gcNewArray<MyClass>(3);  // Create an array of MyClass with 3 elements
int *nums = gcNewArray<int>(15);            // Create an array of int with 15 elements
gcDeleteArray(obj);
gcDeleteArray(num);
```

---

## 📋 Leak Detection API

You can use the global collector instance to query for leaks:

```cpp
auto* gc = gcGetSmartGarbageCollector();
if (gc->gcIsMemoryLeak()) {
    gc->gcPrintLeakInfo(std::cout);
}
```

---

## ⚙️ Thread-Safety

Enable `SMART_GC_THREADSAFETY` if using in a multi-threaded environment.  
This will adds internal thread-locking mechanism to ensure thread-safety, especially when you're working on multi-threaded application or using recursive allocation/deallocation architecture.

---

## 📄 License

MIT License  
© 2025 Anthony Lee Stark. All rights reserved.

---

## 🤖 Author

Developed by **Anthony Lee Stark** `(@anthonyleestark)`.
