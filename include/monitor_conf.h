/*
 * Copyright (C) 2015 Benoit Masson (yahoo@perenite.com) 
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

#ifndef MONITORCONF_H_
#define MONITORCONF_H_

#include "stdbool.h"

unsigned int monitorConf_init(const char *config_file);
void monitorConf_close(void);
bool monitorConf_hasChanged(void);

#endif /* JSON_H_ */
