#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <map>
#include <functional>
#include <cstdio>
#include "main.h"
#include "vex/brain.hpp"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
}

float Battery::voltage(VoltageUnits units){
    float output = 0.0f;
    switch (units)
    {
    case VOLT:
        output = pros::battery::get_voltage() / 1000;
        break;
    
    default:
        break;
    }

    return output;
}

float Battery::current(CurrentUnits units){
    float output = 0.0f;

    switch (units){
        case CURRENT_AMPS:
        {
            output = pros::battery::get_current() / 1000;
            break;
        }
        default:
        {
            break;
        }
    }

    return output;
}

int Battery::capacity(){
    return (int)pros::battery::get_capacity();
}

int Battery::temperature(TemperatureUnits units){
    int output = 2;

    switch (units)
    {
    case TEMPERATURE_PERCENT:
        output = ((pros::battery::get_temperature() - 20) / (70 - 20)) * 100.0;
        
        // Constrain boundaries between 0% and 100%
        if (output < 0.0) output = 0.0;
        if (output > 100.0) output = 100.0;
        break;
    case CELSIUS:
    {
        output = (int)pros::battery::get_temperature();
        break;
    }
    case FAHRENHEIT:
    {
        output = (int)((pros::battery::get_temperature() * 9.0 / 5.0) + 32.0);
        break;
    }
    default:
        break;
    }

    return output;
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
            if (Render){
                BufferLen++;
                YBuffer.push_back(ScreenY);
                XBuffer.push_back(ScreenX);
                StringBuffer.push_back(output);
                return;
            }
            pros::screen::print(TextSize, x, y, "%s", output);
        }else{
            if (Render){
                BufferLen++;
                YBuffer.push_back(ScreenY);
                XBuffer.push_back(ScreenX);
                StringBuffer.push_back(text);
                return;
            }
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

        if (Render){
            BufferLen++;
            YBuffer.push_back(ScreenY);
            XBuffer.push_back(ScreenX);
            StringBuffer.push_back(output);
            return;
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

        XBuffer = {};
        YBuffer = {};
        StringBuffer = {};
        BufferLen = 0;
    }
    Render = true;
}

void Screen::set_clip_region(int x, int y, int width, int height){
    // in progress
}

bool Screen::pressing(){
    return status.touch_status;
}

int Screen::x_position(){
    return status.x;
}

int Screen::y_position(){
    return status.y;
}

template<typename Func, typename... Args>
void Screen::pressed(Func callback, Args... args) {
    // Bind the function and its arguments into a single parameterless lambda
    current_callback_pressed = [=]() {
        std::invoke(callback, args...); 
    };

    // Pass the static bridge function pointer to PROS
    pros::screen::touch_callback(pros_callback_bridge_pressed, pros::E_TOUCH_PRESSED);
}

template<typename Func, typename... Args>
void Screen::released(Func callback, Args... args){

    // Bind the function and its arguments into a single parameterless lambda
    current_callback_released = [=]() {
        std::invoke(callback, args...); 
    };

    // Pass the static bridge function pointer to PROS
    pros::screen::touch_callback(pros_callback_bridge_released, pros::E_TOUCH_RELEASED);
}

char* SdCard::loadfile(std::string filename, char* buffer){
    std::string FilePath = "/usd/" + filename;

    if (buffer == nullptr){
        FILE* file = fopen(FilePath.c_str(), "r");
        if (file == NULL){
            mp_raise_OSError(5);
        }

        fseek(file, 0, SEEK_END);

        long size = ftell(file);

        char* NewBuffer = new char[size + 1];

        NewBuffer[size] = '\0';

        fread(NewBuffer, sizeof(NewBuffer), size, file);
        fclose(file);

        return NewBuffer;
    }else{
        FILE* file = fopen(FilePath.c_str(), "r");
        if (file == NULL){
            mp_raise_OSError(5);
        }

        fseek(file, 0, SEEK_END);

        long size = ftell(file);

        if (size >= sizeof(buffer)){
            char* NewBuffer = new char[size + 1];

            NewBuffer[size] = '\0';

            fread(NewBuffer, sizeof(NewBuffer), size, file);
            fclose(file);

            return NewBuffer;
        }else{
            fread(buffer, sizeof(buffer), size, file);

            buffer[size] = '\0';
            fclose(file);

            return buffer;
        }
    }
}

void SdCard::savefile(std::string filename, char* buffer){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "w");
    if (file != NULL) {
        fprintf(file, buffer);
        fclose(file);
    }else{
        
    }
}

void SdCard::appendfile(std::string filename, char* buffer){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "a");
    if (file != NULL) {
        fprintf(file, buffer);
        fclose(file);
    }else{
        
    }
}

void SdCard::draw_image_from_file(std::string filename, int x, int y){
    // in progress
}

bool SdCard::is_inserted(){
    return pros::usd::is_installed();
}

int SdCard::filesize(std::string filename){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "r");
    if (file == NULL){
        mp_raise_OSError(5);
    }

    fseek(file, 0, SEEK_END);

    long size = ftell(file);
    fclose(file);
    return size;
}

int SdCard::size(std::string filename){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "r");
    if (file == NULL){
        mp_raise_OSError(5);
    }

    fseek(file, 0, SEEK_END);

    long size = ftell(file);
    fclose(file);
    return size;
}

bool SdCard::exists(std::string filename){
    bool output = false;
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "r");
    if (file != NULL){
        output = true;
    }

    fclose(file);
    return output;
}

Brain::Brain(){
    ThreeWirePort three_wire_port;
    Screen screen;
    SdCard sdcard;
    Battery Battery;
}