#include "vex/Thread.hpp"

#include "pros/rtos.hpp"

#include <new>
#include <memory>

Thread::Thread(Callback callback, void* user_data)
    : state_(std::make_shared<State>(callback, user_data)) {
    auto* task_state = new std::shared_ptr<State>(state_);
    const auto task = pros::c::task_create(
        [](void* parameter) {
            std::unique_ptr<std::shared_ptr<State>> state_holder(
                static_cast<std::shared_ptr<State>*>(parameter));
            const auto& state = *state_holder;
            state->callback(state->user_data);
        },
        task_state, TASK_PRIORITY_DEFAULT, TASK_STACK_DEPTH_DEFAULT, "VEX Python Thread");
    if (task == nullptr) {
        delete task_state;
        throw std::bad_alloc();
    }
}

Thread::~Thread() {
    stop();
}

void Thread::stop() {
    state_->stopped.store(true);
}

bool Thread::is_stopped() const {
    return state_->stopped.load();
}
