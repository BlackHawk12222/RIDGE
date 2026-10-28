#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "main.h"

enum ControllerType{
    MASTER=50,
    PARTNER=51
};

class ControllerScreen{
    private:
        pros::Controller* ControllerObj = nullptr;

        int ScreenRow = 0;
        int ScreenColumn = 0;
        
        bool is_number(const std::string& str);

    public:
        ControllerScreen(pros::Controller* controller);

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
        pros::Controller* ControllerObj = nullptr;
        pros::controller_digital_e_t ButtonType;
        bool OldPos = false;

    public:
        ControllerButton(pros::controller_digital_e_t Button, pros::Controller* controller);

        bool pressing();

        template<typename Func, typename...Args>
        void pressed(Func func, Args... args);

        template<typename Func, typename...Args>
        void released(Func func, Args... args);
};

class ControllerAxis{
    private:
        pros::Controller* ControllerObj = nullptr;
        pros::controller_analog_e_t AxisType;
        int OldPos = 0;

    public:
        ControllerAxis(pros::controller_analog_e_t Axis, pros::Controller* controller);

        int position();

        template<typename Func, typename...Args>
        void changed(Func func, Args... args);
};

class Controller{
    private:
        pros::Controller* ControllerObj = nullptr;

    public:
        Controller(ControllerType controller_type);

        void rumble(std::string pattern);
};

#endif // CONTROLLER_HPP