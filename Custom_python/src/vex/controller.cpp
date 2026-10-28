#include "main.h"
#include "vex/controller.hpp"

ControllerScreen::ControllerScreen(pros::Controller* controller){
    ControllerObj = controller;
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

template<typename Func, typename...Args>
void ControllerButton::pressed(Func func, Args... args){
    auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    
    // Wrap it inside a pros::Task lambda. 
    // Capturing by value [=] ensures args remain valid if local variables fall out of scope.
    pros::Task([this, bound_callable]() mutable {
        while (1){
            if (this -> ControllerObj -> get_digital_new_press(this -> ButtonType)){
                bound_callable();
            }

            pros::delay(50);
        }
    });
}

template<typename Func, typename...Args>
void ControllerButton::released(Func func, Args... args){
    auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    
    // Wrap it inside a pros::Task lambda. 
    // Capturing by value [=] ensures args remain valid if local variables fall out of scope.
    pros::Task([this, bound_callable]() mutable {
        while (1){
            if (this -> ControllerObj -> get_digital_new_release(this -> ButtonType)){
                bound_callable();
            }

            pros::delay(50);
        }
    });
}

ControllerAxis::ControllerAxis(pros::controller_analog_e_t Axis, pros::Controller* controller){
    ControllerObj = controller;
    AxisType = Axis;
}

int ControllerAxis::position(){
    return ControllerObj -> get_analog(AxisType);
}

template<typename Func, typename...Args>
void ControllerAxis::changed(Func func, Args... args){
    auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    
    // Wrap it inside a pros::Task lambda. 
    // Capturing by value [=] ensures args remain valid if local variables fall out of scope.
    pros::Task([this, bound_callable]() mutable {
        while (1){
            if (this -> ControllerObj -> get_analog(this -> AxisType) != this -> OldPos){
                bound_callable();
            }

            pros::delay(50);
        }
    });
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

    pros::Controller controller(Type);

    ControllerObj = &controller;

    ControllerScreen screen(ControllerObj);

    ControllerAxis axis1(pros::E_CONTROLLER_ANALOG_RIGHT_X, ControllerObj);
    ControllerAxis axis2(pros::E_CONTROLLER_ANALOG_RIGHT_Y, ControllerObj);
    ControllerAxis axis3(pros::E_CONTROLLER_ANALOG_LEFT_Y, ControllerObj);
    ControllerAxis axis4(pros::E_CONTROLLER_ANALOG_LEFT_X, ControllerObj);

    ControllerButton buttonA(pros::E_CONTROLLER_DIGITAL_A, ControllerObj);
    ControllerButton buttonB(pros::E_CONTROLLER_DIGITAL_B, ControllerObj);
    ControllerButton buttonX(pros::E_CONTROLLER_DIGITAL_X, ControllerObj);
    ControllerButton buttonY(pros::E_CONTROLLER_DIGITAL_Y, ControllerObj);
    ControllerButton buttonDown(pros::E_CONTROLLER_DIGITAL_DOWN, ControllerObj);
    ControllerButton buttonUp(pros::E_CONTROLLER_DIGITAL_UP, ControllerObj);
    ControllerButton buttonLeft(pros::E_CONTROLLER_DIGITAL_LEFT, ControllerObj);
    ControllerButton buttonRight(pros::E_CONTROLLER_DIGITAL_RIGHT, ControllerObj);
    ControllerButton buttonR1(pros::E_CONTROLLER_DIGITAL_R1, ControllerObj);
    ControllerButton buttonR2(pros::E_CONTROLLER_DIGITAL_R2, ControllerObj);
    ControllerButton buttonL1(pros::E_CONTROLLER_DIGITAL_L1, ControllerObj);
    ControllerButton buttonL2(pros::E_CONTROLLER_DIGITAL_L2, ControllerObj);
}

void Controller::rumble(std::string pattern){
    ControllerObj -> rumble(pattern.c_str());
}