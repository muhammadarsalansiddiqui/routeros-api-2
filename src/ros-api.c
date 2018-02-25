/**
 * libmikrotik - src/ros-api.c
 * Copyright (C) 2009  Florian octo Forster
 * Modified by Petri Riihikallio 2018
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:
 *   Florian octo Forster <octo at verplant.org>
 *   Petri Riihikallio <petri dot riihikallio at metis dot fi>
 **/

#ifndef _ISOC99_SOURCE
# define _ISOC99_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <getopt.h>

#include "routeros_api.h"

#if !__GNUC__
# define __attribute__(x) /**/
#endif

static const char *opt_username = "admin";

static int result_handler (ros_connection_t *c, const ros_reply_t *r, /* {{{ */
		void *user_data)
{
	unsigned int i;

	if (r == NULL)
		return (0);

	printf ("Status: %s\n", ros_reply_status (r));

	for (i = 0; /* true */; i++)
	{
		const char *key;
		const char *val;

		key = ros_reply_param_key_by_index (r, i);
		val = ros_reply_param_val_by_index (r, i);

		if ((key == NULL) || (val == NULL))
		{
			if (key != NULL)
				fprintf (stderr, "val is NULL but key is %s!\n", key);
			if (val != NULL)
				fprintf (stderr, "key is NULL but val is %s!\n", val);
			break;
		}

		printf ("  Param %u: %s = %s\n", i, key, val);
	}

	printf ("===\n");

	return (result_handler (c, ros_reply_next (r), user_data));
} /* }}} int result_handler */

static char *read_password (void) /* {{{ */
{
	FILE *tty;
	struct termios old_flags;
	struct termios new_flags;
	int status;
	char buffer[1024];
	size_t buffer_len;
	char *passwd;

	tty = fopen ("/dev/tty", "w+");
	if (tty == NULL)
	{
		fprintf (stderr, "Unable to open /dev/tty: %s\n",
				strerror (errno));
		return (NULL);
	}

	fprintf (tty, "Password for user %s: ", opt_username);
	fflush (tty);

	memset (&old_flags, 0, sizeof (old_flags));
	tcgetattr (fileno (tty), &old_flags);
	new_flags = old_flags;
	/* clear ECHO */
	new_flags.c_lflag &= ~ECHO;
	/* set ECHONL */
	new_flags.c_lflag |= ECHONL;

	status = tcsetattr (fileno (tty), TCSANOW, &new_flags);
	if (status != 0)
	{
		fprintf (stderr, "tcsetattr failed: %s\n", strerror (errno));
		fclose (tty);
		return (NULL);
	}

	if (fgets (buffer, sizeof (buffer), tty) == NULL)
	{
		fprintf (stderr, "fgets failed: %s\n", strerror (errno));
		fclose (tty);
		return (NULL);
	}
	buffer[sizeof (buffer) - 1] = 0;
	buffer_len = strlen (buffer);

	status = tcsetattr (fileno (tty), TCSANOW, &old_flags);
	if (status != 0)
		fprintf (stderr, "tcsetattr failed: %s\n", strerror (errno));

	fclose (tty);
	tty = NULL;

	while ((buffer_len > 0) && ((buffer[buffer_len-1] == '\n') || (buffer[buffer_len-1] == '\r')))
	{
		buffer_len--;
		buffer[buffer_len] = 0;
	}
	if (buffer_len == 0)
		return (NULL);

	passwd = malloc (strlen (buffer) + 1);
	if (passwd == NULL)
		return (NULL);
	memcpy (passwd, buffer, strlen (buffer) + 1);
	memset (buffer, 0, sizeof (buffer));

	return (passwd);
} /* }}} char *read_password */

static void exit_usage (void) /* {{{ */
{
	printf ("Usage: ros [options] <host> <command> [args]\n"
			"\n"
			"OPTIONS:\n"
			"  -u <user>       Use <user> to authenticate.\n"
			"  -p <password>   Use <password> to authenticate.\n"
			"  -h              Display this help message.\n"
			"\n");
	if (ros_version () == ROS_VERSION)
		printf ("Using librouteros %s\n", ROS_VERSION_STRING);
	else
		printf ("Using librouteros %s (%s)\n",
				ros_version_string (), ROS_VERSION_STRING);
	printf ("Copyright (c) 2009-2018 by Florian Forster\n");

	exit (EXIT_SUCCESS);
} /* }}} void exit_usage */

int main (int argc, char **argv) /* {{{ */
{
	ros_connection_t *c;
	char *passwd;
	const char *host;
	const char *command;

	int option;

	while ((option = getopt (argc, argv, "u:p:h?")) != -1)
	{
		switch (option)
		{
			case 'u':
				opt_username = optarg;
				break;

			case 'p':
				passwd = optarg;
				break;
				
			case 'h':
			case '?':
			default:
				exit_usage ();
				break;
		}
	}

	if ((argc - optind) < 2)
		exit_usage ();

	host = argv[optind];
	command = argv[optind+1];

	if (passwd == NULL)
	{
		passwd = read_password ();
		if (passwd == NULL)
			exit (EXIT_FAILURE);
	}

	c = ros_connect (host, ROUTEROS_API_PORT,
			opt_username, passwd);
	memset (passwd, 0, strlen (passwd));
	if (c == NULL)
	{
		fprintf (stderr, "ros_connect failed: %s\n", strerror (errno));
		exit (EXIT_FAILURE);
	}

	if (command[0] == '/')
	{
		ros_query (c, command,
				(size_t) (argc - (optind + 2)), (const char * const *) (argv + optind + 2),
				result_handler, /* user data = */ NULL);
	}
	else
	{
		fprintf (stderr, "Unknown built-in command %s. "
				"Are you missing a leading slash?\n", command);
	}

	ros_disconnect (c);

	return (0);
} /* }}} int main */

/* vim: set ts=2 sw=2 noet fdm=marker : */
