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
