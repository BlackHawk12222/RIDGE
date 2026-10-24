#ifndef BRAIN_HPP
#define BRAIN_HPP

#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <map>
#include "main.h"
#include "timer.hpp"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
}

struct Color{
    std::uint32_t ColorValue;
    Color(std::uint32_t value){
        ColorValue=value;
    }

    std::uint32_t RED=static_cast<int>(pros::Color::red);
    std::uint32_t GREEN=static_cast<int>(pros::Color::green);
    std::uint32_t BLUE=static_cast<int>(pros::Color::blue);
    std::uint32_t YELLOW=static_cast<int>(pros::Color::yellow);
    std::uint32_t ORANGE=static_cast<int>(pros::Color::orange);
    std::uint32_t PURPLE=static_cast<int>(pros::Color::purple);
    std::uint32_t CYAN=static_cast<int>(pros::Color::cyan);
    std::uint32_t BLACK=static_cast<int>(pros::Color::black);
    std::uint32_t WHITE=static_cast<int>(pros::Color::white);
};

struct three_wire_port{

};

enum VoltageUnits{
    VOLT=39,

};

enum CurrentUnits{
    CURRENT_AMPS=33,
};


enum TemperatureUnits{
    CELSIUS=35,
    FAHRENHEIT=36,
    TEMPERATURE_PERCENT=23
};

enum Font{
    MONO12=40,
    MONO15=41,
    MONO20=42,
    MONO30=43,
    MONO40=44,
    MONO60=45,
    PROP20=46,
    PROP30=47,
    PROP40=48,
    PROP60=49,
};

class Battery{

    public:
        float voltage(VoltageUnits units);

        float current(CurrentUnits units);

        int capacity();

        int temperature(TemperatureUnits);

};

class Screen{
    private:
        mp_obj_base_t base; // Tied to MicroPython Screen type identity
        pros::text_format_e_t TextSize = pros::E_TEXT_MEDIUM;
        int ScreenX = 0;
        int ScreenY = 0;
        int PenWidth = 0;
        pros::Color PenColor = pros::Color::black;

        bool is_number(const std::string& str);

    public:
        void print(std::string text, std::string sep = "", int precision = 2);

        void set_cursor(int row, int col);

        void next_row();

        void clear_row(int row=-1, Color color= Color(0)); // Default color is BLACK.

        int row();

        int column();

        void print_at(std::string text, int x, int y, std::string sep="", int precision=2, bool opaque=true);

        int get_string_width(std::string string);

        int get_string_height(std::string string);

        void clear_line(int line);

        void new_line();

        void clear_screen(Color color= Color(0)); // Default color is BLACK.

        void set_font(Font font);

        void set_pen_width(int value);

        void set_pen_color(Color color);

        void set_fill_color(Color color);

        void set_origin(int x, int y);

        void draw_pixel(int x, int y);

        void draw_line(int start_x, int start_y, int end_x, int end_y);

        void draw_rectangle(int x, int y, int width, int height, Color color);

        void draw_circle(int x, int y, int radius, Color color);

        void render();

        void set_clip_region(int x, int y, int width, int height);

        bool pressing();

        int x_position();

        int y_position();

        template<typename Func, typename... Args>
        void pressed(Func callback, Args... args);

        template<typename Func, typename... Args>
        void released(Func callback, Args... args);
};

class SdCard{
    private:

    public:
        SdCard();

        char* loadfile(std::string filename, char* buffer = new char[0]);

        void savefile(std::string filename, char* buffer = new char[0]);

        void appendfile(std::string filename, char* buffer = new char[0]);

        void draw_image_from_file(std::string filename, int x, int y);

        bool is_inserted();

        int filesize(std::string filename);

        int size(std::string filename);

        bool exists(std::string filename);

};

class Brain{
    private:
        

    public:
        Brain();
};

#endif //BRAIN_HPP