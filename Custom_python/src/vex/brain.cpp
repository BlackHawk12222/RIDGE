#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <map>
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

void Screen::print(std::string text, std::string sep, int precision) {
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

void Screen::clear_row(int row, Color color){
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

void Screen::print_at(std::string text, int x, int y, std::string sep, int precision, bool opaque){
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

int Screen::get_string_width(std::string string){
    return 1;
}

int Screen::get_string_height(std::string string){
    return 1;
}

void Screen::clear_line(int line){
    int X0=0;
    int Y0=0;
    int X1=472;
    int Y1=0;

    switch (TextSize){
        case pros::E_TEXT_SMALL:
        {
            Y1=line * 8 + 8;
            Y0=line * 8;

            break;
        }
        case pros::E_TEXT_MEDIUM:
        {
            Y1=line * 16 + 16;
            Y0=line * 16;
            break;
        }
        case pros::E_TEXT_LARGE:
        {
            Y1=line * 32 + 32;
            Y0=line * 32;
            break;
        }
    }

    pros::screen::erase_line(X0, Y0, X1, Y1);
}

void Screen::new_line(){
    ScreenY+=1;
}

void Screen::clear_screen(Color color){
    pros::screen::set_eraser(color.ColorValue);
    pros::screen::erase();
}

void Screen::set_font(Font font){
    //in progress
}

void Screen::set_pen_width(int value){
    PenWidth = value;
}

void Screen::set_pen_color(Color color){
    pros::screen::set_pen(color.ColorValue);
}

void Screen::set_fill_color(Color color){
    
}

void Screen::set_origin(int x, int y){
    
}

void Screen::draw_pixel(int x, int y){
    
}

void Screen::draw_line(int start_x, int start_y, int end_x, int end_y){
    
}

void Screen::draw_rectangle(int x, int y, int width, int height, Color color){
    
}

void Screen::draw_circle(int x, int y, int radius, Color color){
    
}

void Screen::render(){
    
}

void Screen::set_clip_region(int x, int y, int width, int height){
    
}

bool Screen::pressing(){
    return true;
}

int Screen::x_position(){
    return 1;
}

int Screen::y_position(){
    return 1;
}

template<typename Func, typename... Args>
void Screen::pressed(Func callback, Args... args){
    
}

template<typename Func, typename... Args>
void Screen::released(Func callback, Args... args){
    
}

SdCard::SdCard(){

}

void SdCard::savefile(std::string filename, char* buffer){
    
}

void SdCard::appendfile(std::string filename, char* buffer){
    
}

void SdCard::draw_image_from_file(std::string filename, int x, int y){
    
}

bool SdCard::is_inserted(){
    return true;
}

int SdCard::filesize(std::string filename){
    return 1;
}

int SdCard::size(std::string filename){
    return 1;
}

bool SdCard::exists(std::string filename){
    return true;
}

Brain::Brain(){
    Screen screen;
    SdCard sdcard;
    Battery Battery;
}