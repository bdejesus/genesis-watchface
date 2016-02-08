#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

#define COLOR_WIN GColorBlack
#define COLOR_BG GColorClear
#define COLOR_TIME GColorRed
#define COLOR_TEXT GColorWhite

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_weather_layer;
static GFont s_time_font, s_date_font, s_weather_font;
static int s_battery_level;
static Layer *s_battery_layer;

// Store incoming WEATHER info
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];


// BATTERY FUNCTIONS
static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Size the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * bounds.size.w);
  int position = (int)(float)( (bounds.size.w / 2) - (width / 2) );
  
  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBulgarianRose);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(position, 0, width, bounds.size.h), 0, GCornerNone);
}


// WEATHER FUNCTIONS
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Messaged dropped");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success");
}


// MAIN WINDOW LOAD
static void main_window_load(Window *window) {
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
    
  // TIME - Create and style time TextLayer
  s_time_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(34, 34), bounds.size.w, 80));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, COLOR_TIME);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // DATE - Create and style date TextLayer
  s_date_layer = text_layer_create(
    GRect(0, 134, bounds.size.w, 30));
  text_layer_set_text_color(s_date_layer, COLOR_TEXT);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // WEATHER - Create and style weather TextLayer
  s_weather_layer = text_layer_create(
    GRect(0, PBL_IF_ROUND_ELSE(28, 28), bounds.size.w, 25));
  text_layer_set_text_color(s_weather_layer, COLOR_TEXT);
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "BDJ");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  // BATTERY - Create battery meter Layer
  s_battery_layer = layer_create(GRect(8, 126, bounds.size.w - 16, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);  
  // Update meter
  layer_mark_dirty(s_battery_layer);

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_80));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TEXT_16));
  s_weather_font = s_date_font;
  // Apply to Text Layer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_font(s_weather_layer, s_weather_font);
}

// MAIN_WINDOW_UNLOAD
static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_weather_font);
  layer_destroy(s_battery_layer);
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%e %A", tick_time);
  
  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
    
    // Send the message!
    app_message_outbox_send();
  }
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Set the Window background color
  window_set_background_color(s_main_window, COLOR_WIN);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
}


static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
