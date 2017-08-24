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


#ifndef _CL_NET_NL_H
#define _CL_NET_NL_H


#include <linux/rtnetlink.h>
#include <linux/netlink.h>

#ifndef VETH_INFO_PEER
#define VETH_INFO_PEER 1
#endif

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((unsigned char *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

void _nlmsg_put(struct nlmsghdr *nlmsg, int type, void *data, size_t len);

#define NLMSG_STRING(nl, attr, data) \
	_nlmsg_put((nl), (attr), (data), (strlen((data)) + 1))
#define NLMSG_ATTR(nl, attr) \
	_nlmsg_put((nl), (attr), (NULL), (0))

int _nlmsg_send(int fd, struct nlmsghdr *nlmsg);
int _nlmsg_recieve(int fd);
int _nl_socket_init(void);

#endif
