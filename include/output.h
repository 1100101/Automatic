/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#define MSGSIZE_MAX 700

#define TIME_STR_SIZE 30
enum debug_type {
	P_NONE = -1,
	P_ERROR = 0,
	P_MSG = 1,
	P_INFO = 2,
	P_INFO2 = 3,
	P_DBG = 4
};

typedef enum debug_type debug_type;

void dbg_printf(debug_type type, const char *format, ...);
char* getlogtime_str(char *buf);
