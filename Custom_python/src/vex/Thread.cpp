#include "vex/Thread.hpp"

#include "pros/rtos.hpp"

#include <memory>

Thread::Thread(Callback callback, void* user_data)
    : state_(std::make_shared<State>(callback, user_data)) {
    auto* task_state = new std::shared_ptr<State>(state_);
    const auto task = pros::c::task_create(
        [](void* parameter) {
            {
                std::unique_ptr<std::shared_ptr<State>> state_holder(
                    static_cast<std::shared_ptr<State>*>(parameter));
                const auto& state = *state_holder;
                if (state && state->callback) {
                    state->callback(state->user_data);
                }
            } // state_holder and shared_ptr cleanly destroyed here
            pros::c::task_delete(nullptr);
        },
        task_state, TASK_PRIORITY_DEFAULT, 0x2000, "VEX Python Thread");

    if (task == nullptr) {
        delete task_state;
    }
}

Thread::~Thread() {
    stop();
}

void Thread::stop() {
    if (state_) {
        state_->stopped.store(true);
    }
}

bool Thread::is_stopped() const {
    return state_ ? state_->stopped.load() : true;
}