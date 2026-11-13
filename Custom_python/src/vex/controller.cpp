#include <format>
#include "vex/safe_value.hpp"
#include <sstream>
#include <charconv>
#include <algorithm>
#include "main.h"
#include "vex/controller.hpp"

ControllerScreen::ControllerScreen(pros::Controller* controller){
    this -> ControllerObj = controller;
    for (int i = 0; i < ROWS; ++i) {
        Wanted[i] = std::string(COLS, ' ');
        Shown[i] = std::string();   // unknown: force the first send
    }
    Flusher = new pros::Task([this]() { flush_loop(); },
                             TASK_PRIORITY_DEFAULT, TASK_STACK_DEPTH_DEFAULT, "VEX Ctrl Screen");
}

void ControllerScreen::flush_loop(){
    int next = 0;
    while (true) {
        // Find one changed row, starting after the last one sent so rows take turns.
        int row = -1;
        std::string text;
        Lock.take();
        for (int i = 0; i < ROWS; ++i) {
            const int r = (next + i) % ROWS;
            if (Wanted[r] != Shown[r]) {
                row = r;
                text = Wanted[r];
                break;
            }
        }
        Lock.give();

        if (row >= 0 && ControllerObj->is_connected()) {
            // set_text with a full-width row overwrites old text: no separate clear needed.
            if (ControllerObj->set_text(row, 0, text.c_str()) == 1) {
                Shown[row] = text;
            }
            next = row + 1;
        } else if (!ControllerObj->is_connected()) {
            for (auto& s : Shown) s.clear();   // resend everything after a reconnect
        }
        pros::delay(55);   // the controller accepts ~1 update per 50 ms
    }
}

bool ControllerScreen::is_number(const std::string& str) {
    double value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    return ec == std::errc() && ptr == str.data() + str.size();
}

void ControllerScreen::write(int row, int column, const std::string& text){
    if (row < 1 || row > ROWS || column < 1 || column > COLS) return;
    Lock.take();
    std::string& line = Wanted[row - 1];
    const size_t start = static_cast<size_t>(column - 1);
    const size_t n = std::min(text.size(), static_cast<size_t>(COLS) - start);
    line.replace(start, n, text, 0, n);
    Lock.give();
}

void ControllerScreen::print(std::string value, std::string sep, int precision){
    std::string output;
    if (sep.empty()) {
        if (is_number(value)) {
            output = std::format("{:.{}f}", std::stod(value), precision);
        } else {
            output = value;
        }
    } else {
        std::stringstream ss(value);
        std::string line;
        if (std::getline(ss, line)) {
            output += line;
        }
        while (std::getline(ss, line)) {
            output += sep + line;
        }
    }
    write(ScreenRow, ScreenColumn, output);
    ScreenColumn += static_cast<int>(output.size());   // VEXcode advances the cursor
}

void ControllerScreen::set_cursor(int row, int column){
    ScreenColumn = column;
    ScreenRow = row;
}

int ControllerScreen::column(){
    return ScreenColumn;
}

int ControllerScreen::row(){
    return ScreenRow;
}

void ControllerScreen::clear_screen(){
    Lock.take();
    for (auto& line : Wanted) line.assign(COLS, ' ');
    Lock.give();
    ScreenRow = 1;
    ScreenColumn = 1;
}

void ControllerScreen::clear_row(int row){
    if (row < 1 || row > ROWS) return;
    Lock.take();
    Wanted[row - 1].assign(COLS, ' ');
    Lock.give();
}

void ControllerScreen::next_row(){
    ScreenRow += 1;
    ScreenColumn = 1;
}

ControllerButton::ControllerButton(pros::controller_digital_e_t Button, pros::Controller* controller){
    ControllerObj = controller;
    ButtonType = Button;
}

bool ControllerButton::pressing(){
    return vsafe::num(ControllerObj -> get_digital(ButtonType));
}

ControllerAxis::ControllerAxis(pros::controller_analog_e_t Axis, pros::Controller* controller){
    ControllerObj = controller;
    AxisType = Axis;
}

int ControllerAxis::position(){
    return vsafe::num(ControllerObj -> get_analog(AxisType));
}

Controller::Controller(ControllerType controller_type){
    pros::controller_id_e_t Type = pros::E_CONTROLLER_MASTER;
    switch (controller_type){
        case MASTER:
        {
            break;
        }
        case PARTNER:
        {
            Type = pros::E_CONTROLLER_PARTNER;
            break;
        }
        default:
        {
            break;
        }
    }

    this -> ControllerObj = new pros::Controller(Type);

    this -> screen = new ControllerScreen(ControllerObj);

    this -> axis1 = new ControllerAxis(pros::E_CONTROLLER_ANALOG_RIGHT_X, ControllerObj);
    this -> axis2 = new ControllerAxis(pros::E_CONTROLLER_ANALOG_RIGHT_Y, ControllerObj);
    this -> axis3 = new ControllerAxis(pros::E_CONTROLLER_ANALOG_LEFT_Y, ControllerObj);
    this -> axis4 = new ControllerAxis(pros::E_CONTROLLER_ANALOG_LEFT_X, ControllerObj);

    this -> buttonA = new ControllerButton(pros::E_CONTROLLER_DIGITAL_A, ControllerObj);
    this -> buttonB = new ControllerButton(pros::E_CONTROLLER_DIGITAL_B, ControllerObj);
    this -> buttonX = new ControllerButton(pros::E_CONTROLLER_DIGITAL_X, ControllerObj);
    this -> buttonY = new ControllerButton(pros::E_CONTROLLER_DIGITAL_Y, ControllerObj);
    this -> buttonDown = new ControllerButton(pros::E_CONTROLLER_DIGITAL_DOWN, ControllerObj);
    this -> buttonUp = new ControllerButton(pros::E_CONTROLLER_DIGITAL_UP, ControllerObj);
    this -> buttonLeft = new ControllerButton(pros::E_CONTROLLER_DIGITAL_LEFT, ControllerObj);
    this -> buttonRight = new ControllerButton(pros::E_CONTROLLER_DIGITAL_RIGHT, ControllerObj);
    this -> buttonR1 = new ControllerButton(pros::E_CONTROLLER_DIGITAL_R1, ControllerObj);
    this -> buttonR2 = new ControllerButton(pros::E_CONTROLLER_DIGITAL_R2, ControllerObj);
    this -> buttonL1 = new ControllerButton(pros::E_CONTROLLER_DIGITAL_L1, ControllerObj);
    this -> buttonL2 = new ControllerButton(pros::E_CONTROLLER_DIGITAL_L2, ControllerObj);
}

void Controller::rumble(std::string pattern){
    ControllerObj -> rumble(pattern.c_str());
}