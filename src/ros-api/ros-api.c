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

#include "../routeros_api.h"
#include "read_config_file.h"
#include "read_password.h"

#if !__GNUC__
# define __attribute__(x) /**/
#endif

static const char *opt_username = "admin";
int opt_silent = 0;

static int result_handler (ros_connection_t *c, const ros_reply_t *r, /* {{{ */
		void *user_data)
{
	unsigned int i;

	if (r == NULL)
		return (0);

	if (!opt_silent) 
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

		if (!opt_silent) 
			printf ("  Param %u: %s = %s\n", i, key, val);
	}

	if (!opt_silent) 
		printf ("===\n");

	return (result_handler (c, ros_reply_next (r), user_data));
} /* }}} int result_handler */

static void exit_usage (void) /* {{{ */
{
	printf ("Usage: ros-api [options] <host> <command> [args]\n"
			"\n"
			"OPTIONS:\n"
			"  -u <user>       Use <user> to authenticate.\n"
			"  -p <password>   Use <password> to authenticate.\n"
			"  -s              Suppress output.\n"
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
	char *passwd = NULL;
	const char *host;
	const char *command;

	int option;

	while ((option = getopt (argc, argv, "u:p:sh?")) != -1)
	{
		switch (option)
		{
			case 'u':
				opt_username = optarg;
				break;

			case 'p':
				passwd = optarg;
				break;
				
			case 's':
				opt_silent = 1;
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

	if (passwd == NULL && !opt_silent)
		passwd = read_password ();
	if (passwd == NULL)
		exit (EXIT_FAILURE);

	c = ros_connect (host, ROUTEROS_API_PORT, opt_username, passwd);
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
