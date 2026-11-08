#ifndef BRAIN_HPP
#define BRAIN_HPP

#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <map>
#include "main.h"
#include "vex/font_init.h"
#include "timer.hpp"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
typedef void (*c_callback_t)(void* user_data);
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

struct ThreeWirePort{
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int f = 6;
    int g = 7;
    int h = 8;
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

        int temperature(TemperatureUnits units);

};

class Screen{
    private:
        pros::screen_touch_status_s_t status = pros::screen::touch_status();
        Font font = MONO20;
        int ScreenX = 0;
        int ScreenY = 0;
        int XPos = 0;
        int YPos = 0;
        int YOffset = 0;
        int XOffset = 0;
        int PenWidth = 0;
        pros::Color PenColor = pros::Color::white;
        uint32_t BackgroundColor = 0;
        bool Render = false;

        std::vector<int> YBuffer;
        std::vector<int> XBuffer;
        std::vector<std::string> StringBuffer;
        std::vector<bool> OpaqueBuffer;
        std::map<std::pair<int, int>, lv_obj_t*> TextLabels;

        static inline std::function<void()> current_callback_pressed = nullptr;

        static void pros_callback_bridge_pressed() {
            if (current_callback_pressed) {
                current_callback_pressed(); 
            }
        }

        static inline std::function<void()> current_callback_released = nullptr;

        static void pros_callback_bridge_released() {
            if (current_callback_released) {
                current_callback_released(); 
            }
        }

        bool is_number(const std::string& str);
        const lv_font_t* current_font() const;
        int font_height() const;
        int character_width() const;
        std::string format_text(const std::string& text, const std::string& sep, int precision);
        void draw_text(const std::string& text, int x, int y, bool opaque);
        void delete_text_labels(int x, int y, int width, int height);

    public:
        void print(std::string text, std::string sep = "", int precision = 2);

        void set_cursor(int row, int col);

        void next_row();

        void clear_row(int row=-1, Color color= Color(0)); // Default color is BLACK.

        int row();

        int column();

        void print_at(std::string text, int x, int y, std::string sep=" ", int precision=2, bool opaque=true);

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
        void pressed(Func callback, Args... args) {
            // Bind the function and its arguments into a single parameterless lambda
            current_callback_pressed = [=]() {
                std::invoke(callback, args...); 
            };

            // Pass the static bridge function pointer to PROS
            pros::screen::touch_callback(pros_callback_bridge_pressed, pros::E_TOUCH_PRESSED);
        }

        template<typename Func, typename... Args>
        void released(Func callback, Args... args){

            // Bind the function and its arguments into a single parameterless lambda
            current_callback_released = [=]() {
                std::invoke(callback, args...); 
            };

            // Pass the static bridge function pointer to PROS
            pros::screen::touch_callback(pros_callback_bridge_released, pros::E_TOUCH_RELEASED);
        }
};

class SdCard{
    private:
        // --- Private Helper: Safe Bare-Metal BMP Decoder ---
        void draw_bmp(std::ifstream& file, int x_offset, int y_offset);
        
        // --- Private Helper: uPNG ARM-safe Decoder ---
        void draw_png(const std::string& filename, int x_offset, int y_offset);
    public:

        char* loadfile(std::string filename, char* buffer, size_t buffer_size, size_t* result_size);

        void savefile(std::string filename, const char* buffer = nullptr, size_t buffer_size = 0);

        void appendfile(std::string filename, const char* buffer, size_t buffer_size);

        void draw_image_from_file(std::string filename, int x, int y);

        bool is_inserted();

        int filesize(std::string filename);

        int size(std::string filename);

        bool exists(std::string filename);

};

class Brain{
    public:
        ThreeWirePort three_wire_port;
        Screen screen;
        SdCard sdcard;
        Battery battery;
};

#endif //BRAIN_HPP