# Lightweight CRTP Hierarchical State Machine (HSM)

A highly optimized, zero-allocation Hierarchical State Machine (HSM) framework written in standard C++17. 

Designed for performance-critical and resource-constrained environments (like embedded systems or high-frequency trading), this engine completely eliminates dynamic memory allocation and virtual function overhead by leveraging the Curiously Recurring Template Pattern (CRTP) and `std::variant`.

## 🚀 Key Features

* **Zero Heap Allocation:** No `new`, `delete`, or STL containers (like `std::vector` or `std::shared_ptr`) are used. The entire state machine memory footprint is statically allocated on the stack.
* **Zero Virtual Functions:** Eliminates vtable lookup overhead. Polymorphism is handled statically via `std::visit`.
* **Compile-Time Hierarchy:** Event bubbling and Least Common Ancestor (LCA) transition paths (for `on_entry` and `on_exit` chains) are calculated natively by the compiler using `if constexpr`.
* **Strictly Typed:** States are stateless tag classes; application data is cleanly isolated in a user-defined `Context` class.

---

## 🛠️ Architecture

Instead of allocating state objects on the heap and chaining them with pointers, this engine stores the currently active state inside a C++17 `std::variant`. 

States inherit from a CRTP base class, defining their parent. When an event is dispatched, if the current leaf state does not handle it, the engine recursively instantiates the parent state type and attempts to handle it—all resolved during compilation.

---

## 📖 Quick Start Guide

### 1. Include the Engine
Drop `hsm_engine.hpp` into your project. Ensure your compiler is set to C++17 or higher.

### 2. Define Your Context & States (`oven.hpp`)
Forward-declare your Context class. Define your states using the CRTP base class `State<Context, Derived, Parent>`. The `Parent` defaults to `None` if omitted.

```cpp
#pragma once
#include "hsm_engine.hpp"
#include <iostream>
#include <string>

// 1. Forward declare your context
struct Oven; 

// 2. Define your state hierarchy
struct Top : State<Oven, Top> {}; 

struct Off : State<Oven, Off, Top> {
    void on_entry(Oven&) { std::cout << "Display Dark\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct On : State<Oven, On, Top> {
    void on_entry(Oven&) { std::cout << "Light On\n"; }
    void on_exit(Oven&) { std::cout << "Light Off\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct Heating : State<Oven, Heating, On> {
    void on_entry(Oven&) { std::cout << "Coils Red\n"; }
    void on_exit(Oven&) { std::cout << "Coils Cool\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

// 3. Define the Context and instantiate the StateMachine variant
struct Oven {
    StateMachine<Oven, Off, On, Heating> sm;
    Oven() : sm(*this) { sm.template start<Off>(); }
};
```

### 3. Implement the Logic  (`oven.cpp`)

To ensure the engine handles hierarchical transitions correctly, you must implement your transition logic in a `.cpp` file after the `Context` (e.g., `Oven`) is fully defined. This avoids circular dependency errors.

#### Handling Transitions
Use `ctx.sm.template transition<TargetState>()` to move between states. The engine will automatically calculate the entry/exit chain.

```cpp
#include "oven.hpp"

// Implementation of event handling for the 'Off' state
bool Off::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TURN_ON") { 
        ctx.sm.template transition<On>(); 
        return true; 
    }
    return false;
}

// Implementation of event handling for the 'On' state
bool On::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TURN_OFF") { 
        ctx.sm.template transition<Off>(); 
        return true; 
    }
    return false;
}

// Implementation of event handling for the 'Heating' state
bool Heating::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TOO_HOT") { 
        ctx.sm.template transition<On>(); 
        return true; 
    }
    // If an event isn't handled here, the engine automatically 
    // bubbles it up to the 'On' state.
    return false; 
}
```

## ⚙️ Requirements

* **Language Standard:** C++17 or later (utilizes `std::variant`, `std::visit`, and `if constexpr`).
* **Compiler Support:**
    * **GCC:** 7.1+
    * **Clang:** 5.0+
    * **MSVC:** 19.14+ (Visual Studio 2017 version 15.7 or newer)
* **Dependencies:** None. This is a header-only framework relying strictly on the C++ Standard Library.
* **Memory Profile:** Zero dynamic allocation (no `heap` usage). Memory footprint is determined at compile-time based on the size of the largest state in the `std::variant`.


## ⚠️ Common Pitfalls you may encounter

### 1. Missing the `template` Keyword
When calling `transition` from within a state (where the `Context` is a template parameter or the `StateMachine` is dependent), you **must** use the `template` keyword.
* **Wrong:** `ctx.sm.transition<TargetState>();`
* **Right:** `ctx.sm.template transition<TargetState>();`
* *Why:* Without it, the compiler may treat the `<` as a "less than" operator rather than the start of a template argument list.

### 2. Infinite Transition Loops
If you trigger a `transition()` inside an `on_entry()` or `on_exit()` function, ensure there is a guard condition. Otherwise, you can create a recursive loop that leads to a stack overflow.

### 3. Circular Dependencies (`most common`)
Defining `handle_event` logic inside the header file usually leads to compilation errors because the `StateMachine` needs to know the size of all states, while states need to know the members of the `Context`. 
* **Solution:** Always declare your states in the `.hpp` and implement the transition logic in the `.cpp`.

### 4. State Fragmentation
Since the engine uses `std::variant`, the `StateMachine` object will be as large as its largest state. 
* **Tip:** Keep your state classes stateless (as "tags"). Store all persistent data (timers, temperatures, counters) in the `Context` class instead of the states.

### 5. Forgetting to Return `true`
If `handle_event` returns `false`, the engine will bubble the event up to the parent state. 
* **Pitfall:** If you handle an event but accidentally return `false`, the parent might trigger a second, unintended transition for the same event.
