#include "oven.hpp"

// 4. Implement logic AFTER Oven is fully defined in the header
bool Off::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TURN_ON") { ctx.sm.template transition<Idle>(); return true; }
    return false;
}

bool On::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TURN_OFF") { ctx.sm.template transition<Off>(); return true; }
    return false;
}

bool Idle::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TOO_COLD") { ctx.sm.template transition<Heating>(); return true; }
    return false;
}

bool Heating::handle_event(const std::string& e, Oven& ctx) {
    if (e == "TOO_HOT") { ctx.sm.template transition<Idle>(); return true; }
    return false;
}

int main() {
    Oven oven;
    
    std::cout << "\n>> Dispatching TURN_ON\n";
    oven.sm.dispatch("TURN_ON");
    
    std::cout << "\n>> Dispatching TOO_COLD\n";
    oven.sm.dispatch("TOO_COLD");

    std::cout << "\n>> Dispatching TURN_OFF\n";
    oven.sm.dispatch("TURN_OFF"); // Bubbles up to 'On'
    
    return 0;
}
