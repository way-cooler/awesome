/*
 * screen.c - screen management
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright ©      2019 Preston Carpenter <APragmaticPlace@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <x11/screen.h>

#include <stdbool.h>

#include "globalconf.h"
#include "objects/screen.h"

static void
screen_output_wipe(screen_output_t *output)
{
    p_delete(&output->name);
}

ARRAY_FUNCS(screen_output_t, screen_output, screen_output_wipe);

static void screen_scan_x11(lua_State *L, screen_array_t *screens)
{
    xcb_screen_t *xcb_screen = globalconf.screen;
    screen_t *s = screen_add(L, screens, NULL);
    s->geometry.x = 0;
    s->geometry.y = 0;
    s->geometry.width = xcb_screen->width_in_pixels;
    s->geometry.height = xcb_screen->height_in_pixels;
}


/* Monitors were introduced in RandR 1.5 */
#ifdef XCB_RANDR_GET_MONITORS
static void
screen_scan_randr_monitors(lua_State *L, screen_array_t *screens)
{
    xcb_randr_get_monitors_cookie_t monitors_c = xcb_randr_get_monitors(globalconf.connection, globalconf.screen->root, 1);
    xcb_randr_get_monitors_reply_t *monitors_r = xcb_randr_get_monitors_reply(globalconf.connection, monitors_c, NULL);
    xcb_randr_monitor_info_iterator_t monitor_iter;

    if (monitors_r == NULL) {
        warn("RANDR GetMonitors failed; this should not be possible");
        return;
    }

    for(monitor_iter = xcb_randr_get_monitors_monitors_iterator(monitors_r);
            monitor_iter.rem; xcb_randr_monitor_info_next(&monitor_iter))
    {
        screen_t *new_screen;
        screen_output_t output;
        xcb_randr_output_t *randr_outputs;
        xcb_get_atom_name_cookie_t name_c;
        xcb_get_atom_name_reply_t *name_r;

        if(!xcb_randr_monitor_info_outputs_length(monitor_iter.data))
            continue;

        new_screen = screen_add(L, screens, NULL);
        new_screen->geometry.x = monitor_iter.data->x;
        new_screen->geometry.y = monitor_iter.data->y;
        new_screen->geometry.width = monitor_iter.data->width;
        new_screen->geometry.height = monitor_iter.data->height;

        struct x11_screen *x11_screen = new_screen->impl_data;
        x11_screen->xid = monitor_iter.data->name;

        output.mm_width = monitor_iter.data->width_in_millimeters;
        output.mm_height = monitor_iter.data->height_in_millimeters;

        name_c = xcb_get_atom_name_unchecked(globalconf.connection, monitor_iter.data->name);
        name_r = xcb_get_atom_name_reply(globalconf.connection, name_c, NULL);
        if (name_r) {
            const char *name = xcb_get_atom_name_name(name_r);
            size_t len = xcb_get_atom_name_name_length(name_r);

            output.name = memcpy(p_new(char *, len + 1), name, len);
            output.name[len] = '\0';
            p_delete(&name_r);
        } else {
            output.name = a_strdup("unknown");
        }
        randr_output_array_init(&output.outputs);

        randr_outputs = xcb_randr_monitor_info_outputs(monitor_iter.data);
        for(int i = 0; i < xcb_randr_monitor_info_outputs_length(monitor_iter.data); i++) {
            randr_output_array_append(&output.outputs, randr_outputs[i]);
        }

        screen_output_array_append(&x11_screen->outputs, output);
    }

    p_delete(&monitors_r);
}
#else
static void
screen_scan_randr_monitors(lua_State *L, screen_array_t *screens)
{
}
#endif

static void
screen_scan_randr_crtcs(lua_State *L, screen_array_t *screens)
{
    /* A quick XRandR recall:
     * You have CRTC that manages a part of a SCREEN.
     * Each CRTC can draw stuff on one or more OUTPUT. */
    xcb_randr_get_screen_resources_cookie_t screen_res_c = xcb_randr_get_screen_resources(globalconf.connection, globalconf.screen->root);
    xcb_randr_get_screen_resources_reply_t *screen_res_r = xcb_randr_get_screen_resources_reply(globalconf.connection, screen_res_c, NULL);

    if (screen_res_r == NULL) {
        warn("RANDR GetScreenResources failed; this should not be possible");
        return;
    }

    /* We go through CRTC, and build a screen for each one. */
    xcb_randr_crtc_t *randr_crtcs = xcb_randr_get_screen_resources_crtcs(screen_res_r);

    for(int i = 0; i < screen_res_r->num_crtcs; i++)
    {
        /* Get info on the output crtc */
        xcb_randr_get_crtc_info_cookie_t crtc_info_c = xcb_randr_get_crtc_info(globalconf.connection, randr_crtcs[i], XCB_CURRENT_TIME);
        xcb_randr_get_crtc_info_reply_t *crtc_info_r = xcb_randr_get_crtc_info_reply(globalconf.connection, crtc_info_c, NULL);

        if(!crtc_info_r) {
            warn("RANDR GetCRTCInfo failed; this should not be possible");
            continue;
        }

        /* If CRTC has no OUTPUT, ignore it */
        if(!xcb_randr_get_crtc_info_outputs_length(crtc_info_r))
            continue;

        /* Prepare the new screen */
        screen_t *new_screen = screen_add(L, screens, NULL);
        new_screen->geometry.x = crtc_info_r->x;
        new_screen->geometry.y = crtc_info_r->y;
        new_screen->geometry.width= crtc_info_r->width;
        new_screen->geometry.height= crtc_info_r->height;
        struct x11_screen *x11_screen = new_screen->impl_data;
        x11_screen->xid = randr_crtcs[i];

        xcb_randr_output_t *randr_outputs = xcb_randr_get_crtc_info_outputs(crtc_info_r);

        for(int j = 0; j < xcb_randr_get_crtc_info_outputs_length(crtc_info_r); j++)
        {
            xcb_randr_get_output_info_cookie_t output_info_c = xcb_randr_get_output_info(globalconf.connection, randr_outputs[j], XCB_CURRENT_TIME);
            xcb_randr_get_output_info_reply_t *output_info_r = xcb_randr_get_output_info_reply(globalconf.connection, output_info_c, NULL);
            screen_output_t output;

            if (!output_info_r) {
                warn("RANDR GetOutputInfo failed; this should not be possible");
                continue;
            }

            int len = xcb_randr_get_output_info_name_length(output_info_r);
            /* name is not NULL terminated */
            char *name = memcpy(p_new(char *, len + 1), xcb_randr_get_output_info_name(output_info_r), len);
            name[len] = '\0';

            output.name = name;
            output.mm_width = output_info_r->mm_width;
            output.mm_height = output_info_r->mm_height;
            randr_output_array_init(&output.outputs);
            randr_output_array_append(&output.outputs, randr_outputs[j]);

            screen_output_array_append(&x11_screen->outputs, output);


            p_delete(&output_info_r);

            if (A_STREQ(name, "default"))
            {
                /* non RandR 1.2+ X driver don't return any usable multihead
                 * data. I'm looking at you, nvidia binary blob!
                 */
                warn("Ignoring RandR, only a compatibility layer is present.");

                /* Get rid of the screens that we already created */
                foreach(screen, *screens)
                    luaA_object_unref(L, *screen);
                screen_array_wipe(screens);
                screen_array_init(screens);

                return;
            }
        }

        p_delete(&crtc_info_r);
    }

    p_delete(&screen_res_r);
}

static void
screen_scan_randr(lua_State *L, screen_array_t *screens)
{
    const xcb_query_extension_reply_t *extension_reply;
    xcb_randr_query_version_reply_t *version_reply;
    uint32_t major_version;
    uint32_t minor_version;

    /* Check for extension before checking for XRandR */
    extension_reply = xcb_get_extension_data(globalconf.connection, &xcb_randr_id);
    if(!extension_reply || !extension_reply->present)
        return;

    version_reply =
        xcb_randr_query_version_reply(globalconf.connection,
                                      xcb_randr_query_version(globalconf.connection, 1, 5), 0);
    if(!version_reply)
        return;

    major_version = version_reply->major_version;
    minor_version = version_reply->minor_version;
    p_delete(&version_reply);

    /* Do we agree on a supported version? */
    if (major_version != 1 || minor_version < 2)
        return;

    globalconf.have_randr_13 = minor_version >= 3;
#if XCB_RANDR_MAJOR_VERSION > 1 || XCB_RANDR_MINOR_VERSION >= 5
    globalconf.have_randr_15 = minor_version >= 5;
#else
    globalconf.have_randr_15 = false;
#endif

    /* We want to know when something changes */
    xcb_randr_select_input(globalconf.connection,
                           globalconf.screen->root,
                           XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

    if (globalconf.have_randr_15)
        screen_scan_randr_monitors(L, screens);
    else
        screen_scan_randr_crtcs(L, screens);

    if (screens->len == 0)
    {
        /* Scanning failed, disable randr again */
        xcb_randr_select_input(globalconf.connection,
                               globalconf.screen->root,
                               0);
        globalconf.have_randr_13 = false;
        globalconf.have_randr_15 = false;
    }
}

static void
screen_scan_xinerama(lua_State *L, screen_array_t *screens)
{
    bool xinerama_is_active;
    const xcb_query_extension_reply_t *extension_reply;
    xcb_xinerama_is_active_reply_t *xia;
    xcb_xinerama_query_screens_reply_t *xsq;
    xcb_xinerama_screen_info_t *xsi;
    int xinerama_screen_number;

    /* Check for extension before checking for Xinerama */
    extension_reply = xcb_get_extension_data(globalconf.connection, &xcb_xinerama_id);
    if(!extension_reply || !extension_reply->present)
        return;

    xia = xcb_xinerama_is_active_reply(globalconf.connection, xcb_xinerama_is_active(globalconf.connection), NULL);
    xinerama_is_active = xia && xia->state;
    p_delete(&xia);
    if(!xinerama_is_active)
        return;

    xsq = xcb_xinerama_query_screens_reply(globalconf.connection,
                                           xcb_xinerama_query_screens_unchecked(globalconf.connection),
                                           NULL);

    if(!xsq) {
        warn("Xinerama QueryScreens failed; this should not be possible");
        return;
    }

    xsi = xcb_xinerama_query_screens_screen_info(xsq);
    xinerama_screen_number = xcb_xinerama_query_screens_screen_info_length(xsq);

    for(int screen = 0; screen < xinerama_screen_number; screen++)
    {
        screen_t *s = screen_add(L, screens, NULL);
        s->geometry.x = xsi[screen].x_org;
        s->geometry.y = xsi[screen].y_org;
        s->geometry.width = xsi[screen].width;
        s->geometry.height = xsi[screen].height;
    }

    p_delete(&xsq);
}

void x11_new_screen(screen_t *screen, void *data)
{
    screen->impl_data = calloc(1, sizeof(struct x11_screen));
    struct x11_screen *x11_screen = screen->impl_data;

    x11_screen->xid = XCB_NONE;
}

void x11_wipe_screen(screen_t *screen)
{
    struct x11_screen *x11_screen = screen->impl_data;

    screen_output_array_wipe(&x11_screen->outputs);

    free(screen->impl_data);
    screen->impl_data = NULL;
}

void x11_mark_fake_screen(screen_t *screen)
{
    struct x11_screen *x11_screen = screen->impl_data;
    x11_screen->xid = FAKE_SCREEN_XID;
}

void x11_scan_screens(void)
{
    lua_State *L;

    L = globalconf_get_lua_State();

    screen_scan_randr(L, &globalconf.screens);
    if (globalconf.screens.len == 0)
        screen_scan_xinerama(L, &globalconf.screens);
    if (globalconf.screens.len == 0)
        screen_scan_x11(L, &globalconf.screens);
    check(globalconf.screens.len > 0);

    screen_deduplicate(L, &globalconf.screens);

    foreach(screen, globalconf.screens) {
        screen_added(L, *screen);
    }

    screen_update_primary();
}

void x11_get_screens(lua_State *L, struct screen_array_t *new_screens)
{
    if (globalconf.have_randr_15)
        screen_scan_randr_monitors(L, new_screens);
    else
        screen_scan_randr_crtcs(L, new_screens);

    screen_deduplicate(L, new_screens);

    /* Running without any screens at all is no fun. */
    if (new_screens->len == 0)
        screen_scan_x11(L, new_screens);
}

int x11_get_outputs(lua_State *L, screen_t *s)
{
    struct x11_screen *x11_screen = s->impl_data;
    lua_createtable(L, 0, x11_screen->outputs.len);
    foreach(output, x11_screen->outputs)
    {
        lua_createtable(L, 2, 0);

        lua_pushinteger(L, output->mm_width);
        lua_setfield(L, -2, "mm_width");
        lua_pushinteger(L, output->mm_height);
        lua_setfield(L, -2, "mm_height");

        lua_setfield(L, -2, output->name);
    }
    /* The table of tables we created. */
    return 1;
}


screen_t *x11_update_primary(void)
{
    screen_t *primary_screen = NULL;
    if (!globalconf.have_randr_13)
        return NULL;

    xcb_randr_get_output_primary_reply_t *primary =
        xcb_randr_get_output_primary_reply(globalconf.connection,
                xcb_randr_get_output_primary(globalconf.connection, globalconf.screen->root),
                NULL);

    if (!primary)
        return NULL;

    foreach(screen, globalconf.screens)
    {
        struct x11_screen *x11_screen = (*screen)->impl_data;
        foreach(output, x11_screen->outputs)
            foreach (randr_output, output->outputs)
            if (*randr_output == primary->output)
                primary_screen = *screen;
    }
    p_delete(&primary);

    if (!primary_screen || primary_screen == globalconf.primary_screen)
        return NULL;
    return primary_screen;
}

screen_t *x11_screen_by_name(const char *name)
{
    foreach(screen, globalconf.screens)
    {
        struct x11_screen *x11_screen = (*screen)->impl_data;
        foreach(output, x11_screen->outputs)
            if(A_STREQ(output->name, name))
                return *screen;
    }
    return NULL;
}

bool x11_outputs_changed(screen_t *existing, screen_t *other)
{
    struct x11_screen *existing_screen = existing->impl_data;
    struct x11_screen *other_screen = other->impl_data;

    bool outputs_changed = existing_screen->outputs.len != other_screen->outputs.len;
    if(!outputs_changed)
        for(int i = 0; i < existing_screen->outputs.len; i++) {
            screen_output_t *existing_output = &existing_screen->outputs.tab[i];
            screen_output_t *other_output = &other_screen->outputs.tab[i];
            outputs_changed |= existing_output->mm_width != other_output->mm_width;
            outputs_changed |= existing_output->mm_height != other_output->mm_height;
            outputs_changed |= A_STRNEQ(existing_output->name, other_output->name);
        }

    /* Brute-force update the outputs by swapping */
    screen_output_array_t tmp = other_screen->outputs;
    other_screen->outputs = existing_screen->outputs;
    existing_screen->outputs = tmp;

    return outputs_changed;
}

bool x11_does_screen_exist(screen_t *screen, screen_array_t screens)
{
    struct x11_screen *x11_screen = screen->impl_data;

    foreach(old_screen, screens)
    {
        struct x11_screen *x11_old_screen = (*old_screen)->impl_data;
        if (x11_screen->xid == x11_old_screen->xid)
            return true;
    }
    return false;
}

bool x11_is_fake_screen(screen_t *screen)
{
    struct x11_screen *x11_screen = screen->impl_data;
    return x11_screen->xid == FAKE_SCREEN_XID;
}

bool x11_is_same_screen(screen_t *left, screen_t *right)
{
    struct x11_screen *left_x11_screen = left->impl_data;
    struct x11_screen *right_x11_screen = right->impl_data;

    return left_x11_screen->xid == right_x11_screen->xid;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
