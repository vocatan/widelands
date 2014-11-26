/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef WL_GRAPHIC_SUB_TEXTURE_H
#define WL_GRAPHIC_SUB_TEXTURE_H

#include "base/macros.h"
#include "base/rect.h"
#include "graphic/gl/system_headers.h"

class SubTexture {
public:
	SubTexture(GLuint gl_texture, const FloatRect& dimensions);

private:
	DISALLOW_COPY_AND_ASSIGN(SubTexture);
};


#endif  // end of include guard: WL_GRAPHIC_SUB_TEXTURE_H
