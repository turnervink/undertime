#include <pebble.h>

static Window *main_window;
static Layer *background_layer, *character_layer, *hpbar_layer;
static TextLayer *time_layer, *love_layer, *hp_layer, *date_layer;
static GBitmap *background_bitmap;
static GBitmap *character_bitmap;
static GFont time_font, date_font;
static bool use_celsius;

int randnum; // Random number to pick character with
int totchars = 8; // Total number of characters

static bool testing = false;

bool pickedchar = false;

const int CHARACTER_IMAGES[] = {
	RESOURCE_ID_IMAGE_ALPHYS,
	RESOURCE_ID_IMAGE_MTT,
	RESOURCE_ID_IMAGE_NAPSTA,
	RESOURCE_ID_IMAGE_PAPYRUS,
	RESOURCE_ID_IMAGE_SANS,
	RESOURCE_ID_IMAGE_TEM,
	RESOURCE_ID_IMAGE_TORIEL,
	RESOURCE_ID_IMAGE_UNDYNE
};

static void update_time() {
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);

	static char time_buffer[] = "* 00:00";
	static char date_buffer[] = "MM/DD";

	if(clock_is_24h_style() == true) {
		strftime(time_buffer, sizeof("* 00:00"), "*%H:%M", tick_time);
	} else {
		strftime(time_buffer, sizeof("* 00:00"), "*%I:%M", tick_time);
	}

	strftime(date_buffer, sizeof("MM/DD"), "%m/%d", tick_time);

	text_layer_set_text(time_layer, time_buffer);
	text_layer_set_text(date_layer, date_buffer);
}

static void pick_character() {
	APP_LOG(APP_LOG_LEVEL_INFO, "Picking character");
	time_t temp = time(NULL);

	srand(temp); // Seed our random number using the current time
	randnum = rand() % totchars; // Pick a random number between 0 and the amount of available characters
	APP_LOG(APP_LOG_LEVEL_INFO, "Picked character: %d", randnum);

	if (pickedchar == true) { // If there is an old bitmap we destroy it to free memory
		APP_LOG(APP_LOG_LEVEL_INFO, "Destroying old bitmap");
		gbitmap_destroy(character_bitmap);
	}

	character_bitmap = gbitmap_create_with_resource(CHARACTER_IMAGES[randnum]); // Draw the colour version of the character
	pickedchar = true;

	GRect character = gbitmap_get_bounds(character_bitmap); // Get the size of the new character bitmap

	layer_set_frame(character_layer, GRect((144 / 2) - (character.size.w / 2), 69 - character.size.h, character.size.w, character.size.h));
	layer_mark_dirty(character_layer);
}

static void bluetooth_handler(bool connected) {
	APP_LOG(APP_LOG_LEVEL_WARNING, "Bluetooth status changed!");
	layer_mark_dirty(hpbar_layer);
}

static void draw_background(Layer *layer, GContext *ctx) {
		graphics_draw_bitmap_in_rect(ctx, background_bitmap, gbitmap_get_bounds(background_bitmap));
}

static void draw_character(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	graphics_draw_bitmap_in_rect(ctx, character_bitmap, layer_get_bounds(character_layer));
}

static void draw_hpbar(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);

	bool connected = bluetooth_connection_service_peek(); // Are we connected to the phone?

	#ifdef PBL_COLOR
		if (!connected) { // If disconnected
			graphics_context_set_fill_color(ctx, GColorRed); // Pick red as fill colour
		} else {
			graphics_context_set_fill_color(ctx, GColorYellow); // Pick yellow as fill colour
		}

		graphics_fill_rect(ctx, layer_get_bounds(hpbar_layer), 0, GCornerNone); // Draw the box
	#elif PBL_BW
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_context_set_stroke_color(ctx, GColorWhite);
		if (!connected) { // If disconnected
			graphics_draw_rect(ctx, layer_get_bounds(hpbar_layer)); // Draw an empty box
		} else {
			graphics_fill_rect(ctx, layer_get_bounds(hpbar_layer), 0, GCornerNone); // Draw a filled box
		}
	#endif
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
	static char temp_buffer[7];
	static char temp_c_buffer[7];

	Tuple *celsius_t = dict_find(iter, MESSAGE_KEY_useCelsius);
	Tuple *temp_t = dict_find(iter, MESSAGE_KEY_KEY_TEMPERATURE);
	Tuple *tempc_t = dict_find(iter, MESSAGE_KEY_KEY_TEMPERATURE_IN_C);

	if (celsius_t) {
		APP_LOG(APP_LOG_LEVEL_INFO, "KEY_USE_CELSIUS_RECEIVED: %d", celsius_t->value->int8);
		use_celsius = celsius_t->value->int8;
		persist_write_int(MESSAGE_KEY_useCelsius, use_celsius);
	}

	if (temp_t) {
		APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE received");
		snprintf(temp_buffer, sizeof(temp_buffer), "LV %d", (int)temp_t->value->int32);
	}

	if (tempc_t) {
		APP_LOG(APP_LOG_LEVEL_INFO, "KEY_TEMPERATURE_IN_C received");
		snprintf(temp_c_buffer, sizeof(temp_c_buffer), "LV %d", (int)tempc_t->value->int32);
	}

	if (use_celsius == 1) {
		APP_LOG(APP_LOG_LEVEL_INFO, "Setting Celsius text");
		text_layer_set_text(love_layer, temp_c_buffer);
	} else {
		APP_LOG(APP_LOG_LEVEL_INFO, "Setting Fahrenheit text");
		text_layer_set_text(love_layer, temp_buffer);
	}
}

static void main_window_load(Window *window) {
	// Get window size
	GRect bounds = layer_get_bounds(window_get_root_layer(window));

	// Draw battle background
	background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(background_layer, draw_background);

	// Create character layer
	character_layer = layer_create(GRect(0, 0, 144, 168)); // In the top corner & fullscreen - we set this to the size of the bitmap later
	layer_set_update_proc(character_layer, draw_character); // Set the update function for the character image
	pick_character(); // Picks a character and sets the size of character_layer

	// Draw HP bar
	hpbar_layer = layer_create(GRect(85, 117, 15, 15)); // Set the size of the hp bar
	GRect hpbar = layer_get_frame(hpbar_layer); // Get the frame of the hp bar
	layer_set_update_proc(hpbar_layer, draw_hpbar); // Set the update function for the hp bar

	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITOP_19)); // Create font for time
	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITOP_12)); // Create smaller font

	// HP text
	hp_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h)); // Create and set frame later
	text_layer_set_background_color(hp_layer, GColorClear);
	text_layer_set_text_color(hp_layer, GColorWhite);
	text_layer_set_font(hp_layer, date_font);
	text_layer_set_text(hp_layer, "HP");
	GSize hptext = text_layer_get_content_size(hp_layer);

	// LV text
	love_layer = text_layer_create(GRect(3, 118, bounds.size.w, bounds.size.h)); // Create and set frame now, since the content does not change
	text_layer_set_background_color(love_layer, GColorClear);
	text_layer_set_text_color(love_layer, GColorWhite);
	text_layer_set_font(love_layer, date_font);
	text_layer_set_text(love_layer, "LV X");

	// Time layer
	time_layer = text_layer_create(GRect(12, 79, bounds.size.w, bounds.size.h)); // Create and set frame
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_text_color(time_layer, GColorWhite);
	text_layer_set_font(time_layer, time_font);

	// Date layer
	date_layer = text_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h)); // Create and set frame later, since the content varies in size
	text_layer_set_background_color(date_layer, GColorClear);
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_font(date_layer, date_font);

	update_time(); // Set the text of the date/time layers in order to get the size of their content

	GSize datesize = text_layer_get_content_size(date_layer); // Get the size of the date layer's content

	layer_set_frame(text_layer_get_layer(date_layer), GRect((144 - (datesize.w + 3)), 118, datesize.w, datesize.h)); // Set the frame of the date layer to always be 3px from the edge of the screen
	GRect date = layer_get_frame(text_layer_get_layer(date_layer)); // Get the frame of the newly positioned date layer

	layer_set_frame(hpbar_layer, GRect((date.origin.x - hpbar.size.w) - 3, 117, 15, 15)); // Set the frame of the hp bar to be a constant 3px from the date
	GRect hpbar_new = layer_get_frame(hpbar_layer); // Get the frame of the newly positioned hp bar

	layer_set_frame(text_layer_get_layer(hp_layer), GRect((hpbar_new.origin.x - hptext.w) - 3, 118, hptext.w, hptext.h)); // Set the frame of the text "HP" to be a constant 3px from the hp bar

	// Add layers as children to main window
	layer_add_child(window_get_root_layer(window), background_layer);
	layer_add_child(window_get_root_layer(window), character_layer);
	layer_add_child(window_get_root_layer(window), hpbar_layer);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(hp_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(love_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

	if (persist_exists(MESSAGE_KEY_useCelsius)) {
		use_celsius = persist_read_int(MESSAGE_KEY_useCelsius);
	} else {
		use_celsius = 0;
	}
}

static void main_window_unload(Window *window) {
	// Destroy the text layers
	text_layer_destroy(time_layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(hp_layer);
	text_layer_destroy(love_layer);

	// Destroy the correct bitmap, depending on platform
	gbitmap_destroy(background_bitmap);
	gbitmap_destroy(character_bitmap);

	// Destroy other layers
	layer_destroy(background_layer);
	layer_destroy(character_layer);

	// Destroy fonts
	fonts_unload_custom_font(time_font);
	fonts_unload_custom_font(date_font);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();

	if (tick_time->tm_min % 60 == 0) { // Pick a new character every hour
			APP_LOG(APP_LOG_LEVEL_INFO, "It's on the hour - updating character and weather");
			pick_character();
	}

	if (tick_time->tm_min % 60 == 0) { // Update the weather every hour
			text_layer_set_text(love_layer, "LV X");

			// Begin dictionary
			DictionaryIterator *iter;
			app_message_outbox_begin(&iter);

			// Add a key-value pair
			dict_write_uint8(iter, 0, 0);

			// Send the message!
			app_message_outbox_send();
	}

	if (testing == true) {
			APP_LOG(APP_LOG_LEVEL_INFO, "Testing mode - updating character");
			pick_character();

			text_layer_set_text(love_layer, "LV X");

			// Begin dictionary
			DictionaryIterator *iter;
			app_message_outbox_begin(&iter);

			// Add a key-value pair
			dict_write_uint8(iter, 0, 0);

			// Send the message!
			app_message_outbox_send();
	}
}


// This stuff does stuff that I don't really understand, check the Pebble developer site for info
static void init() {
	main_window = window_create();

	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_stack_push(main_window, true);
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	bluetooth_connection_service_subscribe(bluetooth_handler);

	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(500, 500);
}

static void deinit() {
	window_destroy(main_window);
	tick_timer_service_unsubscribe();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
