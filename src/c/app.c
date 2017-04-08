#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static DataLoggingSessionRef s_session_ref;

static void update_time(struct tm *restrict tick_time) {
    // Write the current hours and minutes into a buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer),
             clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

static void main_window_load(Window *window) {
    // Get information about the Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create the TextLayer with specific bounds
    s_time_layer = text_layer_create(
        GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

    // Improve the layout to be more like a watchface
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_font(s_time_layer,
                        fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
}

typedef struct {
    uint64_t timestamp;
    int16_t x, y, z;
} accel_item;

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
    for (uint32_t i = 0; i < num_samples; ++i) {
        accel_item itm = {data[i].timestamp, data[i].x, data[i].y, data[i].z};
        DataLoggingResult result = data_logging_log(s_session_ref, &itm, 1);
        APP_LOG(APP_LOG_LEVEL_INFO, "%d", result);
    }
}

static void init() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window,
                               (WindowHandlers){.load = main_window_load,
                                                .unload = main_window_unload});

    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);

    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    update_time(tick_time);

    s_session_ref = data_logging_create(1, DATA_LOGGING_BYTE_ARRAY,
                                        sizeof(accel_item), true);

    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    uint32_t num_samples = 25;  // Number of samples per batch/callback
    accel_data_service_subscribe(num_samples, accel_data_handler);
}

static void deinit() {
    // Destroy Window
    data_logging_finish(s_session_ref);
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
