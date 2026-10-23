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
    
}

int Screen::get_string_height(std::string string){

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
    
}

int Screen::x_position(){
    
}

int Screen::y_position(){
    
}

template<typename Func, typename... Args>
void Screen::pressed(Func callback, Args... args){
    
}

template<typename Func, typename... Args>
void Screen::released(Func callback, Args... args){
    
}

SdCard::SdCard(){

}

int SdCard::cache_file(std::string filename){

    if (MemoryOffset + sizeof(RobotStateData) > CACHE_MAX_BYTES) {
        return false; // Cache full!
    }

    // 1. Copy the data into the raw block at the current offset
    std::memcpy(raw_memory_block + current_byte_offset, &data, sizeof(RobotStateData));

    // 2. Save the location offset in our map index
    cache_index[key] = current_byte_offset;

    // 3. Move the offset forward for the next data entry
    current_byte_offset += sizeof(RobotStateData);
    return true;

    
    std::string path = "/usd/";
    path += filename;
    FILE* file = fopen(path.c_str(), "r");

    if (file == NULL){
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    
    file_cache[filename] = (char*)malloc(file_size + 1);

    fread(file_cache[filename], 1 , file_size, file);

    file_cache[filename][file_size] = '\0';
    file_sizes[filename] = file_size;

    fclose(file);
}

char* SdCard::read_from_cache(std::string filename){
    return file_cache[filename];
}

char* SdCard::loadfile(std::string filename, char* buffer){
    if (file_cache.count(filename) > 0){
        return read_from_cache(filename);
    }else{
        cache_file(filename);
        return read_from_cache(filename);
    }
}

void SdCard::savefile(std::string filename, char* buffer){
    long new_size = file_sizes[filename] + strlen(buffer);
    char* temp = (char*)realloc(file_cache[filename], new_size);
    if (temp != NULL) {
        file_cache[filename] = temp;
    } else {
        // Handle out-of-memory error
        free(temp);
        return;
    }
    file_cache[filename] = buffer;
}

void SdCard::appendfile(std::string filename, char* buffer){
    long new_size = file_sizes[filename] + strlen(buffer);
    char* temp = (char*)realloc(file_cache[filename], new_size);
    if (temp != NULL) {
        file_cache[filename] = temp;
    } else {
        // Handle out-of-memory error
        free(temp);
        return;
    }

    file_cache[filename][file_sizes[filename]] = buffer;

    free(temp);
}

void SdCard::draw_image_from_file(std::string filename, int x, int y){
    
}

bool SdCard::is_inserted(){

}

int SdCard::filesize(std::string filename){

}

int SdCard::size(std::string filename){

}

bool SdCard::exists(std::string filename){
    
}

Brain::Brain(){
    Screen screen;
    SdCard sdcard;
    Battery Battery;
}