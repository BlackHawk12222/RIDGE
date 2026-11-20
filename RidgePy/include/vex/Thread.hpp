#ifndef VEX_THREAD_HPP
#define VEX_THREAD_HPP

#include <atomic>
#include <memory>

class Thread {
public:
    using Callback = void (*)(void*);

    Thread(Callback callback, void* user_data);
    ~Thread();

    void stop();
    bool is_stopped() const;

private:
    struct State {
        State(Callback callback, void* user_data) : callback(callback), user_data(user_data) {}

        Callback callback;
        void* user_data;
        std::atomic_bool stopped{false};
    };

    std::shared_ptr<State> state_;
};

#endif
