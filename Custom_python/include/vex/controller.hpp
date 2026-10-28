#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "main.h"

enum ControllerType{
    MASTER=50,
    PARTNER=51
};

class ControllerScreen{
    private:
        Controller* ControllerObj = nullptr;

        int ScreenRow = 0;
        int ScreenColumn = 0;
        
    public:
        ControllerScreen(Controller* controller);

        void print(std::string value, std::string sep, int precision);

        void set_cursor(int row, int column);

        int column();

        int row();

        void clear_screen();

        void clear_row(int row);

        void next_row();
};

class ControllerButton{
    private:
        Controller* ControllerObj = nullptr;
        bool OldPos = false;

    public:
        ControllerButton(pros::controller_digital_e_t Button, Controller* controller);

        bool pressing();

        void pressed();

        void released();
};

class ControllerAxis{
    private:
        Controller* ControllerObj = nullptr;
        int OldPos = 0;

    public:
        ControllerAxis(pros::controller_analog_e_t Axis, Controller* controller);

        int position();

        void changed();
};

class Controller{
    private:
        pros::Controller* ControllerObj = nullptr;

    public:
        Controller(ControllerType controller_type);

        void rumble(std::string pattern);

        bool remote_control_code_enabled();
};

#endif // CONTROLLER_HPP