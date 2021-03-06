/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#include "way-cooler-keybindings-unstable-v1.h"

#ifndef AWESOME_WAYLAND_ROOT_H
#define AWESOME_WAYLAND_ROOT_H

#include <cairo/cairo.h>

extern struct zway_cooler_keybindings_listener keybindings_listener;

struct wayland_wallpaper;

int wayland_set_wallpaper(cairo_pattern_t *pattern);
void wayland_update_wallpaper(void);
void wayland_wallpaper_cleanup(struct wayland_wallpaper *wayland_wallpaper);

void wayland_grab_keys(void);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
