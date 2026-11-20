#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <tuple>
#include <utility>
#include <array>
#include <string>

#include "main.h"

enum ControllerType{
    MASTER=50,
    PARTNER=51
};

// The V5 controller screen is 3 rows x 19 columns and its radio link only takes
// about one text update every 50 ms. Writing faster floods the link (lag, missed
// text, dropped connection). So print/clear only change a local copy of the
// screen, and one background task sends a changed row at most every 55 ms.
// Rows and columns are 1-based, like VEXcode.
class ControllerScreen{
    public:
        static constexpr int ROWS = 3;
        static constexpr int COLS = 19;

    private:
        pros::Controller* ControllerObj = nullptr;

        int ScreenRow = 1;
        int ScreenColumn = 1;

        pros::Mutex Lock;                          // guards Wanted
        std::array<std::string, ROWS> Wanted;      // what the screen should show
        std::array<std::string, ROWS> Shown;       // what we last sent (flush task only)
        pros::Task* Flusher = nullptr;

        bool is_number(const std::string& str);
        void write(int row, int column, const std::string& text);
        void flush_loop();

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

    public:
        ControllerButton(pros::controller_digital_e_t Button, pros::Controller* controller);

        bool pressing();

        template<typename Func, typename...Args>
        void pressed(Func func, Args... args){
            auto callback = [func = std::forward<Func>(func),
                             callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const bool initial_pos = ControllerObj->get_digital(ButtonType);

            pros::Task([this, previous_pos = initial_pos, callback = std::move(callback)]() mutable {
                bool previous = previous_pos;
                while (true){
                    const bool current = ControllerObj->get_digital(ButtonType);
                    if (current && !previous){
                        callback();
                    }
                    previous = current;
                    pros::delay(50);
                }
            });
        }

        template<typename Func, typename...Args>
        void released(Func func, Args... args){
            auto callback = [func = std::forward<Func>(func),
                             callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const bool initial_pos = ControllerObj->get_digital(ButtonType);

            pros::Task([this, previous_pos = initial_pos, callback = std::move(callback)]() mutable {
                bool previous = previous_pos;
                while (true){
                    const bool current = ControllerObj->get_digital(ButtonType);
                    if (!current && previous){
                        callback();
                    }
                    previous = current;
                    pros::delay(50);
                }
            });
        }
};

class ControllerAxis{
    private:
        pros::Controller* ControllerObj = nullptr;
        pros::controller_analog_e_t AxisType;

    public:
        ControllerAxis(pros::controller_analog_e_t Axis, pros::Controller* controller);

        int position();

        template<typename Func, typename...Args>
        void changed(Func func, Args... args){
            auto callback = [func = std::forward<Func>(func),
                             callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const int initial_pos = ControllerObj->get_analog(AxisType);

            pros::Task([this, previous_pos = initial_pos, callback = std::move(callback)]() mutable {
                int previous = previous_pos;
                while (true){
                    const int current = ControllerObj->get_analog(AxisType);
                    if (current != previous){
                        callback();
                    }
                    previous = current;
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