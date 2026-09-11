#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <stdio.h>

#define VIEW_MENU 0
#define VIEW_OFFER 1
#define VIEW_RESULT 2
#define VIEW_SHIFT 3

#define PAY_MIN_CENTS 100
#define PAY_STEP_CENTS 25
#define PAY_MAX_CENTS 5000
#define MILES_MIN_TENTHS 1
#define MILES_STEP_TENTHS 1
#define MILES_MAX_TENTHS 250

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    VariableItemList* offer_list;
    Widget* result_widget;
    Widget* shift_widget;
    uint32_t pay_cents;
    uint32_t miles_tenths;
    bool shift_active;
    uint32_t shift_start_tick;
    uint32_t orders;
    uint32_t earnings_cents;
    uint32_t shift_miles_tenths;
} DashMateApp;

enum DashMateMenuItem {
    DashMateMenuStartShift,
    DashMateMenuNewOffer,
    DashMateMenuEarnings,
    DashMateMenuShiftStats,
    DashMateMenuResetShift,
    DashMateMenuAbout,
};

static uint32_t dashmate_to_menu_callback(void* context) {
    UNUSED(context);
    return VIEW_MENU;
}

static void dashmate_pay_changed(VariableItem* item) {
    DashMateApp* app = variable_item_get_context(item);
    app->pay_cents = PAY_MIN_CENTS + variable_item_get_current_value_index(item) * PAY_STEP_CENTS;
    char text[16];
    snprintf(text, sizeof(text), "$%lu.%02lu", (unsigned long)(app->pay_cents / 100), (unsigned long)(app->pay_cents % 100));
    variable_item_set_current_value_text(item, text);
}

static void dashmate_miles_changed(VariableItem* item) {
    DashMateApp* app = variable_item_get_context(item);
    app->miles_tenths = MILES_MIN_TENTHS + variable_item_get_current_value_index(item) * MILES_STEP_TENTHS;
    char text[16];
    snprintf(text, sizeof(text), "%lu.%lu mi", (unsigned long)(app->miles_tenths / 10), (unsigned long)(app->miles_tenths % 10));
    variable_item_set_current_value_text(item, text);
}

static void dashmate_show_result(DashMateApp* app) {
    widget_reset(app->result_widget);
    uint32_t cents_per_mile = (app->pay_cents * 10) / app->miles_tenths;
    const char* rating;
    if(cents_per_mile < 100) rating = "SKIP";
    else if(cents_per_mile < 150) rating = "LOW";
    else if(cents_per_mile < 200) rating = "OK";
    else rating = "GOOD";

    char pay_text[32];
    char miles_text[32];
    char rate_text[32];
    snprintf(pay_text, sizeof(pay_text), "Pay: $%lu.%02lu", (unsigned long)(app->pay_cents / 100), (unsigned long)(app->pay_cents % 100));
    snprintf(miles_text, sizeof(miles_text), "Miles: %lu.%lu", (unsigned long)(app->miles_tenths / 10), (unsigned long)(app->miles_tenths % 10));
    snprintf(rate_text, sizeof(rate_text), "$/mile: $%lu.%02lu", (unsigned long)(cents_per_mile / 100), (unsigned long)(cents_per_mile % 100));

    widget_add_string_element(app->result_widget, 64, 8, AlignCenter, AlignCenter, FontPrimary, "OFFER CHECK");
    widget_add_string_element(app->result_widget, 8, 24, AlignLeft, AlignCenter, FontSecondary, pay_text);
    widget_add_string_element(app->result_widget, 8, 35, AlignLeft, AlignCenter, FontSecondary, miles_text);
    widget_add_string_element(app->result_widget, 8, 46, AlignLeft, AlignCenter, FontSecondary, rate_text);
    widget_add_string_element(app->result_widget, 100, 55, AlignCenter, AlignCenter, FontPrimary, rating);
    view_dispatcher_switch_to_view(app->view_dispatcher, VIEW_RESULT);
}

static void dashmate_show_shift(DashMateApp* app) {
    widget_reset(app->shift_widget);
    char status[32];
    char orders[32];
    char earnings[32];
    char miles[32];

    if(app->shift_active) {
        uint32_t elapsed_ms = furi_get_tick() - app->shift_start_tick;
        uint32_t elapsed_min = elapsed_ms / 60000;
        snprintf(status, sizeof(status), "Time: %luh %02lum", (unsigned long)(elapsed_min / 60), (unsigned long)(elapsed_min % 60));
    } else {
        snprintf(status, sizeof(status), "Shift not started");
    }

    snprintf(orders, sizeof(orders), "Orders: %lu", (unsigned long)app->orders);
    snprintf(earnings, sizeof(earnings), "Earnings: $%lu.%02lu", (unsigned long)(app->earnings_cents / 100), (unsigned long)(app->earnings_cents % 100));
    snprintf(miles, sizeof(miles), "Miles: %lu.%lu", (unsigned long)(app->shift_miles_tenths / 10), (unsigned long)(app->shift_miles_tenths % 10));

    widget_add_string_element(app->shift_widget, 64, 8, AlignCenter, AlignCenter, FontPrimary, "SHIFT STATS");
    widget_add_string_element(app->shift_widget, 8, 23, AlignLeft, AlignCenter, FontSecondary, status);
    widget_add_string_element(app->shift_widget, 8, 34, AlignLeft, AlignCenter, FontSecondary, orders);
    widget_add_string_element(app->shift_widget, 8, 45, AlignLeft, AlignCenter, FontSecondary, earnings);
    widget_add_string_element(app->shift_widget, 8, 56, AlignLeft, AlignCenter, FontSecondary, miles);
    view_dispatcher_switch_to_view(app->view_dispatcher, VIEW_SHIFT);
}

static void dashmate_offer_enter_callback(void* context, uint32_t index) {
    DashMateApp* app = context;
    if(index == 2) dashmate_show_result(app);
}

static void dashmate_menu_callback(void* context, uint32_t index) {
    DashMateApp* app = context;
    switch(index) {
    case DashMateMenuStartShift:
        if(!app->shift_active) {
            app->shift_active = true;
            app->shift_start_tick = furi_get_tick();
            app->orders = 0;
            app->earnings_cents = 0;
            app->shift_miles_tenths = 0;
        }
        dashmate_show_shift(app);
        break;
    case DashMateMenuNewOffer:
        view_dispatcher_switch_to_view(app->view_dispatcher, VIEW_OFFER);
        break;
    case DashMateMenuEarnings:
    case DashMateMenuShiftStats:
        dashmate_show_shift(app);
        break;
    case DashMateMenuResetShift:
        app->shift_active = false;
        app->shift_start_tick = 0;
        app->orders = 0;
        app->earnings_cents = 0;
        app->shift_miles_tenths = 0;
        dashmate_show_shift(app);
        break;
    case DashMateMenuAbout:
        FURI_LOG_I("DashMate", "About selected");
        break;
    }
}

static uint32_t dashmate_exit_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

int32_t dashmate_app(void* p) {
    UNUSED(p);
    DashMateApp* app = malloc(sizeof(DashMateApp));
    app->pay_cents = 800;
    app->miles_tenths = 40;
    app->shift_active = false;
    app->shift_start_tick = 0;
    app->orders = 0;
    app->earnings_cents = 0;
    app->shift_miles_tenths = 0;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();
    app->offer_list = variable_item_list_alloc();
    app->result_widget = widget_alloc();
    app->shift_widget = widget_alloc();

    submenu_set_header(app->submenu, "DashMate");
    submenu_add_item(app->submenu, "Start Shift", DashMateMenuStartShift, dashmate_menu_callback, app);
    submenu_add_item(app->submenu, "New Offer", DashMateMenuNewOffer, dashmate_menu_callback, app);
    submenu_add_item(app->submenu, "Earnings", DashMateMenuEarnings, dashmate_menu_callback, app);
    submenu_add_item(app->submenu, "Shift Stats", DashMateMenuShiftStats, dashmate_menu_callback, app);
    submenu_add_item(app->submenu, "Reset Shift", DashMateMenuResetShift, dashmate_menu_callback, app);
    submenu_add_item(app->submenu, "About", DashMateMenuAbout, dashmate_menu_callback, app);
    view_set_previous_callback(submenu_get_view(app->submenu), dashmate_exit_callback);

    VariableItem* pay_item = variable_item_list_add(app->offer_list, "Pay", ((PAY_MAX_CENTS - PAY_MIN_CENTS) / PAY_STEP_CENTS) + 1, dashmate_pay_changed, app);
    variable_item_set_current_value_index(pay_item, (app->pay_cents - PAY_MIN_CENTS) / PAY_STEP_CENTS);
    dashmate_pay_changed(pay_item);

    VariableItem* miles_item = variable_item_list_add(app->offer_list, "Miles", ((MILES_MAX_TENTHS - MILES_MIN_TENTHS) / MILES_STEP_TENTHS) + 1, dashmate_miles_changed, app);
    variable_item_set_current_value_index(miles_item, (app->miles_tenths - MILES_MIN_TENTHS) / MILES_STEP_TENTHS);
    dashmate_miles_changed(miles_item);

    variable_item_list_add(app->offer_list, "Calculate", 1, NULL, app);
    variable_item_list_set_enter_callback(app->offer_list, dashmate_offer_enter_callback, app);
    view_set_previous_callback(variable_item_list_get_view(app->offer_list), dashmate_to_menu_callback);
    view_set_previous_callback(widget_get_view(app->result_widget), dashmate_to_menu_callback);
    view_set_previous_callback(widget_get_view(app->shift_widget), dashmate_to_menu_callback);

    view_dispatcher_add_view(app->view_dispatcher, VIEW_MENU, submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, VIEW_OFFER, variable_item_list_get_view(app->offer_list));
    view_dispatcher_add_view(app->view_dispatcher, VIEW_RESULT, widget_get_view(app->result_widget));
    view_dispatcher_add_view(app->view_dispatcher, VIEW_SHIFT, widget_get_view(app->shift_widget));

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, VIEW_MENU);
    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, VIEW_MENU);
    view_dispatcher_remove_view(app->view_dispatcher, VIEW_OFFER);
    view_dispatcher_remove_view(app->view_dispatcher, VIEW_RESULT);
    view_dispatcher_remove_view(app->view_dispatcher, VIEW_SHIFT);
    widget_free(app->shift_widget);
    widget_free(app->result_widget);
    variable_item_list_free(app->offer_list);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}
