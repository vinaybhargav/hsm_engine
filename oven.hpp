#pragma once
#include "hsm_engine.hpp"
#include <iostream>
#include <string>

// 2. Forward declare Oven so states can use it
struct Oven; 

// 3. States now cleanly use Oven& instead of templates!
struct Top : State<Oven, Top> {}; 

struct Off : State<Oven, Off, Top> {
    void on_entry(Oven&) { std::cout << "  [OFF] Display Dark\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct On : State<Oven, On, Top> {
    void on_entry(Oven&) { std::cout << "  [ON] Light On\n"; }
    void on_exit(Oven&) { std::cout << "  [ON] Light Off\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct Idle : State<Oven, Idle, On> {
    void on_entry(Oven&) { std::cout << "    [IDLE] Ready\n"; }
    void on_exit(Oven&) { std::cout << "    [IDLE] Exit\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct Heating : State<Oven, Heating, On> {
    void on_entry(Oven&) { std::cout << "    [HEATING] Coils Red\n"; }
    void on_exit(Oven&) { std::cout << "    [HEATING] Coils Cool\n"; }
    bool handle_event(const std::string& e, Oven& ctx);
};

struct Oven {
    StateMachine<Oven, Off, On, Idle, Heating> sm;
    Oven() : sm(*this) { sm.template start<Off>(); }
};
