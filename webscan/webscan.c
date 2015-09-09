#include "stdafx.h"
#include "keyboard.h"

#define RECBUF 42
#define HTTP_ER "HTTP/1.0 500\r\n" \
	"Connection: close\r\n\r\n" \
	"An error has occurred when processing "
#define HTTP_OK "HTTP/1.0 200\r\n" \
	"Connection: close\r\n\r\n"
#define HTTP_NS "HTTP/1.0 405 Method Not Allowed\r\n" \
	"Connection: close\r\n\r\n" \
	"Method Not Allowed: "
#define HTTP_HELP "HTTP/1.0 200\r\n" \
	"Connection: close\r\n" \
	"Content-Type: text/html\r\n\r\n" \
	"<h1>webscan help</h1><p>Just make short GET requests to type things.</p>" \
	"<p>Use ',n' and ',t' escape codes for Enter and Tab.</p>"


/**
 * Creates a TCP client/server socket.
 * @return -2 on getaddrinfo error
 * @return -1 on a socket API error
 * @return socket
 */
int get_tcp_socket(const char *host /**<host to connect to (name or IP)*/,
		const char *service /**<service name or port number*/,
		int server_backlog /**<listen backlog, 0 creates a client socket*/,
		socklen_t *addrlen /**<address length, can be NULL*/) {
	int sock=-1, e;
	const struct addrinfo hints = {server_backlog ? AI_PASSIVE : 0,
		AF_UNSPEC, SOCK_STREAM, 0, 0, NULL, NULL, NULL};
	struct addrinfo *res=NULL, *p;
	if (e = getaddrinfo(host, service, &hints, &res)) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(e));
		return -2;
	}
	for (p=res; p; p=p->ai_next) {
		if ((sock=socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			continue;
		else if ((server_backlog && (bind(sock, p->ai_addr, p->ai_addrlen)==-1 || listen(sock, server_backlog)==-1)) ||
				(!server_backlog && connect(sock, p->ai_addr, p->ai_addrlen)==-1)) {
			shutdown(sock, SHUT_RDWR);
			sock = -1;
		}
		else {
			if (addrlen)
				*addrlen = p->ai_addrlen;
			break;
		}
	}
	freeaddrinfo(res);
	return sock;
}

void handle_client(int sock, struct sockaddr_in *sa, socklen_t sal, int debug) {
	char addr[INET6_ADDRSTRLEN+6] = {0};
	char buf[RECBUF+1] = {0};
	char *p;
	int r, rem=RECBUF;
	//generate a client identifier
	inet_ntop(sa->sin_family, &sa->sin_addr, addr, INET6_ADDRSTRLEN);
	sprintf(addr+strlen(addr), ":%d", ntohs(sa->sin_port)); //FIXME no possible overflow?
	//process a single request (HTTP 1.0!)
	do {
		if ((r=recv(sock, buf+RECBUF-rem, rem, 0)) < 0)
			fprintf(stderr, "%s: recv failed with error code %d.\n", addr, WSAGetLastError());
		else
			rem -= r;
	} while (r>0 && rem>0);
	//locate EOL
	p = strstr(buf, "\r\n"); //we can't use strtok here since the delimiter is two chars long...
	if (!p)
		//FIXME it looks like Firefox tries to reconnect and send the bytes we've just ignored
		; //fprintf(stderr, "%s: too long URL or bad request.\n", addr);
	else {
		*p = 0; //terminate the first linebc
		if (!(p = strtok(buf, " ")) || !(p = strtok(NULL, " ")) || *p!='/')
			fprintf(stderr, "%s: non-HTTP request.\n", addr);
		else {
			if (!stricmp(buf, "GET")) {
				size_t len = strlen(p+1);
				if (strcmp(p+1, "favicon.ico") && len) {
					printf("%s%s\n", addr, p);
					Sleep(debug * 1000);
					if (type(p + 1, len))
						send(sock, HTTP_ER, strlen(HTTP_ER), 0);
					else
						send(sock, HTTP_OK, strlen(HTTP_OK), 0);
					send(sock, p, strlen(p), 0);
				}
				else
					send(sock, HTTP_HELP, strlen(HTTP_HELP), 0);
			}
			else {
				fprintf(stderr, "%s: %s, wut?\n", addr, buf);
				send(sock, HTTP_NS, strlen(HTTP_NS), 0);
				send(sock, buf, strlen(buf), 0);
			}
		}
	}
	//close the socket
	shutdown(sock, SHUT_WR); //FIXME leak?
}

static __inline void init_sockets() {
#ifdef WIN32
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 0), &wd))
		perror("WSAStartup");
#endif
}

int main(int argc, char *argv[]) {
	char *prot = "81", *a0;
	int debug = 0;
	int sock;
	socklen_t sal;
	a0 = argv[0];
	if (setup()) {
		//FIXME better message
		fprintf(stderr, "virtual keyboard setup has failed\n");
		return 3;
	}
	init_sockets();
	while (--argc) {
		char *p = *(++argv);
		if (*p == '-') {
			int was_v = 0;
			while (*(++p) == 'd')
				was_v = ++debug;
			if (was_v)
				continue;
			if (!strcmp(p, "p")) {
				if (argc < 2) {
					fprintf(stderr, "missing parameter for p\n");
					return 2;
				}
				//consume the next argument
				prot = *(++argv);
				argc--;
			}
			else if (!strcmp(p, "h")) {
				printf("usage: %s [-h] [-d+] [-p port]\n", a0);
				return 0;
			}
			else {
				fprintf(stderr, "unknown switch %c\n", *p);
				return 2;
			}
		}
		else {
			fprintf(stderr, "useless parameter\n");
			return 2;
		}
	}
	//FIXME use a kind of universal address to bind on IPv6 addresses, too
	if ((sock=get_tcp_socket("0.0.0.0", prot, 10, &sal)) < 0) {
		if (sock == -1)
			perror("get_tcp_socket");
		return 1;
	}
	else {
		int cli;
		struct sockaddr_in *sa = malloc(sal);
		if (sa) {
			printf("Listening on port %s...\n", prot);
			while ((cli=accept(sock, (struct sockaddr*)sa, &sal)) != -1)
				handle_client(cli, sa, sal, debug);
			shutdown(sock, SHUT_RDWR);
			return 0;
		}
		else {
			perror("malloc");
			return 1;
		}
	}
}