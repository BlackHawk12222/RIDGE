#ifndef VEXLINK_HPP
#define VEXLINK_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "pros/link.hpp"
#include "vex/motors.hpp"

enum VexLinkType {
    MANAGER = 100,
    WORKER = 101
};

class MessageLink {
    private:
        std::unique_ptr<pros::Link> link_;
        std::string last_message_;
        int last_integer_ = 0;
        float last_float_ = 0.0f;

    public:
        MessageLink(Ports smartport, std::string name, VexLinkType type, bool wired);

        bool is_linked();
        void send(std::string message, int integer_input, float float_input);
        char* receive(int timeout);
        int last_integer() const;
        float last_float() const;

        template<typename Callback, typename... Args>
        void received(const std::string& message, Callback&& callback, Args&&... args) {
            while (true) {
                char* received_message = receive(-1);
                if (received_message != nullptr && message == received_message) {
                    std::invoke(std::forward<Callback>(callback), std::forward<Args>(args)...);
                    return;
                }
            }
        }
};

class SerialLink {
    private:
        std::unique_ptr<pros::Link> link_;
        std::string last_message_;
        int last_integer_ = 0;
        float last_float_ = 0.0f;

    public:
        SerialLink(Ports smartport, std::string name, VexLinkType type, bool wired);

        bool is_linked();
        void send(std::string message, int integer_input, float float_input);
        char* receive(int timeout);
        int last_integer() const;
        float last_float() const;

        template<typename Callback, typename... Args>
        void received(const std::string& message, Callback&& callback, Args&&... args) {
            while (true) {
                char* received_message = receive(-1);
                if (received_message != nullptr && message == received_message) {
                    std::invoke(std::forward<Callback>(callback), std::forward<Args>(args)...);
                    return;
                }
            }
        }
};

#endif // VEXLINK_HPP
