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
        void pressed(Func func, std::tuple<Args...> args){
            auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            
            // Wrap it inside a pros::Task lambda. 
            pros::Task([this, bound_callable]() mutable {
                while (1){
                    if (this -> ControllerObj -> get_digital_new_press(this -> ButtonType)){
                        std::apply(callback, args);
                    }

                    pros::delay(50);
                }
            });
        }

        template<typename Func, typename...Args>
        void released(Func func, std::tuple<Args...> args){
            auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            
            // Wrap it inside a pros::Task lambda. 
            pros::Task([this, bound_callable]() mutable {
                while (1){
                    if (this -> ControllerObj -> get_digital_new_release(this -> ButtonType)){
                        std::apply(callback, args);
                    }

                    pros::delay(50);
                }
            });
        }
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
        void changed(Func func, std::tuple<Args...> args){
            auto bound_callable = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
            
            // Wrap it inside a pros::Task lambda. 
            pros::Task([this, bound_callable]() mutable {
                while (1){
                    if (this -> ControllerObj -> get_analog(this -> AxisType) != this -> OldPos){
                        std::apply(callback, args);
                    }

                    pros::delay(50);
                }
            });
        }
};

class Controller{
    private:
        pros::Controller* ControllerObj = nullptr;

        

    public:
        ControllerScreen* screen;

        ControllerAxis* axis1 = nullptr;
        ControllerAxis* axis2 = nullptr;
        ControllerAxis* axis3 = nullptr;
        ControllerAxis* axis4 = nullptr;

        ControllerButton* buttonA = nullptr;
        ControllerButton* buttonB = nullptr;
        ControllerButton* buttonX = nullptr;
        ControllerButton* buttonY = nullptr;
        ControllerButton* buttonDown = nullptr;
        ControllerButton* buttonUp = nullptr;
        ControllerButton* buttonLeft = nullptr;
        ControllerButton* buttonRight = nullptr;
        ControllerButton* buttonR1 = nullptr;
        ControllerButton* buttonR2 = nullptr;
        ControllerButton* buttonL1 = nullptr;
        ControllerButton* buttonL2 = nullptr;

        Controller(ControllerType controller_type);

        void rumble(std::string pattern);
};

#endif // CONTROLLER_HPP