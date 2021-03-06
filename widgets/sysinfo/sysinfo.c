/*
 * Copyright (C) 2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>
#ifndef USE_BOARD_NATIVE
  #include "hal.h"
#endif
#include "log.h"
#include "lvgl.h"
#include "gui.h"
#include "gui/theme.h"
#include "controller.h"
#include "kernel_defines.h"
#ifndef USE_BOARD_NATIVE
  #include "bleman.h"
#endif
#include "xtimer.h"
#include "gui.h"

#include "sysinfo.h"

#define SYSINFO_SCROLL_DISTANCE 180

static const widget_spec_t _sysinfo_spec;
static int sysinfo_event(widget_t *widget, controller_event_t event);

#define LABEL_LABEL_COLOR   "1D2951 "

static const char *sysinfo_on_battery = "On battery";
static const char *sysinfo_charging = "Charging";
static const char *sysinfo_charging_complete = "Fully charged";

#ifndef USE_BOARD_NATIVE
static const char *sysinfo_reset_reason[] = {
    [0] = "Reset pin",
    [1] = "Watchdog",
    [2] = "Soft reset",
    [3] = "CPU Lockup",
    [16] = "GPIO Triggered",
    [17] = "LP Compare",
    [18] = "Debugger",
    [19] = "NFC",
};
#endif

static const char *sysinfo_label = ""
    "#" LABEL_LABEL_COLOR "RIOT version:# \n %.14s\n"
    "#" LABEL_LABEL_COLOR "App version:# \n " PINETIME_VERSION "\n"
    "#" LABEL_LABEL_COLOR "Uptime:# \n %uh %02um %02us\n"
    "#" LABEL_LABEL_COLOR "Battery:# \n %umV\n"
    "#" LABEL_LABEL_COLOR "State:# \n %s\n"
    "#" LABEL_LABEL_COLOR "Reboot Reason:# \n %s\n"
    "#" LABEL_LABEL_COLOR "Temperature:# \n %" PRIu32"\n";

/* Widget context */
sysinfo_widget_t sysinfo_widget = {
    .widget = {.spec = &_sysinfo_spec }
};

static inline sysinfo_widget_t *_from_widget(widget_t *widget)
{
    return container_of(widget, sysinfo_widget_t, widget);
}

/* @@ unused?
static inline sysinfo_widget_t *active_widget(void)
{
    return &sysinfo_widget;
}
*/

static void _sysinfo_set_label(sysinfo_widget_t *sysinfo)
{
    unsigned hours = sysinfo->uptime / 3600;
    unsigned minutes = (sysinfo->uptime / 60) % 60;
    unsigned seconds = sysinfo->uptime % 60;
    const char *power_state = sysinfo->powered ?
        (sysinfo->charging ? sysinfo_charging : sysinfo_charging_complete)
        : sysinfo_on_battery;
    lv_label_set_text_fmt(sysinfo->info_label,
                          sysinfo_label,
                          RIOT_VERSION,
                          hours, minutes, seconds,
                          sysinfo->battery_voltage,
                          power_state,
                          sysinfo->reset_string,
                          sysinfo->temperature/4);
}

static lv_obj_t *_sysinfo_screen_create(sysinfo_widget_t *sysinfo)
{
    /* Show:
     * Uptime
     * Reboot reason
     * RIOT version
     * PineTime commit version
     * Battery voltage
     * Temperature
     * Disk stats
     */
    lv_obj_t *scr = lv_obj_create(NULL, NULL);
    sysinfo->screen = scr;

    lv_obj_t *page = lv_page_create(scr, NULL);
    lv_obj_set_size(page, 240, 240);
    lv_obj_align(page, NULL, LV_ALIGN_CENTER, 0, 0);
    sysinfo->page = page;

    sysinfo->info_label = lv_label_create(page, NULL);
    lv_obj_set_width(sysinfo->info_label, lv_page_get_fit_width(page));
    lv_label_set_long_mode(sysinfo->info_label, LV_LABEL_LONG_EXPAND);
    lv_label_set_recolor(sysinfo->info_label, true);

    _sysinfo_set_label(sysinfo);

    return scr;
}

static void _update_power_stats(sysinfo_widget_t *sysinfo)
{
#ifndef USE_BOARD_NATIVE
    sysinfo->powered = hal_battery_is_powered();
    sysinfo->charging = hal_battery_is_charging();
#endif
    sysinfo->battery_voltage = controller_get_battery_voltage(controller_get());
}

static int sysinfo_init(widget_t *widget)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    (void)sysinfo;
    widget_init_local(widget);
    sysinfo->handler.events = CONTROLLER_EVENT_FLAG(CONTROLLER_EVENT_TICK);
    sysinfo->handler.widget = widget;
    controller_add_control_handler(controller_get(), &sysinfo->handler);
    return 0;
}

static int sysinfo_launch(widget_t *widget)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    (void)sysinfo;
    sysinfo_event(widget, CONTROLLER_EVENT_TICK);
    return 0;
}

static int sysinfo_draw(widget_t *widget, lv_obj_t *parent)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    sysinfo->screen = _sysinfo_screen_create(sysinfo);
    return 0;
}

static int sysinfo_update_screen(widget_t *widget)
{
    if (widget_get_gui_lock(widget) == 0) {
        return 0;
    }
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    _sysinfo_set_label(sysinfo);
    widget_release_gui_lock(widget);
    return 1;
}

static lv_obj_t *sysinfo_get_container(widget_t *widget)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    return sysinfo->screen;
}

static int sysinfo_close(widget_t *widget)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    lv_obj_del(sysinfo->screen);
    return 0;
}

static int sysinfo_event(widget_t *widget, controller_event_t event)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    widget_get_control_lock(widget);
    if (event == CONTROLLER_EVENT_TICK) {
        sysinfo->uptime = xtimer_now_usec64() / US_PER_SEC;
        _update_power_stats(sysinfo);
#ifndef USE_BOARD_NATIVE
        sysinfo->reset_string = sysinfo_reset_reason[controller_get()->reset_reason];
#endif
        sysinfo->reset_string = "N/A";
#ifndef USE_BOARD_NATIVE
        sysinfo->temperature = hal_get_internal_temp();
#endif
    }
    widget_release_control_lock(widget);
    return 0;
}

static int sysinfo_gui_event(widget_t *widget, int event)
{
    sysinfo_widget_t *sysinfo = _from_widget(widget);
    switch (event) {
#ifndef USE_BOARD_NATIVE
        case GUI_EVENT_GESTURE_UP:
            lv_page_scroll_ver(sysinfo->page, -SYSINFO_SCROLL_DISTANCE);
            break;
        case GUI_EVENT_GESTURE_DOWN:
            lv_page_scroll_ver(sysinfo->page, SYSINFO_SCROLL_DISTANCE);
            break;
#endif
        case GUI_EVENT_GESTURE_LEFT:
            controller_action_submit_input_action(&sysinfo->widget,
                                                  CONTROLLER_ACTION_WIDGET_LEAVE, NULL);
            break;
        case GUI_EVENT_GESTURE_RIGHT:
            break;
    }

    return 0;
}

static const widget_spec_t _sysinfo_spec = {
    .name = "system info",
    .label = "system info",
    .init = sysinfo_init,
    .draw = sysinfo_draw,
    .launch = sysinfo_launch,
    .container = sysinfo_get_container,
    .close = sysinfo_close,
    .event = sysinfo_event,
    .gui_event = sysinfo_gui_event,
    .update_draw = sysinfo_update_screen,
};
