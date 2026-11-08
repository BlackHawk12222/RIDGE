#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <array>
#include <map>
#include <functional>
#include <fstream>
#include <cstdio>
#include <cerrno>
#include "main.h"
#include "vex/brain.hpp"

extern "C" {
    #include "vex/upng/upng.h"
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

const lv_font_t* Screen::current_font() const {
    // The generated mono files currently contain only the space glyph.
    static const std::array<const lv_font_t*, 6> fallback_fonts = {
        &lv_font_montserrat_12,
        &lv_font_montserrat_16,
        &lv_font_montserrat_20,
        &lv_font_montserrat_30,
        &lv_font_montserrat_40,
        &lv_font_montserrat_48,
    };
    static const std::array<lv_font_t, 6> screen_fonts = [] {
        std::array<lv_font_t, 6> fonts{};
        for (size_t i = 0; i < fonts.size(); ++i) {
            fonts[i] = *font_options[i];
            fonts[i].fallback = fallback_fonts[i];
            fonts[i].line_height = fallback_fonts[i]->line_height;
            fonts[i].base_line = fallback_fonts[i]->base_line;
        }
        return fonts;
    }();

    const int value = static_cast<int>(font);
    if (value >= MONO12 && value <= MONO60) {
        return &screen_fonts[value - MONO12];
    }

    switch (font) {
        case PROP20: return &lv_font_montserrat_20;
        case PROP30: return &lv_font_montserrat_30;
        case PROP40: return &lv_font_montserrat_40;
        case PROP60: return &lv_font_montserrat_48;
        default: mp_raise_ValueError(MP_ERROR_TEXT("Invalid screen font"));
    }
    return nullptr;
}

int Screen::font_height() const {
    return current_font()->line_height;
}

int Screen::character_width() const {
    switch (font) {
        case MONO12: return 6;
        case MONO15: return 7;
        case MONO20:
        case PROP20: return 10;
        case MONO30:
        case PROP30: return 15;
        case MONO40:
        case PROP40: return 20;
        case MONO60:
        case PROP60: return 30;
        default: return 10;
    }
}

std::string Screen::format_text(const std::string& text, const std::string& sep, int precision) {
    if (sep.empty()) {
        if (is_number(text)) {
            return std::format("{:.{}f}", std::stod(text), precision);
        }
        return text;
    }

    std::stringstream stream(text);
    std::string line;
    std::string output;
    if (std::getline(stream, line)) {
        output = line;
    }
    while (std::getline(stream, line)) {
        output += sep + line;
    }
    return output;
}

void Screen::draw_text(const std::string& text, int x, int y, bool opaque) {
    lv_lock();
    const std::pair<int, int> position{x, y};
    auto label = TextLabels.find(position);
    if (label == TextLabels.end()) {
        lv_obj_t* object = lv_label_create(lv_screen_active());
        if (object == nullptr) {
            lv_unlock();
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to create screen text"));
        }
        label = TextLabels.emplace(position, object).first;
    }

    lv_obj_t* object = label->second;
    lv_label_set_text(object, text.c_str());
    lv_obj_set_style_text_font(object, current_font(), LV_PART_MAIN);
    lv_obj_set_style_text_color(object, lv_color_hex(static_cast<uint32_t>(PenColor)), LV_PART_MAIN);
    lv_obj_set_style_text_opa(object, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(object, lv_color_hex(BackgroundColor), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(object, opaque ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(object, 0, LV_PART_MAIN);
    lv_obj_set_pos(object, x, y);
    lv_unlock();
}

void Screen::delete_text_labels(int x, int y, int width, int height) {
    lv_lock();
    for (auto label = TextLabels.begin(); label != TextLabels.end();) {
        lv_obj_t* object = label->second;
        const int label_x = lv_obj_get_x(object);
        const int label_y = lv_obj_get_y(object);
        const int label_right = label_x + lv_obj_get_width(object);
        const int label_bottom = label_y + lv_obj_get_height(object);
        if (label_x < x + width && label_right > x && label_y < y + height && label_bottom > y) {
            lv_obj_delete(object);
            label = TextLabels.erase(label);
        } else {
            ++label;
        }
    }
    lv_unlock();
}

void Screen::print(std::string text, std::string sep, int precision) {
    std::string output = format_text(text, sep, precision);
    const int x = ScreenX * character_width();
    const int y = ScreenY * font_height();
    if (Render) {
        XBuffer.push_back(x);
        YBuffer.push_back(y);
        StringBuffer.push_back(std::move(output));
        OpaqueBuffer.push_back(true);
        return;
    }
    draw_text(output, x, y, true);
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
    int text_height = font_height();
    pros::screen::set_pen(color.ColorValue);
    pros::screen::draw_rect(0, row * text_height, 480, (row * text_height) + text_height);
    pros::screen::set_pen(PenColor);
    delete_text_labels(0, row * text_height, 480, text_height);
}

int Screen::row(){
    return ScreenY;
}

int Screen::column(){
    return ScreenX;
}

void Screen::print_at(std::string text, int x, int y, std::string sep, int precision, bool opaque){
    std::string output = format_text(text, sep, precision);
    x += XOffset;
    y += YOffset;
    if (Render) {
        XBuffer.push_back(x);
        YBuffer.push_back(y);
        StringBuffer.push_back(std::move(output));
        OpaqueBuffer.push_back(opaque);
        return;
    }
    draw_text(output, x, y, opaque);
}

int Screen::get_string_width(std::string string){
    lv_lock();
    int width = lv_text_get_width(string.c_str(), static_cast<uint32_t>(string.size()), current_font(), 0);
    lv_unlock();
    return width;
}

int Screen::get_string_height(std::string string){
    (void)string;
    return current_font()->line_height;
}

void Screen::clear_line(int line){
    const int y = line * font_height();
    pros::screen::erase_line(0, y, 479, y + font_height());
    delete_text_labels(0, y, 480, font_height());
}

void Screen::new_line(){
    ScreenY+=1;
}

void Screen::clear_screen(Color color){
    lv_lock();
    for (const auto& label : TextLabels) {
        lv_obj_delete(label.second);
    }
    TextLabels.clear();
    lv_unlock();
    BackgroundColor = color.ColorValue;
    pros::screen::set_eraser(color.ColorValue);
    pros::screen::erase();
}

void Screen::set_font(Font font){
    this->font = font;
    current_font();
}

void Screen::set_pen_width(int value){
    PenWidth = value;
}

void Screen::set_pen_color(Color color){
    PenColor = static_cast<pros::Color>(color.ColorValue);
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
        for (size_t i = 0; i < StringBuffer.size(); ++i) {
            draw_text(StringBuffer[i], XBuffer[i], YBuffer[i], OpaqueBuffer[i]);
        }

        XBuffer.clear();
        YBuffer.clear();
        StringBuffer.clear();
        OpaqueBuffer.clear();
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

char* SdCard::loadfile(std::string filename, char* buffer, size_t buffer_size, size_t* result_size){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "rb");
    if (file == NULL){
        mp_raise_OSError(errno);
    }

    if (fseek(file, 0, SEEK_END) != 0){
        int error = errno == 0 ? EIO : errno;
        fclose(file);
        mp_raise_OSError(error);
    }

    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0){
        int error = errno == 0 ? EIO : errno;
        fclose(file);
        mp_raise_OSError(error);
    }

    size_t file_size = static_cast<size_t>(size);
    if (buffer != nullptr && buffer_size < file_size){
        fclose(file);
        mp_raise_ValueError("buffer too small");
    }

    char* output = (buffer == nullptr) ? new char[file_size == 0 ? 1 : file_size] : buffer;
    if (fread(output, 1, file_size, file) != file_size){
        int error = errno == 0 ? EIO : errno;
        fclose(file);
        if (buffer == nullptr){
            delete[] output;
        }
        mp_raise_OSError(error);
    }

    if (fclose(file) != 0){
        int error = errno == 0 ? EIO : errno;
        if (buffer == nullptr){
            delete[] output;
        }
        mp_raise_OSError(error);
    }
    *result_size = file_size;
    return output;
}

void SdCard::savefile(std::string filename, const char* buffer, size_t buffer_size){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "wb");
    if (file == NULL) {
        mp_raise_OSError(errno);
    }

    if (buffer_size > 0 && fwrite(buffer, 1, buffer_size, file) != buffer_size) {
        int error = errno == 0 ? EIO : errno;
        fclose(file);
        mp_raise_OSError(error);
    }
    if (fclose(file) != 0) {
        mp_raise_OSError(errno == 0 ? EIO : errno);
    }
}

void SdCard::appendfile(std::string filename, const char* buffer, size_t buffer_size){
    std::string FilePath = "/usd/" + filename;

    FILE* file = fopen(FilePath.c_str(), "ab");
    if (file == NULL) {
        mp_raise_OSError(errno);
    }

    if (buffer_size > 0 && fwrite(buffer, 1, buffer_size, file) != buffer_size) {
        int error = errno == 0 ? EIO : errno;
        fclose(file);
        mp_raise_OSError(error);
    }
    if (fclose(file) != 0) {
        mp_raise_OSError(errno == 0 ? EIO : errno);
    }
}

// --- Private Helper: Safe Bare-Metal BMP Decoder ---
void SdCard::draw_bmp(std::ifstream& file, int x_offset, int y_offset) {
    uint8_t header[54];
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(header), 54);

    uint32_t data_offset = *reinterpret_cast<uint32_t*>(&header[10]);
    int32_t  width       = *reinterpret_cast<int32_t*>(&header[18]);
    int32_t  height      = *reinterpret_cast<int32_t*>(&header[22]);
    uint16_t bits_per_px = *reinterpret_cast<uint16_t*>(&header[28]);

    if (bits_per_px != 24) {
        printf("Error: Only uncompressed 24-bit BMP files are supported.\n");
        return;
    }

    int row_size = ((width * 3 + 3) & ~3);
    std::vector<uint8_t> row_buffer(row_size);
    file.seekg(data_offset, std::ios::beg);

    for (int y = height - 1; y >= 0; --y) {
        file.read(reinterpret_cast<char*>(row_buffer.data()), row_size);
        for (int x = 0; x < width; ++x) {
            int target_x = x + x_offset;
            int target_y = y + y_offset;

            if (target_x >= 0 && target_x < 480 && target_y >= 0 && target_y < 272) {
                uint8_t b = row_buffer[x * 3];
                uint8_t g = row_buffer[x * 3 + 1];
                uint8_t r = row_buffer[x * 3 + 2];
                pros::screen::set_pen((r << 16) | (g << 8) | b);
                pros::screen::draw_pixel(target_x, target_y);
            }
        }
    }
}

// --- Private Helper: uPNG ARM-safe Decoder ---
void SdCard::draw_png(const std::string& filename, int x_offset, int y_offset) {
    upng_t* upng = upng_new_from_file(filename.c_str());
    if (upng == NULL) return;

    if (upng_header(upng) == UPNG_EOK && upng_decode(upng) == UPNG_EOK) {
        unsigned width = upng_get_width(upng);
        unsigned height = upng_get_height(upng);
        const unsigned char* buffer = upng_get_buffer(upng);
        upng_format format = upng_get_format(upng);

        // Standard PNG formats throw either 3 channels (RGB) or 4 channels (RGBA)
        int channels = (format == UPNG_RGBA8) ? 4 : 3;

        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                int target_x = x + x_offset;
                int target_y = y + y_offset;

                if (target_x >= 0 && target_x < 480 && target_y >= 0 && target_y < 272) {
                    int idx = (y * width + x) * channels;
                    uint8_t r = buffer[idx + 0];
                    uint8_t g = buffer[idx + 1];
                    uint8_t b = buffer[idx + 2];

                    pros::screen::set_pen((r << 16) | (g << 8) | b);
                    pros::screen::draw_pixel(target_x, target_y);
                }
            }
        }
    } else {
        printf("uPNG Error: Failed to decode PNG line. Code: %d\n", upng_get_error(upng));
    }
    upng_free(upng);
}

void SdCard::draw_image_from_file(std::string filename, int x, int y){
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        printf("Error: File not found %s\n", filename.c_str());
        return;
    }

    uint8_t magic[4];
    file.read(reinterpret_cast<char*>(magic), 4);

    if (magic[0] == 'B' && magic[1] == 'M') {
        printf("Drawing BMP...\n");
        draw_bmp(file, x, y);
        file.close();
    } 
    else if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        printf("Drawing PNG...\n");
        file.close(); // Close file stream so uPNG can mount the path cleanly
        draw_png(filename, x, y);
    } 
    else {
        printf("Format not supported. Only standard .png and 24-bit .bmp files.\n");
        file.close();
    }
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