/*
 * drawable.h - drawable functions header
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2010-2012 Uli Schlachter <psychon@znc.in>
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

#ifndef AWESOME_OBJECTS_DRAWABLE_H
#define AWESOME_OBJECTS_DRAWABLE_H

#include "common/luaclass.h"
#include "draw.h"

typedef void drawable_refresh_callback(void *);

struct drawable_t;

struct drawable_impl
{
    xcb_pixmap_t (*get_pixmap)(struct drawable_t *drawable);

    /* Drawable private functions */
    void (*drawable_allocate)(struct drawable_t *drawable);
    void (*drawable_unset_surface)(struct drawable_t *drawable);
    void (*drawable_allocate_buffer)(struct drawable_t *drawable);
    void (*drawable_cleanup)(struct drawable_t *drawable);
};

/** drawable type */
struct drawable_t
{
    LUA_OBJECT_HEADER
    /* XXX This data should only be cast from drawable_impl functions */
    void *impl_data;
    /** Surface for drawing. */
    cairo_surface_t *surface;
    /** The geometry of the drawable (in root window coordinates). */
    area_t geometry;
    /** Surface contents are undefined if this is false. */
    bool refreshed;
    /** Callback for refreshing. */
    drawable_refresh_callback *refresh_callback;
    /** Data for refresh callback. */
    void *refresh_data;
};
typedef struct drawable_t drawable_t;

drawable_t *drawable_allocator(lua_State *, drawable_refresh_callback *, void *);
void drawable_set_geometry(lua_State *, int, area_t);
void drawable_class_setup(lua_State *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
