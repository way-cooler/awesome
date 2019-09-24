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

#ifndef AWESOME_X11_SCREEN_H
#define AWESOME_X11_SCREEN_H

#include <stdint.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>

#include "common/array.h"
#include "common/luaclass.h"
#include "objects/screen.h"

/* The XID that is used on fake screens. X11 guarantees that the top three bits
 * of a valid XID are zero, so this will not clash with anything.
 */
#define FAKE_SCREEN_XID ((uint32_t) 0xffffffff)

DO_ARRAY(xcb_randr_output_t, randr_output, DO_NOTHING);

struct screen_output_t
{
    /** The XRandR names of the output */
    char *name;
    /** The size in millimeters */
    uint32_t mm_width, mm_height;
    /** The XID */
    randr_output_array_t outputs;
};

typedef struct screen_output_t screen_output_t;
ARRAY_TYPE(screen_output_t, screen_output);

struct x11_screen
{
    /** The screen outputs informations */
    screen_output_array_t outputs;
    /** Some XID identifying this screen */
    uint32_t xid;
};

void x11_new_screen(screen_t *screen, void *data);
void x11_wipe_screen(screen_t *screen);
void x11_mark_fake_screen(screen_t *screen);
void x11_scan_screens(void);
void x11_get_screens(lua_State *L, struct screen_array_t *screens);

int x11_get_outputs(lua_State *L, screen_t *s);

screen_t *x11_update_primary(void);
screen_t *x11_screen_by_name(const char *name);

bool x11_outputs_changed(screen_t *existing, screen_t *other);
bool x11_does_screen_exist(screen_t *screen, screen_array_t screens);
bool x11_is_fake_screen(screen_t *screen);
bool x11_is_same_screen(screen_t *left, screen_t *right);

#endif // AWESOME_X11_SCREEN_H
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
