/*
 * Copyright (C) 2005-2017 Junjiro R. Okajima
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

/*
 * The main purpose of this script is updating /etc/mtab and calling auplilnk.
 * This behaviour is highly depending on mount(8) in util-linux package.
 */

#define _XOPEN_SOURCE	500	/* getsubopt */
#define _BSD_SOURCE		/* dirfd */

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <mntent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/aufs_type.h>
#include "au_util.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define DROPLVL_STR(lvl)		   \
	{				   \
		.set = DROPLVL ## lvl,	   \
		.clr = DROPLVL ## lvl ## R \
	}

static struct {
	int	val;
	char	*arg;

	struct {
		char *set, *clr;
	} str[3];
} droplvl = {
	/* .val	= DROPLVL_INVALID, */
	.str = {
		DROPLVL_STR(1),
		DROPLVL_STR(2),
		DROPLVL_STR(3)
	}
};

#define DROPLVL_INVALID		ARRAY_SIZE(droplvl.str)

enum { Remount, Bind, Drop, Fake, Update, Verbose, AuFlush, LastOpt };
static void test_opts(char opts[], unsigned char flags[])
{
	int c;
	long l;
	char *p, *last, *o, *val, *pat[] = {
		[Remount]	= "remount",
		[Bind]		= "bind",
		[Drop]		= DROPLVL,
		NULL
	};

	o = strdup(opts);
	if (!o)
		AuFin("stdup");

	droplvl.arg = NULL;
	droplvl.val = DROPLVL_INVALID;
	p = o;
	while (*p) {
		last = opts + (p - o);
		c = getsubopt(&p, pat, &val);
		switch (c) {
		case Remount:
			flags[Remount] = 1;
			break;
		case Bind:
			flags[Bind] = 1;
			break;
		case Drop:
			flags[Drop] = 1;
			droplvl.arg = last;
			errno = 0;
			l = strtol(val, NULL, 0);;
			if (errno
			    || !l
			    || DROPLVL_INVALID < abs(l)) {
				errno = EINVAL;
				AuFin("invalid value %ld, %s", l, droplvl.arg);
			}
			droplvl.val = l;
			break;
		}
	}
	free(o);
}

static int test_flush(char opts[])
{
	int err, i;
	regex_t preg;
	char *p, *o;
	const char *pat = "^((add|ins|append|prepend|del)[:=]"
		"|(mod|imod)[:=][^,]*=ro"
		"|(noplink|ro)$)";


	o = strdup(opts);
	if (!o)
		AuFin("stdup");

	p = o;
	i = 1;
	while ((p = strchr(p, ','))) {
		i++;
		*p++ = 0;
	}

	/* todo: try getsubopt(3)? */
	err = regcomp(&preg, pat, REG_EXTENDED | REG_NOSUB);
	if (err) {
		size_t sz;
		char a[128];

		sz = regerror(err, &preg, a, sizeof(a));
		AuFin("regcomp: %.*s", (int)sz, a);
	}

	p = o;
	while (i--) {
		if (!regexec(&preg, p, 0, NULL, 0)) {
			err = 1;
			break;
		} else
			p += strlen(p) + 1;
	}
	regfree(&preg);
	free(o);

	return err;
}

static int drop_level(int argc, char **argv, int idx)
{
	int i, lvl, neg;
	size_t l, t;
	char *src, *o, *p, *str;

	src = argv[idx];
	l = strlen(src) + 1;
	if (droplvl.arg < src || src + l < droplvl.arg) {
		errno = EINVAL;
		AuFin("internal error, src %p, l %zu, droplvl %p",
		      src, l, droplvl.arg);
	}
	l -= sizeof(DROPLVL) - 1 + 2;	/* "=N" */

	o = malloc(l);
	t = droplvl.arg - src;
	memcpy(o, src, t);
	p = o + t;
	*p = '\0';

	lvl = abs(droplvl.val);
	neg = 0;
	if (droplvl.val < 0)
		neg = 1;
	for (i = 0; i < lvl; i++) {
		str = droplvl.str[i].set;
		if (neg)
			str = droplvl.str[i].clr;

		/* with comma or terminating NULL */
		l += strlen(str) + 1;
		p = realloc(o, l);
		if (!p)
			AuFin("realloc");
		o = p;
		strcat(o, str);
		if (i + 1 < lvl)
			strcat(o, ",");
	}

	p = strchr(droplvl.arg, ',');
	if (p)
		strcat(o, p);
	Dpri("o %s\n", o);

	argv[idx] = o;
	return 0;
}

static void do_mount(char *dev, char *mntpnt, int argc, char *argv[],
		     unsigned char flags[])
{
	int i;
	const int ac = argc + 7;
	char *av[ac], **a;

	/* todo: eliminate the duplicated options */
	a = av;
	*a++ = "mount";
	*a++ = "-i";
	if (flags[Fake])
		*a++ = "-f";
	if (!flags[Bind] || !flags[Update])
		*a++ = "-n";
	if (flags[Bind] && flags[Verbose])
		*a++ = "-v";
	*a++ = "-t";
	*a++ = AUFS_NAME;

	for (i = 3; i < argc; i++)
		if (strcmp(argv[i], "-f")
		    && strcmp(argv[i], "-n")
		    && strcmp(argv[i], "-v"))
			*a++ = argv[i];
	*a++ = dev;
	*a++ = mntpnt;
	*a = NULL;

	i = a - av;
	if (i > ac)
		AuFin("internal error, %d > %d\n", i, ac);

#ifdef DEBUG
	for (i = 0; av[i] && i < ac; i++)
		puts(av[i]);
	exit(0);
#endif
	execv(MOUNT_CMD, av);
	AuFin("mount");
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	int err, c, status, fd, opts_idx;
	pid_t pid;
	unsigned char flags[LastOpt];
	struct mntent ent;
	char *dev, *mntpnt, *opts, *cwd;
	DIR *cur;

	for (c = 0; c < argc; c++)
		Dpri("%s\n", argv[c]);
	if (argc < 3) {
		puts(AuVersion);
		errno = EINVAL;
		AuFin(NULL);
	}

	memset(flags, 0, sizeof(flags));
	flags[Update] = 1;
	opts = NULL;
	opts_idx = -1;

	/* mount(8) always passes the arguments in this order */
	dev = argv[1];
	mntpnt = argv[2];
	while ((c = getopt(argc - 2, argv + 2, "fnvo:")) != -1) {
		switch (c) {
		case 'f':
			flags[Fake] = 1;
			break;
		case 'n':
			flags[Update] = 0;
			break;
		case 'v':
			flags[Verbose] = 1;
			break;
		case 'o':
			if (!opts) {
				opts = optarg;
				opts_idx = optind + 1;
				break;
			}
			/*FALLTHROUGH*/
		case '?':
		case ':':
			errno = EINVAL;
			AuFin("internal error");
		}
	}

	cur = opendir(".");
	if (!cur)
		AuFin(".");
	err = chdir(mntpnt);
	if (err)
		AuFin("%s", mntpnt);
	cwd = getcwd(NULL, 0); /* glibc */
	if (!cwd)
		AuFin("getcwd");
	err = fchdir(dirfd(cur));
	if (err)
		AuFin("fchdir");
	closedir(cur); /* ignore */

	if (opts)
		test_opts(opts, flags);

	if (!flags[Bind] && flags[Update]) {
		err = access(MTab, R_OK | W_OK);
		if (err)
			AuFin(MTab);
	}

	fd = -1;
	if (flags[Remount]) {
		errno = EINVAL;
		if (flags[Bind])
			AuFin("both of remount and bind are specified");
		flags[AuFlush] = test_flush(opts);
		if (flags[AuFlush] /* && !flags[Fake] */) {
			err = au_plink(cwd, AuPlink_FLUSH,
				       AuPlinkFlag_OPEN | AuPlinkFlag_CLOEXEC,
				       &fd);
			if (err)
				AuFin(NULL);
		}
	}

	if (flags[Drop])
		err = drop_level(argc, argv, opts_idx);

	pid = fork();
	if (!pid) {
		/* actual mount operation */
		do_mount(dev, mntpnt, argc, argv, flags);
		return 0;
	} else if (pid < 0)
		AuFin("fork");

	if (fd >= 0)
		close(fd); /* ignore */
	err = waitpid(pid, &status, 0);
	if (err < 0)
		AuFin("child process");

	err = !WIFEXITED(status);
	if (!err)
		err = WEXITSTATUS(status);

	mng_fhsm(cwd, /*umount*/0);

	if (!err && !flags[Bind]) {
		if (flags[Update])
			err = au_update_mtab(cwd, flags[Remount],
					     flags[Verbose]);
		else if (flags[Verbose]) {
			/* withoug blocking plink */
			err = au_proc_getmntent(cwd, &ent);
			if (!err)
				au_print_ent(&ent);
			else
				AuFin("internal error");
		}
	}

	return err;
}
