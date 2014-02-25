#include <pebble.h>
#include "TransRotBmp.h"

static const GPathInfo SECOND_HAND_POINTS =
{
	7, (GPoint []) { { 0, 20 }, { -4, 15 }, { -2, 0 }, { -2, -72 }, { 2, -72 }, { 2, 0 }, { 4, 15 } } 
};

Window *window;
static GBitmap *background;
BitmapLayer *background_layer;
Layer *date_layer, *secs_layer;
TransRotBmp *minute_layer, *hour_layer;
static GPath *second_arrow;
char ddBuffer[] = "00";
static GFont digitS;

//-----------------------------------------------------------------------------------------------------------------------
static void date_update_proc(Layer *layer, GContext *ctx) 
{
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_context_set_text_color(ctx, GColorBlack);
	
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	snprintf(ddBuffer, sizeof(ddBuffer), "%d", t->tm_mday < 10 ? t->tm_mday+20 : t->tm_mday);
	GSize txtSize = graphics_text_layout_get_content_size(ddBuffer, digitS, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);

	GRect rcDraw = GRect(center.x-txtSize.w/2, center.y-txtSize.h/2-3, txtSize.w, txtSize.h);
	
	snprintf(ddBuffer, sizeof(ddBuffer), "%d", t->tm_mday);
	graphics_draw_text(ctx, ddBuffer, digitS, rcDraw, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

	rcDraw.origin.x -= 3;
	rcDraw.origin.y += 2;
	rcDraw.size.w += 6;
	rcDraw.size.h += 3;
	graphics_draw_round_rect(ctx, rcDraw, 5);
	
	/*
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Rect: x:%d, y:%d, w:%d, h:%d",
		rcDraw.origin.x, rcDraw.origin.y, rcDraw.size.w, rcDraw.size.h);
	*/
}
//-----------------------------------------------------------------------------------------------------------------------
static void secs_update_proc(Layer *layer, GContext *ctx) 
{
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	gpath_rotate_to(second_arrow, TRIG_MAX_ANGLE * t->tm_sec / 60);
	gpath_move_to(second_arrow, center);
	gpath_draw_filled(ctx, second_arrow);
	gpath_draw_outline(ctx, second_arrow);
	
	graphics_fill_circle(ctx, center, 5);
	graphics_draw_circle(ctx, center, 5);
}
//-----------------------------------------------------------------------------------------------------------------------
void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	if (units_changed == MINUTE_UNIT || tick_time->tm_sec == 0)
	{
		if (units_changed == MINUTE_UNIT || (tick_time->tm_hour == 0 && tick_time->tm_min == 0))
			layer_mark_dirty(date_layer);

		int32_t angle = (TRIG_MAX_ANGLE * (((tick_time->tm_hour % 12) * 6) + (tick_time->tm_min / 10)) / (12 * 6));
		transrotbmp_set_angle(hour_layer, angle);
		angle = (TRIG_MAX_ANGLE * ((tick_time->tm_min * 6) + (tick_time->tm_sec / 10)) / (60 * 6));
		transrotbmp_set_angle(minute_layer, angle);
	}
	layer_mark_dirty(secs_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_init(void) 
{
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	//Init Background
	background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(background_layer, background);
	layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
	
	digitS = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DATE_20));
	date_layer = layer_create(GRect(28, 88, 33, 23));
	layer_set_update_proc(date_layer, date_update_proc);
	layer_add_child(window_layer, date_layer);	

	minute_layer = transrotbmp_create_with_resource_prefix(RESOURCE_ID_IMAGE_MINUTE_HAND);
	transrotbmp_set_pos_centered(minute_layer, -31, -39);
	transrotbmp_set_src_ic(minute_layer, GPoint(8, 71));
	transrotbmp_add_to_layer(minute_layer, window_layer);

	hour_layer = transrotbmp_create_with_resource_prefix(RESOURCE_ID_IMAGE_HOUR_HAND);
	transrotbmp_set_pos_centered(hour_layer, -19, -27);
	transrotbmp_set_src_ic(hour_layer, GPoint(9, 48));
	transrotbmp_add_to_layer(hour_layer, window_layer);
	
	secs_layer = layer_create(GRect(0, 4, bounds.size.w, bounds.size.w+1));
	layer_set_update_proc(secs_layer, secs_update_proc);
	second_arrow = gpath_create(&SECOND_HAND_POINTS);
	layer_add_child(window_layer, secs_layer);	

	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	tick_handler(t, MINUTE_UNIT);

	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
}
//-----------------------------------------------------------------------------------------------------------------------
void handle_deinit(void) 
{
	tick_timer_service_unsubscribe();

	layer_destroy(date_layer);
	fonts_unload_custom_font(digitS);
	layer_destroy(secs_layer);
	gpath_destroy(second_arrow);
	bitmap_layer_destroy(background_layer);
	gbitmap_destroy(background);
	transrotbmp_destroy(minute_layer);
	transrotbmp_destroy(hour_layer);
	window_destroy(window);
}
//-----------------------------------------------------------------------------------------------------------------------
int main(void) 
{
	handle_init();
	app_event_loop();
	handle_deinit();
}
//-----------------------------------------------------------------------------------------------------------------------