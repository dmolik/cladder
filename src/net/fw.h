/*
 * Copyright 2017 Dan Molik <dan@danmolik.com>
 *
 * This file is part of Cladder
 *
 * Cladder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Cladderis distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Cladder.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _CL_NET_FW_H
#define _CL_NET_FW_H


struct _rule {
	char *table;
	char *entry;
	char *type;
	char *iface;
	char *oface;
	char *saddr;
	char *daddr;
};

struct _addr_t {
	unsigned int addr;
	unsigned int mask;
};


struct _addr_t *  _init_addr(const char *ip);

void _free_addr(struct _addr_t *addr);

int _ipt_rule(struct _rule *rule);


#endif
