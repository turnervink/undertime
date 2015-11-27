#include <pebble.h>

static Window *main_window;
static Layer *background_layer, *character_layer, *hpbar_layer;
static TextLayer *time_layer, *love_layer, *hp_layer, *date_layer;
static GBitmap *background_bitmap; 
#ifdef PBL_PLATFORM_BASALT
	static GBitmap *character_bitmap; 
#elif PBL_PLATFORM_APLITE
	static GBitmap *character_bitmap_b, *character_bitmap_w;
#endif
static GFont time_font, date_font;

int randnum; // Random number to pick character with
int totchars = 8; // Total number of characters

#ifdef PBL_PLATFORM_BASALT // Colour character images (not really colour, just antialiased)
	const int CHARACTER_IMAGES_COLOUR[] = {
		RESOURCE_ID_IMAGE_SANS,				// 0
		RESOURCE_ID_IMAGE_PAPYRUS,		// 1
		RESOURCE_ID_IMAGE_MTT,				// 2
		RESOURCE_ID_IMAGE_TORIEL,			// 3
		RESOURCE_ID_IMAGE_UNDYNE,			// 4
		RESOURCE_ID_IMAGE_ALPHYS,			// 5
		RESOURCE_ID_IMAGE_NAPSTA,			// 6
		RESOURCE_ID_IMAGE_TEM					// 7
	};
#endif

#ifdef PBL_PLATFORM_APLITE // Black and white character images
	const int CHARACTER_IMAGES_WHITE[] = {
		RESOURCE_ID_IMAGE_SANS_BW_WHITE,			// 0
		RESOURCE_ID_IMAGE_PAPYRUS_BW_WHITE,		// 1
		RESOURCE_ID_IMAGE_MTT_BW_WHITE,				// 2
		RESOURCE_ID_IMAGE_TORIEL_BW_WHITE,		// 3
		RESOURCE_ID_IMAGE_UNDYNE_BW_WHITE,		// 4
		RESOURCE_ID_IMAGE_ALPHYS_BW_WHITE,		// 5
		RESOURCE_ID_IMAGE_NAPSTA_BW_WHITE,		// 6
		RESOURCE_ID_IMAGE_TEM_BW_WHITE				// 7
	};

	const int CHARACTER_IMAGES_BLACK[] = {
		RESOURCE_ID_IMAGE_SANS_BW_BLACK,			// 0
		RESOURCE_ID_IMAGE_PAPYRUS_BW_BLACK,		// 1
		RESOURCE_ID_IMAGE_MTT_BW_BLACK,				// 2
		RESOURCE_ID_IMAGE_TORIEL_BW_BLACK,		// 3
		RESOURCE_ID_IMAGE_UNDYNE_BW_BLACK,		// 4
		RESOURCE_ID_IMAGE_ALPHYS_BW_BLACK,		// 5
		RESOURCE_ID_IMAGE_NAPSTA_BW_BLACK,		// 6
		RESOURCE_ID_IMAGE_TEM_BW_BLACK				// 7
	};
#endif

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
	
	srand(temp); // Seed our random number using the current time
	randnum = rand() % totchars; // Pick a random number between 0 and the amount of available characters
	APP_LOG(APP_LOG_LEVEL_INFO, "Picked character: %d", randnum);
	
	#ifdef PBL_PLATFORM_BASALT
		character_bitmap = gbitmap_create_with_resource(CHARACTER_IMAGES_COLOUR[randnum]); // Draw the colour version of the character
	#elif PBL_PLATFORM_APLITE
		character_bitmap_b = gbitmap_create_with_resource(CHARACTER_IMAGES_BLACK[randnum]); // Draw the black image of the character
		character_bitmap_w = gbitmap_create_with_resource(CHARACTER_IMAGES_WHITE[randnum]); // Draw the white image of the character
	#endif
}

static void bluetooth_handler(bool connected) {
	APP_LOG(APP_LOG_LEVEL_WARNING, "Bluetooth status changed!");
	layer_mark_dirty(hpbar_layer);
}

static void draw_background(Layer *layer, GContext *ctx) {
	#ifdef PBL_PLATFORM_BASALT
		graphics_context_set_compositing_mode(ctx, GCompOpSet);
		graphics_draw_bitmap_in_rect(ctx, background_bitmap, gbitmap_get_bounds(background_bitmap));
	#elif PBL_PLATFORM_APLITE
		graphics_draw_bitmap_in_rect(ctx, background_bitmap, gbitmap_get_bounds(background_bitmap));
	#endif
}

static void draw_character(Layer *layer, GContext *ctx) {
	#ifdef PBL_PLATFORM_BASALT
		graphics_context_set_compositing_mode(ctx, GCompOpSet);	
		graphics_draw_bitmap_in_rect(ctx, character_bitmap, gbitmap_get_bounds(character_bitmap));
	#elif PBL_PLATFORM_APLITE
		graphics_context_set_compositing_mode(ctx, GCompOpOr);
		graphics_draw_bitmap_in_rect(ctx, character_bitmap_w, gbitmap_get_bounds(character_bitmap_w));
	
		graphics_context_set_compositing_mode(ctx, GCompOpClear);
		graphics_draw_bitmap_in_rect(ctx, character_bitmap_b, gbitmap_get_bounds(character_bitmap_b));
	#endif
}

static void draw_hpbar(Layer *layer, GContext *ctx) {
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	
	bool connected = bluetooth_connection_service_peek(); // Are we connected to the phone?
		
	#ifdef PBL_PLATFORM_BASALT
		if (!connected) { // If disconnected
			graphics_context_set_fill_color(ctx, GColorRed); // Pick red as fill colour
		} else {
			graphics_context_set_fill_color(ctx, GColorYellow); // Pick yellow as fill colour
		}

		graphics_fill_rect(ctx, layer_get_bounds(hpbar_layer), 0, GCornerNone); // Draw the box
	#elif PBL_PLATFORM_APLITE
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_context_set_stroke_color(ctx, GColorWhite);
		if (!connected) { // If disconnected
			graphics_draw_rect(ctx, layer_get_bounds(hpbar_layer)); // Draw an empty box
		} else {
			graphics_fill_rect(ctx, layer_get_bounds(hpbar_layer), 0, GCornerNone); // Draw a filled box
		}
	#endif
}

static void main_window_load(Window *window) {
	// Get window size
	GRect bounds = layer_get_bounds(window_get_root_layer(window));
	
	// Draw battle background
	#ifdef PBL_PLATFORM_BASALT
		background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTLE);
	#elif PBL_PLATFORM_APLITE
		background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTLE_BW);
	#endif
	background_layer = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(background_layer, draw_background);
	
	// Draw character
	pick_character();
	#ifdef PBL_PLATFORM_BASALT
		GRect character = gbitmap_get_bounds(character_bitmap); // Get the size of the colour character image
	#elif PBL_PLATFORM_APLITE
		GRect character = gbitmap_get_bounds(character_bitmap_b); // Get the size of the bw character image
	#endif
	character_layer = layer_create(GRect((bounds.size.w / 2) - (character.size.w / 2), 3, character.size.w, character.size.h)); // Position the character down 3px and in the middle of the screen
	layer_set_update_proc(character_layer, draw_character); // Set the update function for the character image
	
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
	text_layer_set_text(love_layer, "LV 1");
	
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
}

static void main_window_unload(Window *window) {
	text_layer_destroy(time_layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(hp_layer);
	text_layer_destroy(love_layer);
	
	gbitmap_destroy(background_bitmap);
	#ifdef PBL_PLATFORM_BASALT
		gbitmap_destroy(character_bitmap);
	#elif PBL_PLATFORM_APLITE
		gbitmap_destroy(character_bitmap_b);
		gbitmap_destroy(character_bitmap_w);
	#endif
	
	layer_destroy(background_layer);
	layer_destroy(character_layer);
	
	fonts_unload_custom_font(time_font);
	fonts_unload_custom_font(date_font);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time();
	
	if (tick_time->tm_min % 60 == 0) { // Pick a new character every hour
		APP_LOG(APP_LOG_LEVEL_INFO, "It's on the hour - updating character");
		pick_character();
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