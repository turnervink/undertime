#include <pebble.h>

static Window *main_window;
static Layer *background_layer, *character_layer, *hpbar_layer;
static TextLayer *time_layer, *love_layer, *hp_layer, *date_layer;
static GBitmap *background_bitmap, *character_bitmap;
static GFont time_font, date_font;

int randnum;

const int CHARACTER_IMAGES[] = {
	RESOURCE_ID_IMAGE_SANS,		// 0
	RESOURCE_ID_IMAGE_TORIEL,	// 1
	RESOURCE_ID_IMAGE_PAPYRUS,	// 2
	RESOURCE_ID_IMAGE_UNDYNE,	// 3
	RESOURCE_ID_IMAGE_MTT		// 4
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
	struct tm *tick_time = localtime(&temp);
	
	srand(temp);
	randnum = rand() % 5;
	character_bitmap = gbitmap_create_with_resource(CHARACTER_IMAGES[randnum]);
}

static void bluetooth_handler(bool connected) {
	layer_mark_dirty(hpbar_layer);
}

static void draw_background(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	graphics_draw_bitmap_in_rect(ctx, background_bitmap, gbitmap_get_bounds(background_bitmap));
}

static void draw_character(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);	
	graphics_draw_bitmap_in_rect(ctx, character_bitmap, gbitmap_get_bounds(character_bitmap));
}

static void draw_hpbar(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	
	bool connected = bluetooth_connection_service_peek();
		
	if (!connected) {
		graphics_context_set_fill_color(ctx, GColorRed);
	} else {
		graphics_context_set_fill_color(ctx, GColorYellow);
	}
	
	graphics_fill_rect(ctx, layer_get_bounds(hpbar_layer), 0, GCornerNone);
}

static void main_window_load(Window *window) {
	// Get window size
	GRect bounds = layer_get_bounds(window_get_root_layer(window));
	
	// Draw battle background
	background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTLE);
	background_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(background_layer, draw_background);
	
	// Draw character
	pick_character();
	GRect character = gbitmap_get_bounds(character_bitmap); // Get the size of the character image
	character_layer = layer_create(GRect((bounds.size.w / 2) - (character.size.w / 2), 3, character.size.w, character.size.h)); // Position the character down 3px and in the middle of the screen
	layer_set_update_proc(character_layer, draw_character);
	
	// Draw HP bar
	hpbar_layer = layer_create(GRect(85, 117, 15, 15));
	GRect hpbar = layer_get_frame(hpbar_layer);
	layer_set_update_proc(hpbar_layer, draw_hpbar);
	
	time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITOP_19)); // Create font for time
	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITOP_12));
	
	// HP text
	hp_layer = text_layer_create(GRect(62, 118, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(hp_layer, GColorClear);
	text_layer_set_text_color(hp_layer, GColorWhite);
	text_layer_set_font(hp_layer, date_font);
	text_layer_set_text(hp_layer, "HP");
	
	// LV text
	love_layer = text_layer_create(GRect(4, 118, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(love_layer, GColorClear);
	text_layer_set_text_color(love_layer, GColorWhite);
	text_layer_set_font(love_layer, date_font);
	text_layer_set_text(love_layer, "LV 1");
	
	// Time layer
	time_layer = text_layer_create(GRect(12, 79, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(time_layer, GColorClear);
	text_layer_set_text_color(time_layer, GColorWhite);
	text_layer_set_font(time_layer, time_font);
	
	// Date layer
	date_layer = text_layer_create(GRect((hpbar.origin.x + 15) + 5, 118, bounds.size.w, bounds.size.h));
	text_layer_set_background_color(date_layer, GColorClear);
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_font(date_layer, date_font);
	text_layer_set_text(date_layer, "20/20");
	
	
	// Add children
	layer_add_child(window_get_root_layer(window), background_layer);
	layer_add_child(window_get_root_layer(window), character_layer);
	layer_add_child(window_get_root_layer(window), hpbar_layer);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(hp_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(love_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
	
	update_time();
}

static void main_window_unload(Window *window) {
	
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	if (tick_time->tm_min % 30 == 0) {
		APP_LOG(APP_LOG_LEVEL_INFO, "It's on the hour");
		pick_character();
	}
}


static void init() {
	main_window = window_create();

	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	pick_character();

	window_stack_push(main_window, true);
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	bluetooth_connection_service_subscribe(bluetooth_handler);
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