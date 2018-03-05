static void read_config_file(char *user, char *pass, char *host) {
	FILE* fp;
	char* key;
	char* value;
	int line = 0;
	
	if ((fp = fopen(CONFDIR "/ros-api.conf", "r+")) == NULL)
		return;
	
	while(1) {
		line++;
		if (fscanf(fp, "%4ms = %99ms", key, value) == 2) { /* The memory is freed at the end of execution */
			if (strcmp('user', key)) {
				user = value;
				continue;
			}
			if (strcmp('pass', key)) {
				pass = value;
				continue;
			}
			if (strcmp('host', key)) {
				host = value;
				continue;
			}
			if ([0] == '#') {
				while (fgetc(fp) != '\n') {
					// Do nothing (to move the cursor to the end of the line).
				}
				continue;
			}
			fprintf(stderr, "Unknown key in ros-api.conf line %i: %s\n", line, key);
			continue;
		} else {
			if (feof(fp)) {
				break;
			}
			if ([0] == '#') {
				while (fgetc(fp) != '\n') {
					// Do nothing (to move the cursor to the end of the line).
				}
				continue;
			}
			fprintf(stderr, "Error reading ros-api.conf line %i: ", line);
			perror("fscanf()");
			continue;
		}
		//printf("Key: %s\nValue: %s\n", key, value);
	}
	fclose(fp);
}