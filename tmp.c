int download_xml(char **pdata, int *plength) {
   char hostname[100];
	int sd;
	struct sockaddr_in pin;
	struct hostent *hp;
	char header[MAXBUF];
	char *pc;
	int  ret, n, length = -1;
	const char *get_msg = "GET /feed/eztv/ HTTP/1.0\r\nUser-Agent: bla/1.0\r\nHost: tvrss.net:80\r\nAccept: */*\r\n\r\n";

	if (!pdata) {
		return -8;
	} else {
		*pdata = NULL;
	}
	if (plength) {
		*plength = 0;
	}

	strcpy(hostname,"tvrss.net");

	/* go find out about the desired host machine */
	if ((hp = gethostbyname(hostname)) == 0) {
		perror("gethostbyname");
		exit(1);
	} else {
		dbg_printf("gethostbyname succeeded\n");
	}

	/* fill in the socket structure with host information */
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(80);

	/* grab an Internet domain socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	} else {
		setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, 0, 0);
		dbg_printf("socket bounded: %d\n", sd);
	}

	/* connect to PORT on HOST */
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("connect");
		exit(1);
	} else {
		dbg_printf("connect() succeeded\n");
	}

	/* send a message to the server PORT on machine HOST */
	if (send(sd, get_msg, strlen(get_msg), 0) == -1) {
		perror("send");
		return 1;
	} else {
		dbg_printf("GET sent\n");
	}

	/* get response */
	ret = read_line(sd, header, MAXBUF-1);
	dbg_printf("read bytes: %d\n", ret);
	sscanf(header,"HTTP/1.%*d %03d",(int*)&ret);
	dbg_printf("return val: %d\n", ret);
	if (ret == 200) {
		while (1) {
			n = read_line(sd,header,MAXBUF-1);
			if (n <= 0) {
				close(sd);
			return -1;
			}
			/* empty line ? (=> end of header) */
			if ( n > 0 && (*header)=='\0') {
				break;
			}
			dbg_printf("l: %s\n", header);
			/* try to parse some keywords : */
			/* convert to lower case 'till a : is found or end of string */
			for (pc = header; (*pc != ':' && *pc) ; pc++) {
				*pc = tolower(*pc);
			}
			sscanf(header,"content-length: %d",&length);
		}
		if (length <= 0) {
			dbg_printf("Could not determine content-length\n");
			close(sd);
			return -1;
		}
		if (plength) {
			*plength = length;
		}
		if (!(*pdata=malloc(length))) {
			perror("malloc");
			close(sd);
			return -1;
		}
		n = read_buffer(sd, *pdata, length);
		dbg_printf("read_buffer: %d\n", n);
		close(sd);
		if (n != length) {
			ret = -1;
		}
	} else if (ret >= 0) {
		close(sd);
	} else {
		fprintf(stderr, "header: %s\n", header);
	}
	return ret;
}

static int read_line (int fd, char *buffer, int max) {
  int read_bytes = 0;
	while (read_bytes < max) {
		if (read(fd,buffer,1) != 1) {
			read_bytes = -read_bytes;
			break;
		}
		read_bytes++;
		if (*buffer=='\r') continue; /* ignore CR */
		if (*buffer=='\n') break;    /* LF is the separator */
		buffer++;
	}
	*buffer = 0;
	return read_bytes;
}

static int read_buffer (int fd, char *buffer, int length) {
	int n,r;
	if(length > 0) {
		for (n = 0; n < length; n += r) {
			r = read(fd, buffer, 1024);
			if (r <= 0)
				return -n;
			buffer += r;
		}
	} else {
		n = 0;
		while(1) {
			r = read(fd, buffer, 1024);
			if (r <= 0) {
				return n;
			} else {
				n += r;
			}
			buffer += r;
		}
	}
	return n;
}
