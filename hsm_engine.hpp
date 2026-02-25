#pragma once
#include <variant>
#include <type_traits>
#include <string>

struct None {}; 

// --- 1. Compile-Time Hierarchy Resolution ---
namespace detail {
    // Checks if 'Ancestor' is in the parent chain of 'T'
    template <typename Ancestor, typename T>
    struct is_ancestor {
        static constexpr bool value = std::is_same_v<Ancestor, T> || 
            (!std::is_same_v<typename T::ParentType, None> && is_ancestor<Ancestor, typename T::ParentType>::value);
    };

    template <typename Ancestor>
    struct is_ancestor<Ancestor, None> {
        static constexpr bool value = false;
    };

    template <typename Ancestor, typename T>
    inline constexpr bool is_ancestor_v = is_ancestor<Ancestor, T>::value;

    // Finds the Least Common Ancestor (LCA) between A and B
    template <typename A, typename B>
    struct common_ancestor {
        using type = std::conditional_t<
            is_ancestor_v<A, B>, 
            A, 
            typename common_ancestor<typename A::ParentType, B>::type
        >;
    };

    template <typename B>
    struct common_ancestor<None, B> {
        using type = None;
    };

    template <typename A, typename B>
    using common_ancestor_t = typename common_ancestor<A, B>::type;
}

// --- 2. State and StateMachine ---

template <typename Context, typename Derived, typename Parent = None>
struct State {
    using ParentType = Parent;

    void on_entry(Context&) {}
    void on_exit(Context&) {}
    bool handle_event(const std::string&, Context&) { return false; }
};

template <typename Context, typename... States>
class StateMachine {
    std::variant<States...> current_state;
    Context& ctx;

public:
    StateMachine(Context& c) : ctx(c) {}

    template <typename InitialState>
    void start() {
        // Enter from top level down to InitialState
        do_entries<InitialState, None>();
        current_state.template emplace<InitialState>();
    }

    template <typename TargetState>
    void transition() {
        // 1. Visit current state to determine its exact type at compile time
        std::visit([this](auto& current_leaf) {
            using CurrentType = std::decay_t<decltype(current_leaf)>;
            using LCA = detail::common_ancestor_t<CurrentType, TargetState>;
            
            // 2. Execute hierarchical exits up to LCA
            this->template do_exits<CurrentType, LCA>();
            
            // 3. Execute hierarchical entries down from LCA to Target
            this->template do_entries<TargetState, LCA>();
            
        }, current_state);

        // 4. Update the variant to hold the new state
        current_state.template emplace<TargetState>();
    }

    void dispatch(const std::string& e) {
        std::visit([&](auto& state) { this->bubble_event(state, e); }, current_state);
    }

private:
    // Recursive Exit: From Current up to LCA (exclusive)
    template <typename Current, typename TargetLCA>
    void do_exits() {
        if constexpr (!std::is_same_v<Current, TargetLCA>) {
            Current temp;         // Instantiate stateless temporary
            temp.on_exit(ctx);    // Fire exit
            if constexpr (!std::is_same_v<typename Current::ParentType, None>) {
                do_exits<typename Current::ParentType, TargetLCA>();
            }
        }
    }

    // Recursive Entry: From LCA (exclusive) down to Target
    template <typename Target, typename TargetLCA>
    void do_entries() {
        if constexpr (!std::is_same_v<Target, TargetLCA>) {
            if constexpr (!std::is_same_v<typename Target::ParentType, TargetLCA>) {
                do_entries<typename Target::ParentType, TargetLCA>(); // Dig down first
            }
            Target temp;          // Instantiate stateless temporary
            temp.on_entry(ctx);   // Fire entry on the way back up the call stack
        }
    }

    // Hierarchical Event Bubbling
    template <typename S>
    bool bubble_event(S& state, const std::string& e) {
        if (state.handle_event(e, ctx)) return true;
        if constexpr (!std::is_same_v<typename S::ParentType, None>) {
            typename S::ParentType parent; 
            return bubble_event(parent, e); 
        }
        return false;
    }
};