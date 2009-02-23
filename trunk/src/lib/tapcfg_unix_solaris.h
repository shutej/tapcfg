/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008-2009  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <sys/stropts.h>
#include "if_tun.h"

#define TUNNEWPPA       (('T'<<16) | 0x0001)
#define TUNSETPPA       (('T'<<16) | 0x0002)

static int
tapcfg_start_dev(tapcfg_t *tapcfg, const char *ifname, int fallback)
{
	int tap_fd = -1;
	char buf[128];
	struct ifreq ifr;
	struct lifreq lifr;
	struct strioctl strioc;
	int ret, ppa;

	buf[sizeof(buf)-1] = '\0';

	tap_fd = open("/dev/tap", O_RDWR, 0);
	if (tap_fd < 0) {
		taplog_log(TAPLOG_ERR,
			   "Couldn't open the tap device\n");
		taplog_log(TAPLOG_INFO,
			   "Check that you are running the program with "
			   "root privileges and have TUN/TAP driver installed\n");
		return -1;
	}

	strioc.ic_cmd = TUNNEWPPA;
	strioc.ic_timout = 0;
	strioc.ic_len = sizeof(ppa);
	strioc.ic_dp = (char *) &ppa;
	if ((ppa = ioctl(tap_fd, I_STR, &strioc)) == -1) {
		taplog_log(TAPLOG_ERR, "Couldn't assign new interface\n");
	}

	if (ioctl(tap_fd, I_PUSH, "ip") == -1) {
		taplog_log(TAPLOG_ERR, "Couldn't push IP module\n");
		close(tap_fd);
		return -1;
	}


	snprintf(buf, 128, "tap%d", ppa);
	printf("Device name %s\n", buf);

	memset(&lifr, 0, sizeof(struct lifreq));
	if (ioctl(tap_fd, SIOCGLIFFLAGS, &lifr) == -1) {
		taplog_log(TAPLOG_ERR, "Can't get flags\n");
	}

	strcpy(lifr.lifr_name, buf);
	lifr.lifr_ppa = ppa;
	if (ioctl(tap_fd, SIOCSLIFNAME, &lifr) == -1) {
		taplog_log(TAPLOG_ERR, "Couldn't set interface name\n");
	}

	/* Set the device name to be the one we found finally */
	strcpy(lifr.lifr_name, buf);
	taplog_log(TAPLOG_DEBUG, "Device name %s\n", lifr.lifr_name);
	strncpy(tapcfg->ifname, lifr.lifr_name, sizeof(tapcfg->ifname)-1);

	/* Get MAC address on Solaris */
	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, tapcfg->ifname);
	ret = ioctl(tap_fd, SIOCGENADDR, &ifr);
	if (ret == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error getting the MAC address of TAP device '%s': %s\n",
		           tapcfg->ifname, strerror(errno));
	}
	memcpy(tapcfg->hwaddr, ifr.ifr_enaddr, HWADDRLEN);

	return tap_fd;
}

static void
tapcfg_iface_prepare(const char *ifname)
{
}

static int
tapcfg_hwaddr_ioctl(int ctrl_fd,
                    const char *ifname,
                    const char *hwaddr)
{
	return -1;
}

static int
tapcfg_ifaddr_ioctl(int ctrl_fd,
                    const char *ifname,
                    unsigned int addr,
                    unsigned int mask)
{
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;

	if (ioctl(ctrl_fd, SIOCSIFADDR, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error trying to configure IPv4 address: %s\n",
		           strerror(errno));
		return -1;
	}

	memset(&ifr,  0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifname);

	sin = (struct sockaddr_in *) &ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mask;

	if (ioctl(ctrl_fd, SIOCSIFNETMASK, &ifr) == -1) {
		taplog_log(TAPLOG_ERR,
		           "Error trying to configure IPv4 netmask: %s\n",
		           strerror(errno));
		return -1;
	}

	return 0;
}
