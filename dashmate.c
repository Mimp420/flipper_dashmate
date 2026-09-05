#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
} DashMateApp;

enum DashMateMenuItem {
    DashMateMenuStartShift,
    DashMateMenuNewOffer,
    DashMateMenuEarnings,
    DashMateMenuShiftStats,
    DashMateMenuResetShift,
    DashMateMenuAbout,
};

static void dashmate_menu_callback(void* context, uint32_t index) {
    DashMateApp* app = context;

    switch(index) {
    case DashMateMenuStartShift:
        FURI_LOG_I("DashMate", "Start Shift selected");
        break;

    case DashMateMenuNewOffer:
        FURI_LOG_I("DashMate", "New Offer selected");
        break;

    case DashMateMenuEarnings:
        FURI_LOG_I("DashMate", "Earnings selected");
        break;

    case DashMateMenuShiftStats:
        FURI_LOG_I("DashMate", "Shift Stats selected");
        break;

    case DashMateMenuResetShift:
        FURI_LOG_I("DashMate", "Reset Shift selected");
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

    app->gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);

    app->submenu = submenu_alloc();

    submenu_set_header(app->submenu, "DashMate");

    submenu_add_item(
        app->submenu,
        "Start Shift",
        DashMateMenuStartShift,
        dashmate_menu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "New Offer",
        DashMateMenuNewOffer,
        dashmate_menu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Earnings",
        DashMateMenuEarnings,
        dashmate_menu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Shift Stats",
        DashMateMenuShiftStats,
        dashmate_menu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "Reset Shift",
        DashMateMenuResetShift,
        dashmate_menu_callback,
        app);

    submenu_add_item(
        app->submenu,
        "About",
        DashMateMenuAbout,
        dashmate_menu_callback,
        app);

    view_set_previous_callback(
        submenu_get_view(app->submenu),
        dashmate_exit_callback);

    view_dispatcher_add_view(
        app->view_dispatcher,
        0,
        submenu_get_view(app->submenu));

    view_dispatcher_attach_to_gui(
        app->view_dispatcher,
        app->gui,
        ViewDispatcherTypeFullscreen);

    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, 0);

    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);

    free(app);

    return 0;
}
