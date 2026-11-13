#ifndef BRAIN_HPP
#define BRAIN_HPP

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <deque>
#include <functional>
#include <cstdint>
#include <atomic>
#include "main.h"
#include "vex/font_init.h"
#include "timer.hpp"
#include "pros/rtos.hpp"

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
    public:
        static constexpr int WIDTH = 480;
        static constexpr int HEIGHT = 272;
        // Passed as a color to draw_rectangle/draw_circle to mean "use set_fill_color()".
        static constexpr std::uint32_t USE_FILL = 0xFFFFFFFFu;

        // Starts the LVGL backend. Call once from initialize(); later calls do nothing.
        // Drawing calls also start it on first use.
        static void init();

        // Shows an error box on top of everything on the brain screen, like the
        // red error popup VEXcode V5 Python shows for an uncaught exception.
        // Safe to call from any task. Tap the box to hide it; a new error
        // shows it again. Only the last lines fit, so the error line is kept.
        static void show_error(const std::string& text);

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
        void draw_rectangle(int x, int y, int width, int height, Color color = Color(USE_FILL));
        void draw_circle(int x, int y, int radius, Color color = Color(USE_FILL));
        // Blits 0xRRGGBB pixels (row-major, width*height). Used by SdCard images.
        void draw_buffer(const std::uint32_t* pixels, int x, int y, int width, int height);
        void render();
        void set_clip_region(int x, int y, int width, int height);
        bool pressing();
        int x_position();
        int y_position();

        template<typename Func, typename... Args>
        void pressed(Func callback, Args... args) {
            set_touch_callback(true, [=]() { std::invoke(callback, args...); });
        }

        template<typename Func, typename... Args>
        void released(Func callback, Args... args){
            set_touch_callback(false, [=]() { std::invoke(callback, args...); });
        }

    private:
        enum class Op : std::uint8_t { Text, Rect, Circle, Line, Pixel, Blit, Clear };

        // One drawing command. Built on the caller's thread, executed on the LVGL thread.
        struct Cmd {
            Op op = Op::Clear;
            std::int16_t x = 0, y = 0;     // Line: start. Circle: center.
            std::int16_t w = 0, h = 0;     // Line: end x/y. Circle: w = radius.
            std::int16_t cx1 = 0, cy1 = 0, cx2 = WIDTH - 1, cy2 = HEIGHT - 1; // clip, inclusive
            std::uint32_t pen = 0xFFFFFF;
            std::uint32_t fill = 0x000000;
            std::uint8_t pen_width = 1;
            bool opaque = false;
            const lv_font_t* font = nullptr;
            std::string text;
            std::vector<std::uint32_t> pixels;
        };

        // Per-object drawing state (caller's thread only).
        Font font = MONO20;
        int ScreenX = 1;              // 1-based column, like VEXcode
        int ScreenY = 1;              // 1-based row, like VEXcode
        int XOffset = 0;
        int YOffset = 0;
        int PenWidth = 1;
        std::uint32_t PenColor = 0xFFFFFF;
        std::uint32_t FillColor = 0x000000;
        std::uint32_t BackgroundColor = 0x000000;
        std::int16_t ClipX1 = 0, ClipY1 = 0, ClipX2 = WIDTH - 1, ClipY2 = HEIGHT - 1;
        bool Render = false;          // true after the first render(): double-buffered
        std::vector<Cmd> Frame;       // commands waiting for the next render()

        bool is_number(const std::string& str);
        const lv_font_t* current_font() const;
        int font_height() const;
        int character_width() const;
        std::string format_text(const std::string& text, const std::string& sep, int precision);
        Cmd make_cmd(Op op) const;
        void submit(Cmd&& cmd);

        // Shared LVGL backend (one physical screen).
        static inline std::vector<Cmd> queue_;                 // guarded by queue_mutex()
        static inline lv_obj_t* canvas_ = nullptr;             // LVGL thread only
        static inline lv_draw_buf_t* buf_ = nullptr;           // LVGL thread only
        static inline std::atomic<bool> started_{false};
        static inline std::atomic<int> touch_x_{0};
        static inline std::atomic<int> touch_y_{0};
        static inline std::atomic<bool> touch_down_{false};
        static inline std::atomic<std::uint32_t> press_events_{0};
        static inline std::atomic<std::uint32_t> release_events_{0};
        static inline std::function<void()> on_pressed_;       // guarded by callback_mutex()
        static inline std::function<void()> on_released_;      // guarded by callback_mutex()
        static inline pros::Task* callback_task_ = nullptr;
        static pros::Mutex& queue_mutex();
        static pros::Mutex& callback_mutex();
        static void enqueue(Cmd&& cmd);
        static void enqueue_frame(std::vector<Cmd>& frame);
        static void lvgl_tick(lv_timer_t* timer);
        static inline std::string error_text_;                 // guarded by queue_mutex()
        static inline std::atomic<bool> error_pending_{false};
        static inline lv_obj_t* error_box_ = nullptr;          // LVGL thread only
        static inline lv_obj_t* error_label_ = nullptr;        // LVGL thread only
        static void update_error_box();
        static void create_canvas();
        static void execute(std::vector<Cmd>& work);
        static void poll_touch();
        static void set_touch_callback(bool on_press, std::function<void()> fn);
};

class SdCard{
    private:
        Screen* screen = nullptr;
        friend class Brain;
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
        Brain() { sdcard.screen = &screen; }
        ThreeWirePort three_wire_port;
        Screen screen;
        SdCard sdcard;
        Battery battery;
};

#endif //BRAIN_HPP