#include "main.h"
#include "vex/controller.hpp"

ControllerScreen::ControllerScreen(pros::Controller* controller){
    this -> ControllerObj = controller;
}

bool ControllerScreen::is_number(const std::string& str) {
    double value;
    
    // &str[0] gets the pointer to the start, + str.size() gets the end
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    
    // Success means no error (ec) AND the pointer reached the end of the string
    return ec == std::errc() && ptr == str.data() + str.size();
}

void ControllerScreen::print(std::string value, std::string sep, int precision){
    if (sep == "") {
        if (is_number(value)){
            double number=std::stod(value);
            std::string output=std::format("{:.{}f}", number, precision);

            ControllerObj->print(ScreenRow, ScreenColumn, "%s", output);
        }else{
            ControllerObj->print(ScreenRow, ScreenColumn, "%s", value.c_str());
        }
        
    }else {
        std::vector<std::string> TextList;
        std::stringstream ss(value);
        std::string line;
        std::string output;

        if (std::getline(ss, line)) {
            output += line; // Add first line without separator
        }
        
        while (std::getline(ss, line)) {
            output += sep + line;
        }

        ControllerObj->print(ScreenRow, ScreenColumn, "%s", output.c_str());
    }
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
    ControllerObj -> clear();
}

void ControllerScreen::clear_row(int row){
    ControllerObj -> clear_line(row);
}

void ControllerScreen::next_row(){
    ScreenRow+=1;
}

ControllerButton::ControllerButton(pros::controller_digital_e_t Button, pros::Controller* controller){
    ControllerObj = controller;
    ButtonType = Button;
}

bool ControllerButton::pressing(){
    return ControllerObj -> get_digital(ButtonType);
}

ControllerAxis::ControllerAxis(pros::controller_analog_e_t Axis, pros::Controller* controller){
    ControllerObj = controller;
    AxisType = Axis;
}

int ControllerAxis::position(){
    return ControllerObj -> get_analog(AxisType);
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