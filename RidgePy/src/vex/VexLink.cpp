#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "vex/VexLink.hpp"

namespace {
constexpr std::size_t MESSAGE_CAPACITY = 240;

struct LinkFrame {
    std::uint16_t message_size;
    char message[MESSAGE_CAPACITY];
    std::uint16_t reserved;
    std::int32_t integer;
    float decimal;
};
static_assert(sizeof(LinkFrame) == 252, "VexLink frame must have a stable wire layout");

pros::link_type_e_t to_pros_type(VexLinkType type) {
    switch (type) {
        case MANAGER:
            return pros::E_LINK_TX;
        case WORKER:
            return pros::E_LINK_RX;
        default:
            throw std::invalid_argument("VexLink type must be MANAGER or WORKER");
    }
}

void send_frame(pros::Link& link, LinkFrame& frame, bool raw) {
    const std::uint32_t sent = raw
        ? link.transmit_raw(&frame, sizeof(frame))
        : link.transmit(&frame, sizeof(frame));
    if (sent != sizeof(frame)) {
        throw std::runtime_error("VexLink message could not be transmitted");
    }
}

bool receive_frame(pros::Link& link, LinkFrame& frame, int timeout, bool raw) {
    const std::uint32_t started = pros::millis();
    while (true) {
        if (link.connected() && link.raw_receivable_size() >= sizeof(frame)) {
            const std::uint32_t received = raw
                ? link.receive_raw(&frame, sizeof(frame))
                : link.receive(&frame, sizeof(frame));
            if (received != sizeof(frame)) {
                throw std::runtime_error("VexLink message could not be received");
            }
            if (frame.message_size > MESSAGE_CAPACITY) {
                throw std::runtime_error("VexLink message has an invalid length");
            }
            return true;
        }

        if (timeout >= 0 && static_cast<std::uint32_t>(pros::millis() - started) >=
                                static_cast<std::uint32_t>(timeout)) {
            return false;
        }
        pros::delay(1);
    }
}

void fill_frame(LinkFrame& frame, const std::string& message, int integer_input, float float_input) {
    if (message.size() > MESSAGE_CAPACITY) {
        throw std::length_error("VexLink messages may not exceed 240 bytes");
    }
    frame = {};
    frame.message_size = static_cast<std::uint16_t>(message.size());
    std::memcpy(frame.message, message.data(), message.size());
    frame.integer = integer_input;
    frame.decimal = float_input;
}

void set_received_message(std::string& message, int& integer, float& decimal, const LinkFrame& frame) {
    message.assign(frame.message, frame.message_size);
    integer = frame.integer;
    decimal = frame.decimal;
}

std::unique_ptr<pros::Link> make_link(Ports port, const std::string& name, VexLinkType type, bool wired) {
    return std::make_unique<pros::Link>(
        static_cast<std::uint8_t>(port), name, to_pros_type(type), wired);
}
}

MessageLink::MessageLink(Ports smartport, std::string name, VexLinkType type, bool wired)
    : link_(make_link(smartport, name, type, wired)) {}

bool MessageLink::is_linked() {
    return link_->connected();
}

void MessageLink::send(std::string message, int integer_input, float float_input) {
    LinkFrame frame{};
    fill_frame(frame, message, integer_input, float_input);
    send_frame(*link_, frame, false);
}

char* MessageLink::receive(int timeout) {
    LinkFrame frame{};
    if (!receive_frame(*link_, frame, timeout, false)) {
        return nullptr;
    }
    set_received_message(last_message_, last_integer_, last_float_, frame);
    return last_message_.data();
}

int MessageLink::last_integer() const {
    return last_integer_;
}

float MessageLink::last_float() const {
    return last_float_;
}

SerialLink::SerialLink(Ports smartport, std::string name, VexLinkType type, bool wired)
    : link_(make_link(smartport, name, type, wired)) {}

bool SerialLink::is_linked() {
    return link_->connected();
}

void SerialLink::send(std::string message, int integer_input, float float_input) {
    LinkFrame frame{};
    fill_frame(frame, message, integer_input, float_input);
    send_frame(*link_, frame, true);
}

char* SerialLink::receive(int timeout) {
    LinkFrame frame{};
    if (!receive_frame(*link_, frame, timeout, true)) {
        return nullptr;
    }
    set_received_message(last_message_, last_integer_, last_float_, frame);
    return last_message_.data();
}

int SerialLink::last_integer() const {
    return last_integer_;
}

float SerialLink::last_float() const {
    return last_float_;
}
