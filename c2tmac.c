
/*
 * Copyright (C) 2005-2015 Junjiro R. Okajima
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

#include <stdio.h>
#include <linux/aufs_type.h>
#include "au_util.h"
#include "libau.h"

int
main(int argc, char *argv[])
{
#define p(m, v, fmt)	printf(".ds %s " fmt "\n", m, v)
#define pstr(m)		p(#m, m, "%s")
#define pint(m)		p(#m, m, "%d")
	pstr(AUFS_VERSION);
	pstr(AUFS_XINO_FNAME);
	pstr(AUFS_XINO_DEFPATH);
	pint(AUFS_DIRWH_DEF);
	pstr(AUFS_WH_PFX);
	pint(AUFS_WH_PFX_LEN);
	pint(AUFS_WH_TMP_LEN);
	pint(AUFS_MAX_NAMELEN);
	pstr(AUFS_WKQ_NAME);
	pstr(AUFS_WH_DIROPQ);
	pstr(AUFS_WH_BASE);
	pstr(AUFS_WH_PLINKDIR);
	pint(AUFS_MFS_DEF_SEC);
	pint(AUFS_MFS_MAX_SEC);
	pint(AUFS_RDBLK_DEF);
	pint(AUFS_RDHASH_DEF);
	pint(AUFS_RDCACHE_DEF);
	pint(AUFS_RDCACHE_MAX);

	pstr(DROPLVL);
	pstr(DROPLVL1);
	pstr(DROPLVL1R);
	pstr(DROPLVL2);
	pstr(DROPLVL2R);
	pstr(DROPLVL3);
	pstr(DROPLVL3R);

	pstr(LibAuEnv);
	return 0;
}
