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

extern "C" void call_robot_print(const char* text) {
    pros::screen::print(TEXT_SMALL, 1, "%s", text);
}

// 1. Separate Screen layout
class Screen {
private:
    mp_obj_base_t base; // Tied to MicroPython Screen type identity
    pros::text_format_e_t TextSize = pros::E_TEXT_MEDIUM;
    int ScreenX = 0;
    int ScreenY = 0;
    pros::Color PenColor = pros::Color::black;

    bool is_number(const std::string& str) {
        double value;
        
        // &str[0] gets the pointer to the start, + str.size() gets the end
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        
        // Success means no error (ec) AND the pointer reached the end of the string
        return ec == std::errc() && ptr == str.data() + str.size();
    }

public:
    void print(std::string text, std::string sep = "", int precision = 2) {
        if (sep == "") {
            if (is_number(text)){
                double number=std::stod(text);
                std::string output=std::format("{:.{}f}", number, precision);
                pros::screen::print(TextSize, ScreenX, ScreenY, "%s", output);
            }else{
                pros::screen::print(TextSize, ScreenX, ScreenY, "%s", text.c_str());
            }
            
        }else {
            std::vector<std::string> TextList;
            std::stringstream ss(text);
            std::string line;
            std::string output;

            if (std::getline(ss, line)) {
                output += line; // Add first line without separator
            }
            
            while (std::getline(ss, line)) {
                output += sep + line;
            }

            pros::screen::print(TextSize, ScreenX, ScreenY, "%s", output.c_str());
        }
    }

    void set_cursor(int row, int col){
        ScreenY=row;
        ScreenX=col;
        
    }

    void next_row(){
        ScreenY+=1;
    }

    void clear_row(int row=-1, Color color= Color(0x00000000)){
        if (row == -1){
            row=ScreenY;
        }
        int TextHight;
        pros::screen::set_pen(color.ColorValue);
        if (TextSize == pros::E_TEXT_SMALL){
            TextHight=8;
        }else if (TextSize == pros::E_TEXT_MEDIUM) {
            TextHight=16;
        }else if (TextSize == pros::E_TEXT_LARGE) {
            TextHight=32;
        }
        pros::screen::draw_rect(0, row*TextHight, 480, (row*TextHight) + TextHight);
        pros::screen::set_pen(PenColor);
    }

    int row(){
        return ScreenY;
    }

    int column(){
        return ScreenX;
    }

    void print_at(std::string text, int x, int y, std::string sep="", int precision=2, bool opaque=true){
        if (sep == "") {
            if (is_number(text)){
                double number=std::stod(text);
                std::string output=std::format("{:.{}f}", number, precision);
                pros::screen::print(TextSize, x, y, "%s", output);
            }else{
                pros::screen::print(TextSize, x, y, "%s", text.c_str());
            }
            
        }else {
            std::vector<std::string> TextList;
            std::stringstream ss(text);
            std::string line;
            std::string output;

            if (std::getline(ss, line)) {
                output += line; // Add first line without separator
            }
            
            while (std::getline(ss, line)) {
                output += sep + line;
            }

            pros::screen::print(TextSize, x, y, "%s", output.c_str());
        }
    }
};

// 2. Main Brain layout containing a reference to a Screen object
class Brain {
public:
    mp_obj_base_t base; // Tied to MicroPython Brain type identity
    mp_obj_t screen_subobj; // References the Screen instance inside Python
};