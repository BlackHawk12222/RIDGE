#include <string>
#include <vector>
#include <sstream>
#include <charconv>
#include <array>
#include <map>
#include <functional>
#include <fstream>
#include <cstdio>
#include <deque>
#include <mutex>
#include <cerrno>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <format>
#include "main.h"
#include "vex/brain.hpp"
#include "vex/safe_value.hpp"

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
        output = vsafe::num(pros::battery::get_voltage()) / 1000.0f;
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
            output = vsafe::num(pros::battery::get_current()) / 1000.0f;
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
    return (int)vsafe::num(pros::battery::get_capacity());
}

int Battery::temperature(TemperatureUnits units){
    int output = 2;

    switch (units)
    {
    case TEMPERATURE_PERCENT:
        output = ((vsafe::num(pros::battery::get_temperature()) - 20) / (70 - 20)) * 100.0;
        
        // Constrain boundaries between 0% and 100%
        if (output < 0.0) output = 0.0;
        if (output > 100.0) output = 100.0;
        break;
    case CELSIUS:
    {
        output = (int)vsafe::num(pros::battery::get_temperature());
        break;
    }
    case FAHRENHEIT:
    {
        output = (int)((vsafe::num(pros::battery::get_temperature()) * 9.0 / 5.0) + 32.0);
        break;
    }
    default:
        break;
    }

    return output;
}

// ===================== Screen (LVGL-only backend) =====================
//
// How it works:
// - The whole brain screen is one LVGL canvas with a 480x272 XRGB8888 buffer.
// - Drawing calls never touch LVGL. They build small commands and queue them.
// - An lv_timer runs inside lv_timer_handler (PROS's LVGL thread). It takes the
//   queue, draws it into the canvas with LVGL's renderer, and invalidates the
//   canvas. Every LVGL call happens on LVGL's own thread, so nothing races
//   (this liblvgl is built with LV_USE_OS = NONE, so lv_lock() is a no-op and
//   can't protect calls from other tasks). A batch always finishes before the
//   next refresh, so you never see half a frame.
// - After the first render(), commands are held until render() and handed over
//   as one batch: real double buffering, no flicker.
// - clear_screen() drops everything queued before it, so a loop that clears and
//   redraws can't build a backlog. Immediate mode blocks briefly past
//   kMaxQueued commands instead of growing forever.
// - Touch comes from LVGL's pointer input device, read on the same timer.
//   pressed()/released() callbacks run on their own task, never on LVGL's.
//
// Nothing here uses pros::screen.

namespace {
    constexpr std::size_t kMaxQueued = 2048;
    constexpr std::uint32_t kTickMs = 5;

    constexpr std::uint32_t kTransparent = 0x1000000u;   // Color.TRANSPARENT
    constexpr std::uint32_t kColorMask = 0x1FFFFFFu;     // RGB + transparent bit
    inline std::uint32_t xrgb(std::uint32_t rgb) { return 0xFF000000u | (rgb & 0x00FFFFFFu); }
    inline bool is_clear(std::uint32_t c) { return (c & kTransparent) != 0; }
    inline lv_opa_t opa_of(std::uint32_t c) { return is_clear(c) ? LV_OPA_TRANSP : LV_OPA_COVER; }
    inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
}

pros::Mutex& Screen::queue_mutex() { static pros::Mutex m; return m; }
pros::Mutex& Screen::callback_mutex() { static pros::Mutex m; return m; }

void Screen::init() {
    bool expected = false;
    if (started_.compare_exchange_strong(expected, true)) {
        // The only LVGL call made off the LVGL thread, done once.
        lv_timer_create(&Screen::lvgl_tick, kTickMs, nullptr);
    }
}

// ---------- LVGL thread ----------

void Screen::create_canvas() {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    buf_ = lv_draw_buf_create(WIDTH, HEIGHT, LV_COLOR_FORMAT_XRGB8888, LV_STRIDE_AUTO);
    if (buf_ == nullptr) {
        return;
    }
    const std::uint32_t stride_px = buf_->header.stride / 4;
    for (int y = 0; y < HEIGHT; ++y) {
        std::uint32_t* row = reinterpret_cast<std::uint32_t*>(buf_->data) + y * stride_px;
        std::fill_n(row, WIDTH, xrgb(0));
    }

    canvas_ = lv_canvas_create(scr);
    lv_canvas_set_draw_buf(canvas_, buf_);
    lv_obj_set_pos(canvas_, 0, 0);
    lv_obj_remove_flag(canvas_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(canvas_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_to_index(canvas_, -1);   // on top of anything else on the screen
    lv_obj_invalidate(canvas_);
}

void Screen::poll_touch() {
    static lv_indev_t* indev = nullptr;
    static bool was_down = false;
    if (indev == nullptr) {
        for (lv_indev_t* i = lv_indev_get_next(nullptr); i != nullptr; i = lv_indev_get_next(i)) {
            if (lv_indev_get_type(i) == LV_INDEV_TYPE_POINTER) {
                indev = i;
                break;
            }
        }
        if (indev == nullptr) {
            return;
        }
    }

    const bool down = lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED;
    if (down) {
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        touch_x_.store(p.x, std::memory_order_relaxed);
        touch_y_.store(p.y, std::memory_order_relaxed);
    }
    touch_down_.store(down, std::memory_order_relaxed);

    if (down != was_down) {
        was_down = down;
        if (down && error_active_) {
            dismiss_error_box();
        }
        (down ? press_events_ : release_events_).fetch_add(1, std::memory_order_relaxed);
        if (callback_task_ != nullptr) {
            callback_task_->notify();
        }
    }
}

void Screen::lvgl_tick(lv_timer_t* /*timer*/) {
    if (canvas_ == nullptr) {
        create_canvas();
        if (canvas_ == nullptr) {
            return;
        }
    }

    poll_touch();

    static std::vector<Cmd> work;   // LVGL thread only; keeps its capacity
    if (!queue_mutex().take(0)) {   // never stall the LVGL thread; try next tick
        return;
    }
    work.swap(queue_);
    queue_mutex().give();

    const bool drew = !work.empty();
    if (drew) {
        execute(work);
        work.clear();
    }

    // The error box is painted into the canvas after the user's drawing, every
    // time that drawing changes, so it stays on top.
    bool new_error = false;
    if (error_pending_.load(std::memory_order_acquire)) {
        new_error = update_error_box();
    }
    if (error_active_ && (drew || new_error)) {
        draw_error_box();
    }
}

// ---------- Error popup ----------

void Screen::show_error(const std::string& text) {
    // Keep the last lines: the exception type and message come last in a
    // traceback, and they are the part that must stay readable.
    constexpr int kMaxLines = 11;
    std::string trimmed = text;
    while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r')) {
        trimmed.pop_back();
    }
    int lines = 0;
    for (std::size_t i = trimmed.size(); i-- > 0;) {
        if (trimmed[i] == '\n' && ++lines == kMaxLines) {
            trimmed = "...\n" + trimmed.substr(i + 1);
            break;
        }
    }

    init();
    queue_mutex().take();
    error_text_ = std::move(trimmed);
    queue_mutex().give();
    error_pending_.store(true, std::memory_order_release);
}

// LVGL thread only. Takes a new error from show_error(); false if the
// queue lock was busy (tried again next tick).
bool Screen::update_error_box() {
    if (!queue_mutex().take(0)) {
        return false;
    }
    error_shown_ = error_text_;
    error_pending_.store(false, std::memory_order_relaxed);
    queue_mutex().give();
    error_active_ = true;
    return true;
}

namespace {
    constexpr int kErrorMargin = 10;
    constexpr int kErrorPad = 8;
    constexpr std::uint32_t kErrorRed = 0xB00000;
}

// LVGL thread only. Paints the red error box into the canvas.
void Screen::draw_error_box() {
    const lv_font_t* font = &lv_font_montserrat_16;
    const int box_w = WIDTH - 2 * kErrorMargin;
    const int text_w = box_w - 2 * kErrorPad;

    lv_point_t size;
    lv_text_get_size(&size, error_shown_.c_str(), font, 0, 0, text_w, LV_TEXT_FLAG_NONE);
    const int box_h = clampi(size.y + 2 * kErrorPad, 40, HEIGHT - 2 * kErrorMargin);
    const int x0 = kErrorMargin;
    const int y0 = (HEIGHT - box_h) / 2;
    error_area_ = {x0, y0, x0 + box_w - 1, y0 + box_h - 1};

    lv_layer_t layer;
    lv_canvas_init_layer(canvas_, &layer);

    lv_draw_rect_dsc_t box;
    lv_draw_rect_dsc_init(&box);
    box.bg_color = lv_color_hex(kErrorRed);
    box.bg_opa = LV_OPA_COVER;
    box.border_color = lv_color_white();
    box.border_width = 2;
    box.border_opa = LV_OPA_COVER;
    box.radius = 6;
    lv_draw_rect(&layer, &box, &error_area_);

    lv_area_t text_area = {x0 + kErrorPad, y0 + kErrorPad,
                           x0 + box_w - 1 - kErrorPad, y0 + box_h - 1 - kErrorPad};
    lv_draw_label_dsc_t label;
    lv_draw_label_dsc_init(&label);
    label.text = error_shown_.c_str();
    label.font = font;
    label.color = lv_color_white();
    label.opa = LV_OPA_COVER;
    lv_draw_label(&layer, &label, &text_area);

    lv_canvas_finish_layer(canvas_, &layer);
    lv_obj_invalidate(canvas_);
}

// LVGL thread only. A tap dismisses the box; its area goes black until the
// program draws there again.
void Screen::dismiss_error_box() {
    error_active_ = false;
    const std::uint32_t stride_px = buf_->header.stride / 4;
    std::uint32_t* const base = reinterpret_cast<std::uint32_t*>(buf_->data);
    for (int y = error_area_.y1; y <= error_area_.y2; ++y) {
        std::fill_n(base + y * stride_px + error_area_.x1, error_area_.x2 - error_area_.x1 + 1, xrgb(0));
    }
    lv_obj_invalidate(canvas_);
}

void Screen::execute(std::vector<Cmd>& work) {
    const std::uint32_t stride_px = buf_->header.stride / 4;
    std::uint32_t* const base = reinterpret_cast<std::uint32_t*>(buf_->data);

    lv_layer_t layer;
    bool layer_open = false;
    auto open_layer = [&]() {
        if (!layer_open) {
            lv_canvas_init_layer(canvas_, &layer);
            layer_open = true;
        }
    };
    // Direct pixel writes must come after any LVGL draws queued before them.
    auto close_layer = [&]() {
        if (layer_open) {
            lv_canvas_finish_layer(canvas_, &layer);
            layer_open = false;
        }
    };

    for (Cmd& c : work) {
        switch (c.op) {
            case Op::Clear: {
                close_layer();
                const std::uint32_t v = xrgb(c.fill);
                for (int y = 0; y < HEIGHT; ++y) {
                    std::fill_n(base + y * stride_px, WIDTH, v);
                }
                break;
            }
            case Op::Pixel: {
                if (is_clear(c.pen) || c.x < c.cx1 || c.x > c.cx2 || c.y < c.cy1 || c.y > c.cy2) break;
                close_layer();
                base[c.y * stride_px + c.x] = xrgb(c.pen);
                break;
            }
            case Op::Blit: {
                close_layer();
                const int x0 = std::max<int>(c.x, c.cx1);
                const int x1 = std::min<int>(c.x + c.w - 1, c.cx2);
                const int y0 = std::max<int>(c.y, c.cy1);
                const int y1 = std::min<int>(c.y + c.h - 1, c.cy2);
                for (int y = y0; y <= y1; ++y) {
                    const std::uint32_t* src = c.pixels.data() + (y - c.y) * c.w + (x0 - c.x);
                    std::uint32_t* dst = base + y * stride_px + x0;
                    for (int x = x0; x <= x1; ++x) {
                        *dst++ = xrgb(*src++);
                    }
                }
                break;
            }
            default: {
                open_layer();
                lv_area_t clip = {c.cx1, c.cy1, c.cx2, c.cy2};
                layer._clip_area = clip;   // documented as settable before adding draw tasks

                if (c.op == Op::Text) {
                    lv_point_t size;
                    lv_text_get_size(&size, c.text.c_str(), c.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
                    lv_area_t area = {c.x, c.y, static_cast<int32_t>(c.x + size.x - 1), static_cast<int32_t>(c.y + size.y - 1)};
                    if (c.opaque && !is_clear(c.fill)) {
                        lv_draw_rect_dsc_t bg;
                        lv_draw_rect_dsc_init(&bg);
                        bg.bg_color = lv_color_hex(c.fill);
                        bg.bg_opa = LV_OPA_COVER;
                        lv_draw_rect(&layer, &bg, &area);
                    }
                    lv_draw_label_dsc_t d;
                    lv_draw_label_dsc_init(&d);
                    d.text = c.text.c_str();   // stays alive until finish_layer
                    d.font = c.font;
                    d.color = lv_color_hex(c.pen);
                    d.opa = opa_of(c.pen);
                    lv_draw_label(&layer, &d, &area);
                } else if (c.op == Op::Rect || c.op == Op::Circle) {
                    lv_draw_rect_dsc_t d;
                    lv_draw_rect_dsc_init(&d);
                    d.bg_color = lv_color_hex(c.fill);
                    d.bg_opa = opa_of(c.fill);
                    d.border_color = lv_color_hex(c.pen);
                    d.border_width = c.pen_width;
                    d.border_opa = c.pen_width > 0 ? opa_of(c.pen) : LV_OPA_TRANSP;
                    lv_area_t area;
                    if (c.op == Op::Rect) {
                        area = {c.x, c.y, static_cast<int32_t>(c.x + c.w - 1), static_cast<int32_t>(c.y + c.h - 1)};
                        d.radius = 0;
                    } else {
                        area = {static_cast<int32_t>(c.x - c.w), static_cast<int32_t>(c.y - c.w),
                                static_cast<int32_t>(c.x + c.w), static_cast<int32_t>(c.y + c.w)};
                        d.radius = LV_RADIUS_CIRCLE;
                    }
                    lv_draw_rect(&layer, &d, &area);
                } else if (c.op == Op::Line) {
                    lv_draw_line_dsc_t d;
                    lv_draw_line_dsc_init(&d);
                    d.p1.x = c.x;  d.p1.y = c.y;
                    d.p2.x = c.w;  d.p2.y = c.h;
                    d.color = lv_color_hex(c.pen);
                    d.width = c.pen_width > 0 ? c.pen_width : 1;
                    d.opa = opa_of(c.pen);
                    d.round_start = d.round_end = d.width > 2;   // thick lines join cleanly
                    lv_draw_line(&layer, &d);
                }
                break;
            }
        }
    }
    close_layer();
    lv_obj_invalidate(canvas_);
}

// ---------- caller's thread ----------

void Screen::enqueue(Cmd&& cmd) {
    init();
    while (true) {
        queue_mutex().take();
        if (cmd.op == Op::Clear) {
            queue_.clear();   // a full clear hides everything queued before it
        }
        if (queue_.size() < kMaxQueued) {
            queue_.push_back(std::move(cmd));
            queue_mutex().give();
            return;
        }
        queue_mutex().give();
        pros::delay(1);       // LVGL is behind; wait instead of growing the queue
    }
}

void Screen::enqueue_frame(std::vector<Cmd>& frame) {
    init();
    // Only the part after the last full clear matters.
    std::size_t first = 0;
    for (std::size_t i = frame.size(); i-- > 0;) {
        if (frame[i].op == Op::Clear) { first = i; break; }
    }
    const bool has_clear = !frame.empty() && frame[first].op == Op::Clear;

    queue_mutex().take();
    if (has_clear) {
        queue_.clear();
    }
    queue_.insert(queue_.end(),
                  std::make_move_iterator(frame.begin() + first),
                  std::make_move_iterator(frame.end()));
    queue_mutex().give();
    frame.clear();   // keeps capacity for the next frame
}

Screen::Cmd Screen::make_cmd(Op op) const {
    Cmd c;
    c.op = op;
    c.pen = PenColor;
    c.fill = FillColor;
    c.pen_width = static_cast<std::uint8_t>(clampi(PenWidth, 0, 255));
    c.cx1 = ClipX1; c.cy1 = ClipY1; c.cx2 = ClipX2; c.cy2 = ClipY2;
    return c;
}

void Screen::submit(Cmd&& cmd) {
    if (Render) {
        Frame.push_back(std::move(cmd));   // shown on the next render()
        return;
    }
    enqueue(std::move(cmd));
}

bool Screen::is_number(const std::string& str) {
    double value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    return ec == std::errc() && ptr == str.data() + str.size();
}

const lv_font_t* Screen::current_font() const {
    // The generated mono files currently contain only the space glyph,
    // so each one falls back to a Montserrat font of similar size.
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

void Screen::print(std::string text, std::string sep, int precision) {
    Cmd c = make_cmd(Op::Text);
    c.text = format_text(text, sep, precision);
    c.font = current_font();
    c.x = static_cast<std::int16_t>((ScreenX - 1) * character_width() + XOffset);
    c.y = static_cast<std::int16_t>((ScreenY - 1) * font_height() + YOffset);
    c.opaque = true;
    c.fill = BackgroundColor;
    ScreenX += static_cast<int>(c.text.size());   // VEXcode advances the cursor
    submit(std::move(c));
}

void Screen::print_at(std::string text, int x, int y, std::string sep, int precision, bool opaque){
    Cmd c = make_cmd(Op::Text);
    c.text = format_text(text, sep, precision);
    c.font = current_font();
    c.x = static_cast<std::int16_t>(x + XOffset);
    c.y = static_cast<std::int16_t>(y + YOffset);
    c.opaque = opaque;
    c.fill = BackgroundColor;
    submit(std::move(c));
}

void Screen::set_cursor(int row, int col){
    ScreenY = row;
    ScreenX = col;
}

void Screen::next_row(){
    ScreenY += 1;
    ScreenX = 1;
}

void Screen::new_line(){
    next_row();
}

int Screen::row(){
    return ScreenY;
}

int Screen::column(){
    return ScreenX;
}

void Screen::clear_row(int row, Color color){
    if (row == -1){
        row = ScreenY;
    }
    const int h = font_height();
    Cmd c = make_cmd(Op::Rect);
    c.x = 0;
    c.y = static_cast<std::int16_t>((row - 1) * h);
    c.w = WIDTH;
    c.h = static_cast<std::int16_t>(h);
    c.fill = color.ColorValue;
    c.pen_width = 0;
    submit(std::move(c));
}

void Screen::clear_line(int line){
    clear_row(line, Color(BackgroundColor));
}

void Screen::clear_screen(Color color) {
    BackgroundColor = is_clear(color.ColorValue) ? 0 : (color.ColorValue & 0x00FFFFFFu);
    Cmd c = make_cmd(Op::Clear);
    c.fill = BackgroundColor;
    submit(std::move(c));
}

int Screen::get_string_width(std::string string){
    // Pure font math: safe on any thread.
    return lv_text_get_width(string.c_str(), static_cast<uint32_t>(string.size()), current_font(), 0);
}

int Screen::get_string_height(std::string string){
    (void)string;
    return current_font()->line_height;
}

void Screen::set_font(Font font){
    this->font = font;
    current_font();   // raises ValueError for a bad font now, not at draw time
}

void Screen::set_pen_width(int value){
    PenWidth = value;
}

void Screen::set_pen_color(Color color){
    PenColor = color.ColorValue & kColorMask;
}

void Screen::set_fill_color(Color color){
    FillColor = color.ColorValue & kColorMask;
}

void Screen::set_origin(int x, int y){
    XOffset = x;
    YOffset = y;
}

void Screen::set_clip_region(int x, int y, int width, int height){
    ClipX1 = static_cast<std::int16_t>(clampi(x, 0, WIDTH - 1));
    ClipY1 = static_cast<std::int16_t>(clampi(y, 0, HEIGHT - 1));
    ClipX2 = static_cast<std::int16_t>(clampi(x + width - 1, 0, WIDTH - 1));
    ClipY2 = static_cast<std::int16_t>(clampi(y + height - 1, 0, HEIGHT - 1));
}

void Screen::draw_pixel(int x, int y){
    x += XOffset;
    y += YOffset;
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    Cmd c = make_cmd(Op::Pixel);
    c.x = static_cast<std::int16_t>(x);
    c.y = static_cast<std::int16_t>(y);
    submit(std::move(c));
}

void Screen::draw_line(int start_x, int start_y, int end_x, int end_y){
    Cmd c = make_cmd(Op::Line);
    c.x = static_cast<std::int16_t>(start_x + XOffset);
    c.y = static_cast<std::int16_t>(start_y + YOffset);
    c.w = static_cast<std::int16_t>(end_x + XOffset);
    c.h = static_cast<std::int16_t>(end_y + YOffset);
    submit(std::move(c));
}

void Screen::draw_rectangle(int x, int y, int width, int height, Color color){
    if (width <= 0 || height <= 0) return;
    Cmd c = make_cmd(Op::Rect);
    c.x = static_cast<std::int16_t>(x + XOffset);
    c.y = static_cast<std::int16_t>(y + YOffset);
    c.w = static_cast<std::int16_t>(width);
    c.h = static_cast<std::int16_t>(height);
    if (color.ColorValue != USE_FILL) c.fill = color.ColorValue & kColorMask;
    submit(std::move(c));
}

void Screen::draw_circle(int x, int y, int radius, Color color){
    if (radius <= 0) return;
    Cmd c = make_cmd(Op::Circle);
    c.x = static_cast<std::int16_t>(x + XOffset);
    c.y = static_cast<std::int16_t>(y + YOffset);
    c.w = static_cast<std::int16_t>(radius);
    if (color.ColorValue != USE_FILL) c.fill = color.ColorValue & kColorMask;
    submit(std::move(c));
}

void Screen::draw_buffer(const std::uint32_t* pixels, int x, int y, int width, int height){
    if (pixels == nullptr || width <= 0 || height <= 0) return;
    Cmd c = make_cmd(Op::Blit);
    c.x = static_cast<std::int16_t>(x + XOffset);
    c.y = static_cast<std::int16_t>(y + YOffset);
    c.w = static_cast<std::int16_t>(width);
    c.h = static_cast<std::int16_t>(height);
    c.pixels.assign(pixels, pixels + static_cast<std::size_t>(width) * height);
    submit(std::move(c));
}

void Screen::render(){
    if (Render) {
        enqueue_frame(Frame);   // whole frame lands in one LVGL tick
    }
    Render = true;              // like VEXcode: after the first render(), draw only on render()
}

bool Screen::pressing(){
    init();
    return touch_down_.load(std::memory_order_relaxed);
}

int Screen::x_position(){
    init();
    return touch_x_.load(std::memory_order_relaxed);
}

int Screen::y_position(){
    init();
    return touch_y_.load(std::memory_order_relaxed);
}

void Screen::set_touch_callback(bool on_press, std::function<void()> fn) {
    init();
    callback_mutex().take();
    (on_press ? on_pressed_ : on_released_) = std::move(fn);
    if (callback_task_ == nullptr) {
        callback_task_ = new pros::Task([]() {
            std::uint32_t seen_press = press_events_.load();
            std::uint32_t seen_release = release_events_.load();
            while (true) {
                pros::Task::notify_take(true, TIMEOUT_MAX);
                const std::uint32_t p = press_events_.load();
                const std::uint32_t r = release_events_.load();
                std::function<void()> pressed_fn, released_fn;
                callback_mutex().take();
                pressed_fn = on_pressed_;
                released_fn = on_released_;
                callback_mutex().give();
                if (p != seen_press) { seen_press = p; if (pressed_fn) pressed_fn(); }
                if (r != seen_release) { seen_release = r; if (released_fn) released_fn(); }
            }
        }, TASK_PRIORITY_DEFAULT, TASK_STACK_DEPTH_DEFAULT, "VEX Screen Touch");
    }
    callback_mutex().give();
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

    if (bits_per_px != 24 || width <= 0 || height == 0) {
        printf("Error: Only uncompressed 24-bit BMP files are supported.\n");
        return;
    }
    const bool bottom_up = height > 0;
    if (height < 0) height = -height;

    int row_size = ((width * 3 + 3) & ~3);
    std::vector<uint8_t> row_buffer(row_size);
    std::vector<uint32_t> pixels(static_cast<size_t>(width) * height);
    file.seekg(data_offset, std::ios::beg);

    for (int i = 0; i < height; ++i) {
        const int y = bottom_up ? (height - 1 - i) : i;
        file.read(reinterpret_cast<char*>(row_buffer.data()), row_size);
        uint32_t* dst = pixels.data() + static_cast<size_t>(y) * width;
        for (int x = 0; x < width; ++x) {
            dst[x] = (uint32_t(row_buffer[x * 3 + 2]) << 16) | (uint32_t(row_buffer[x * 3 + 1]) << 8) | row_buffer[x * 3];
        }
    }
    if (screen != nullptr) {
        screen->draw_buffer(pixels.data(), x_offset, y_offset, width, height);
    }
}

// --- Private Helper: uPNG ARM-safe Decoder ---
void SdCard::draw_png(const std::string& filename, int x_offset, int y_offset) {
    upng_t* upng = upng_new_from_file(filename.c_str());
    if (upng == NULL) return;

    if (upng_header(upng) == UPNG_EOK && upng_decode(upng) == UPNG_EOK) {
        const unsigned width = upng_get_width(upng);
        const unsigned height = upng_get_height(upng);
        const unsigned char* buffer = upng_get_buffer(upng);
        const int channels = (upng_get_format(upng) == UPNG_RGBA8) ? 4 : 3;

        std::vector<uint32_t> pixels(static_cast<size_t>(width) * height);
        for (size_t i = 0; i < pixels.size(); ++i) {
            const unsigned char* p = buffer + i * channels;
            pixels[i] = (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2];
        }
        if (screen != nullptr) {
            screen->draw_buffer(pixels.data(), x_offset, y_offset, static_cast<int>(width), static_cast<int>(height));
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
        fclose(file);   // fclose(NULL) crashes
    }
    return output;
}