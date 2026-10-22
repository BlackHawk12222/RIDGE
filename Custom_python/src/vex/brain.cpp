#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include "main.h"
#include "vex/brain.hpp"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
}

    bool Screen::is_number(const std::string& str) {
        double value;
        
        // &str[0] gets the pointer to the start, + str.size() gets the end
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
        
        // Success means no error (ec) AND the pointer reached the end of the string
        return ec == std::errc() && ptr == str.data() + str.size();
    }

    void Screen::print(std::string text, std::string sep = "", int precision = 2) {
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

    void Screen::set_cursor(int row, int col){
        ScreenY=row;
        ScreenX=col;
        
    }

    void Screen::next_row(){
        ScreenY+=1;
    }

    void Screen::clear_row(int row=-1, Color color= Color(0x00000000)){
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

    int Screen::row(){
        return ScreenY;
    }

    int Screen::column(){
        return ScreenX;
    }

    void Screen::print_at(std::string text, int x, int y, std::string sep="", int precision=2, bool opaque=true){
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