/*
OBS CRT Filter
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-source.h>
#include <obs.h>
#include <plugin-support.h>

#define SETTING_LOOK "look"
#define SETTING_INTENSITY "intensity"
#define SETTING_SCANLINES "scanlines"
#define SETTING_SCANLINE_AMOUNT "scanline_amount"
#define SETTING_CURVATURE "curvature"
#define SETTING_CURVATURE_AMOUNT "curvature_amount"
#define SETTING_GLOW "glow"
#define SETTING_GLOW_AMOUNT "glow_amount"
#define SETTING_COLOR_BLEED "color_bleed"
#define SETTING_COLOR_BLEED_AMOUNT "color_bleed_amount"
#define SETTING_VIGNETTE "vignette"
#define SETTING_VIGNETTE_AMOUNT "vignette_amount"
#define SETTING_FLICKER "flicker"
#define SETTING_FLICKER_AMOUNT "flicker_amount"

#define TEXT_LOOK obs_module_text("CRT.Look")
#define TEXT_LOOK_CONSUMER obs_module_text("CRT.Look.Consumer")
#define TEXT_LOOK_ARCADE obs_module_text("CRT.Look.Arcade")
#define TEXT_LOOK_PVM obs_module_text("CRT.Look.PVM")
#define TEXT_INTENSITY obs_module_text("CRT.Intensity")
#define TEXT_SCANLINES obs_module_text("CRT.Scanlines")
#define TEXT_SCANLINE_AMOUNT obs_module_text("CRT.ScanlineAmount")
#define TEXT_CURVATURE obs_module_text("CRT.Curvature")
#define TEXT_CURVATURE_AMOUNT obs_module_text("CRT.CurvatureAmount")
#define TEXT_GLOW obs_module_text("CRT.Glow")
#define TEXT_GLOW_AMOUNT obs_module_text("CRT.GlowAmount")
#define TEXT_COLOR_BLEED obs_module_text("CRT.ColorBleed")
#define TEXT_COLOR_BLEED_AMOUNT obs_module_text("CRT.ColorBleedAmount")
#define TEXT_VIGNETTE obs_module_text("CRT.Vignette")
#define TEXT_VIGNETTE_AMOUNT obs_module_text("CRT.VignetteAmount")
#define TEXT_FLICKER obs_module_text("CRT.Flicker")
#define TEXT_FLICKER_AMOUNT obs_module_text("CRT.FlickerAmount")

enum crt_look {
	CRT_LOOK_CONSUMER = 0,
	CRT_LOOK_ARCADE = 1,
	CRT_LOOK_PVM = 2,
};

struct crt_filter_data {
	obs_source_t *context;

	gs_effect_t *effect;
	gs_eparam_t *look_param;
	gs_eparam_t *intensity_param;
	gs_eparam_t *scanlines_param;
	gs_eparam_t *scanline_amount_param;
	gs_eparam_t *curvature_param;
	gs_eparam_t *curvature_amount_param;
	gs_eparam_t *glow_param;
	gs_eparam_t *glow_amount_param;
	gs_eparam_t *color_bleed_param;
	gs_eparam_t *color_bleed_amount_param;
	gs_eparam_t *vignette_param;
	gs_eparam_t *vignette_amount_param;
	gs_eparam_t *flicker_param;
	gs_eparam_t *flicker_amount_param;
	gs_eparam_t *texture_width_param;
	gs_eparam_t *texture_height_param;
	gs_eparam_t *time_param;

	enum crt_look look;
	float intensity;
	bool scanlines;
	float scanline_amount;
	bool curvature;
	float curvature_amount;
	bool glow;
	float glow_amount;
	bool color_bleed;
	float color_bleed_amount;
	bool vignette;
	float vignette_amount;
	bool flicker;
	float flicker_amount;
	float elapsed_time;
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static const char *crt_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CRT.FilterName");
}

static float get_percent_setting(obs_data_t *settings, const char *name, int fallback)
{
	const int value = (obs_data_has_user_value(settings, name) || obs_data_has_default_value(settings, name))
				  ? (int)obs_data_get_int(settings, name)
				  : fallback;
	return (float)value * 0.01f;
}

static void crt_filter_update(void *data, obs_data_t *settings)
{
	struct crt_filter_data *filter = data;

	filter->look = (enum crt_look)obs_data_get_int(settings, SETTING_LOOK);
	filter->intensity = (float)obs_data_get_int(settings, SETTING_INTENSITY) * 0.01f;
	filter->scanlines = obs_data_get_bool(settings, SETTING_SCANLINES);
	filter->scanline_amount = get_percent_setting(settings, SETTING_SCANLINE_AMOUNT, 70);
	filter->curvature = obs_data_get_bool(settings, SETTING_CURVATURE);
	filter->curvature_amount = get_percent_setting(settings, SETTING_CURVATURE_AMOUNT, 60);
	filter->glow = obs_data_get_bool(settings, SETTING_GLOW);
	filter->glow_amount = get_percent_setting(settings, SETTING_GLOW_AMOUNT, 65);
	filter->color_bleed = obs_data_get_bool(settings, SETTING_COLOR_BLEED);
	filter->color_bleed_amount = get_percent_setting(settings, SETTING_COLOR_BLEED_AMOUNT, 60);
	filter->vignette = obs_data_get_bool(settings, SETTING_VIGNETTE);
	filter->vignette_amount = get_percent_setting(settings, SETTING_VIGNETTE_AMOUNT, 55);
	filter->flicker = obs_data_get_bool(settings, SETTING_FLICKER);
	filter->flicker_amount = get_percent_setting(settings, SETTING_FLICKER_AMOUNT, 45);
}

static void crt_filter_destroy(void *data)
{
	struct crt_filter_data *filter = data;

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(filter);
}

static void *crt_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct crt_filter_data *filter = bzalloc(sizeof(*filter));
	char *effect_path = obs_module_file("effects/crt.effect");

	filter->context = context;

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	if (filter->effect) {
		filter->look_param = gs_effect_get_param_by_name(filter->effect, "look");
		filter->intensity_param = gs_effect_get_param_by_name(filter->effect, "intensity");
		filter->scanlines_param = gs_effect_get_param_by_name(filter->effect, "scanlines");
		filter->scanline_amount_param = gs_effect_get_param_by_name(filter->effect, "scanline_amount");
		filter->curvature_param = gs_effect_get_param_by_name(filter->effect, "curvature");
		filter->curvature_amount_param = gs_effect_get_param_by_name(filter->effect, "curvature_amount");
		filter->glow_param = gs_effect_get_param_by_name(filter->effect, "glow");
		filter->glow_amount_param = gs_effect_get_param_by_name(filter->effect, "glow_amount");
		filter->color_bleed_param = gs_effect_get_param_by_name(filter->effect, "color_bleed");
		filter->color_bleed_amount_param = gs_effect_get_param_by_name(filter->effect, "color_bleed_amount");
		filter->vignette_param = gs_effect_get_param_by_name(filter->effect, "vignette");
		filter->vignette_amount_param = gs_effect_get_param_by_name(filter->effect, "vignette_amount");
		filter->flicker_param = gs_effect_get_param_by_name(filter->effect, "flicker");
		filter->flicker_amount_param = gs_effect_get_param_by_name(filter->effect, "flicker_amount");
		filter->texture_width_param = gs_effect_get_param_by_name(filter->effect, "texture_width");
		filter->texture_height_param = gs_effect_get_param_by_name(filter->effect, "texture_height");
		filter->time_param = gs_effect_get_param_by_name(filter->effect, "elapsed_time");
	}
	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		crt_filter_destroy(filter);
		return NULL;
	}

	crt_filter_update(filter, settings);
	return filter;
}

static void crt_filter_video_tick(void *data, float seconds)
{
	struct crt_filter_data *filter = data;
	filter->elapsed_time += seconds;

	if (filter->elapsed_time > 3600.0f)
		filter->elapsed_time = 0.0f;
}

static void crt_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct crt_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

	if (!target) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space =
		obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	if (source_space == GS_CS_709_EXTENDED) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_format format = gs_get_format_from_space(source_space);
	if (!obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
							     OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	const float width = (float)obs_source_get_width(target);
	const float height = (float)obs_source_get_height(target);

	gs_effect_set_int(filter->look_param, (int)filter->look);
	gs_effect_set_float(filter->intensity_param, filter->intensity);
	gs_effect_set_float(filter->scanlines_param, filter->scanlines ? 1.0f : 0.0f);
	gs_effect_set_float(filter->scanline_amount_param, filter->scanline_amount);
	gs_effect_set_float(filter->curvature_param, filter->curvature ? 1.0f : 0.0f);
	gs_effect_set_float(filter->curvature_amount_param, filter->curvature_amount);
	gs_effect_set_float(filter->glow_param, filter->glow ? 1.0f : 0.0f);
	gs_effect_set_float(filter->glow_amount_param, filter->glow_amount);
	gs_effect_set_float(filter->color_bleed_param, filter->color_bleed ? 1.0f : 0.0f);
	gs_effect_set_float(filter->color_bleed_amount_param, filter->color_bleed_amount);
	gs_effect_set_float(filter->vignette_param, filter->vignette ? 1.0f : 0.0f);
	gs_effect_set_float(filter->vignette_amount_param, filter->vignette_amount);
	gs_effect_set_float(filter->flicker_param, filter->flicker ? 1.0f : 0.0f);
	gs_effect_set_float(filter->flicker_amount_param, filter->flicker_amount);
	gs_effect_set_float(filter->texture_width_param, width);
	gs_effect_set_float(filter->texture_height_param, height);
	gs_effect_set_float(filter->time_param, filter->elapsed_time);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	obs_source_process_filter_end(filter->context, filter->effect, 0, 0);

	gs_blend_state_pop();
}

static obs_properties_t *crt_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *look = obs_properties_add_list(props, SETTING_LOOK, TEXT_LOOK, OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(look, TEXT_LOOK_CONSUMER, CRT_LOOK_CONSUMER);
	obs_property_list_add_int(look, TEXT_LOOK_ARCADE, CRT_LOOK_ARCADE);
	obs_property_list_add_int(look, TEXT_LOOK_PVM, CRT_LOOK_PVM);

	obs_properties_add_int_slider(props, SETTING_INTENSITY, TEXT_INTENSITY, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_SCANLINES, TEXT_SCANLINES);
	obs_properties_add_int_slider(props, SETTING_SCANLINE_AMOUNT, TEXT_SCANLINE_AMOUNT, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_CURVATURE, TEXT_CURVATURE);
	obs_properties_add_int_slider(props, SETTING_CURVATURE_AMOUNT, TEXT_CURVATURE_AMOUNT, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_GLOW, TEXT_GLOW);
	obs_properties_add_int_slider(props, SETTING_GLOW_AMOUNT, TEXT_GLOW_AMOUNT, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_COLOR_BLEED, TEXT_COLOR_BLEED);
	obs_properties_add_int_slider(props, SETTING_COLOR_BLEED_AMOUNT, TEXT_COLOR_BLEED_AMOUNT, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_VIGNETTE, TEXT_VIGNETTE);
	obs_properties_add_int_slider(props, SETTING_VIGNETTE_AMOUNT, TEXT_VIGNETTE_AMOUNT, 0, 100, 1);
	obs_properties_add_bool(props, SETTING_FLICKER, TEXT_FLICKER);
	obs_properties_add_int_slider(props, SETTING_FLICKER_AMOUNT, TEXT_FLICKER_AMOUNT, 0, 100, 1);

	UNUSED_PARAMETER(data);
	return props;
}

static void crt_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_LOOK, CRT_LOOK_CONSUMER);
	obs_data_set_default_int(settings, SETTING_INTENSITY, 65);
	obs_data_set_default_bool(settings, SETTING_SCANLINES, true);
	obs_data_set_default_int(settings, SETTING_SCANLINE_AMOUNT, 70);
	obs_data_set_default_bool(settings, SETTING_CURVATURE, true);
	obs_data_set_default_int(settings, SETTING_CURVATURE_AMOUNT, 60);
	obs_data_set_default_bool(settings, SETTING_GLOW, true);
	obs_data_set_default_int(settings, SETTING_GLOW_AMOUNT, 65);
	obs_data_set_default_bool(settings, SETTING_COLOR_BLEED, true);
	obs_data_set_default_int(settings, SETTING_COLOR_BLEED_AMOUNT, 60);
	obs_data_set_default_bool(settings, SETTING_VIGNETTE, true);
	obs_data_set_default_int(settings, SETTING_VIGNETTE_AMOUNT, 55);
	obs_data_set_default_bool(settings, SETTING_FLICKER, false);
	obs_data_set_default_int(settings, SETTING_FLICKER_AMOUNT, 45);
}

static enum gs_color_space crt_filter_get_color_space(void *data, size_t count,
						      const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);

	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct crt_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);

	if (!target)
		return GS_CS_SRGB;

	return obs_source_get_color_space(target, OBS_COUNTOF(potential_spaces), potential_spaces);
}

static struct obs_source_info crt_filter_info = {
	.id = "obs_crt_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = crt_filter_get_name,
	.create = crt_filter_create,
	.destroy = crt_filter_destroy,
	.update = crt_filter_update,
	.video_tick = crt_filter_video_tick,
	.video_render = crt_filter_video_render,
	.get_properties = crt_filter_properties,
	.get_defaults = crt_filter_defaults,
	.video_get_color_space = crt_filter_get_color_space,
};

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Simple CRT TV effect filter for OBS Studio.";
}

bool obs_module_load(void)
{
	obs_register_source(&crt_filter_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
