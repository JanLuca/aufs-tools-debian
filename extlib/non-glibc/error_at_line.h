/*
 * Copyright (C) 2013-2017 Junjiro R. Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __ERROR_AT_LINE_H__
#define __ERROR_AT_LINE_H__

#ifdef __GNU_LIBRARY__
#error this is for non-glibc.
#else
void error_at_line(int status, int errnum, const char *filename,
		   unsigned int linenum, const char *format, ...);
#endif

#endif /* __ERROR_AT_LINE_H__ */
