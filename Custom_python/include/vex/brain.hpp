#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include "main.h"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
}

struct Color{
    std::uint32_t ColorValue;
    Color(std::uint32_t value){
        ColorValue=value;
    }
};

class Battery{

};

class Screen{
    private:
        mp_obj_base_t base; // Tied to MicroPython Screen type identity
        pros::text_format_e_t TextSize = pros::E_TEXT_MEDIUM;
        int ScreenX = 0;
        int ScreenY = 0;
        pros::Color PenColor = pros::Color::black;

        bool is_number(const std::string& str);

    public:
        void print(std::string text, std::string sep = "", int precision = 2);

        void set_cursor(int row, int col);

        void next_row();

        void clear_row(int row=-1, Color color= Color(0x00000000));

        int row();

        int column();

        void print_at(std::string text, int x, int y, std::string sep="", int precision=2, bool opaque=true);
};

class SdCard{

};

class timer{

};

class Brain{
    private:


    public:
        Screen screen;
        
};
