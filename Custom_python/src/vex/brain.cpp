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
            if (Render){
                BufferLen++;
                YBuffer.push_back(ScreenY);
                XBuffer.push_back(ScreenX);
                StringBuffer.push_back(output);
                return;
            }
            pros::screen::print(TextSize, ScreenX, ScreenY, "%s", output);
        }else{
            if (Render){
                BufferLen++;
                YBuffer.push_back(ScreenY);
                XBuffer.push_back(ScreenX);
                StringBuffer.push_back(text);
                return;
            }
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

        if (Render){
            BufferLen++;
            YBuffer.push_back(ScreenY);
            XBuffer.push_back(ScreenX);
            StringBuffer.push_back(output);
            return;
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
    int output = 0;
    switch (font)
    {
    case MONO12:
    {
        output = 10;
        break;
    }
    case MONO15:
    {
        output = 12;
        break;
    }
    case MONO20:
    {
        output = 15;
        break;
    }
    case MONO30:
    {
        output = 22;
        break;
    }
    case MONO40:
    {
        output = 30;
        break;
    }
    case MONO60:
    {
        output = 44;
        break;
    }
    default:
        break;
    }

    /*Due to compexity the PROP fonts cant be calcuated.*/

    output *= string.size();
    
    return output;
}

int Screen::get_string_height(std::string string){
    int output = 0;
    switch(font){
        case MONO12:
        {
            output = 12;
            break;

        }
        case MONO15:
        {
            output = 15;
            break;
        }
        case MONO20:
        {
            output = 20;
            break;
        }
        case MONO30:
        {
            output = 30;
            break;
        }
        case MONO40:
        {
            output = 40;
            break;
        }
        case MONO60:
        {
            output = 60;
            break;
        }
        case PROP20:
        {
            output = 20;
            break;
        }
        case PROP30:
        {
            output = 30;
            break;
        }
        case PROP40:
        {
            output = 40;
            break;
        }
        case PROP60:
        {
            output = 60;
            break;
        }
        default:
        {
            break;
        }

    }
    return output;
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
    // in progress
}

void Screen::set_origin(int x, int y){
    XOffset = x;
    YOffset = y;
}

void Screen::draw_pixel(int x, int y){
    pros::screen::draw_pixel(x,y);
}

void Screen::draw_line(int start_x, int start_y, int end_x, int end_y){
    pros::screen::draw_line(start_x, start_y, end_x, end_y);
}

void Screen::draw_rectangle(int x, int y, int width, int height, Color color){
    int X1 = x + width;
    int Y1 = y + height;

    if (color.ColorValue != (int)PenColor){
        pros::screen::set_pen(color.ColorValue);
    }

    pros::screen::draw_rect(x, y, X1, Y1);

    pros::screen::set_pen(PenColor);
}

void Screen::draw_circle(int x, int y, int radius, Color color){
    if (color.ColorValue != (int)PenColor){
        pros::screen::set_pen(color.ColorValue);
    }

    pros::screen::draw_circle(x, y, radius);

    pros::screen::set_pen(PenColor);
}

void Screen::render(){
    if (Render == true){
        for (int i = 0; BufferLen <= i; i++){
            std::string text = StringBuffer[i];
            int x = XBuffer[i];
            int y = YBuffer[i];

            pros::screen::print(TextSize, x, y, "%s", text.c_str());
        }
    }
    Render = true;
}

void Screen::set_clip_region(int x, int y, int width, int height){
    // in progress
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