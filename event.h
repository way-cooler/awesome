/*
 * event.h - event handlers header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_EVENT_H
#define AWESOME_EVENT_H

#include "banning.h"
#include "globalconf.h"
#include "stack.h"

#include <xcb/xcb.h>

/* luaa.c */
void luaA_emit_refresh(void);

/* objects/drawin.c */
void drawin_refresh(void);

/* objects/client.c */
void client_refresh(void);
void client_focus_refresh(void);
void client_destroy_later(void);

static inline int
awesome_refresh(void)
{
#ifdef WITH_WAYLAND
    if (globalconf.wl_display)
    {
        wl_display_roundtrip(globalconf.wl_display);
    }
#endif
    luaA_emit_refresh();
    drawin_refresh();
    client_refresh();
    banning_refresh();
    stack_refresh();
    client_destroy_later();
    return xcb_flush(globalconf.connection);
}

void event_init(void);
void event_handle(xcb_generic_event_t *);
void event_drawable_under_mouse(lua_State *, int);

#ifdef WITH_WAYLAND
void event_mouse_button(void *data, struct zway_cooler_mousegrabber *mousegrabber,
        int32_t x, int32_t y, uint32_t button);
void event_mouse_moved(void *data, struct zway_cooler_mousegrabber *mousegrabber,
        int32_t x, int32_t y, uint32_t button);
#endif

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
