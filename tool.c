#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <uv.h>
#include <curl/curl.h>

char *proxy = NULL;
void err_exit();
bool is_allocated;

typedef struct socket_controller {
	/* TODO poll_handle description
	 * socket_fd selects a specific socket in CURL Multi handle
	 * to monitor it for any state change with libuv
	 */
	uv_poll_t poll_handle;
	curl_socket_t socket_fd;
} socket_controller_t;

int main( int argc, char *argv[] ) {
	if ( argc > 0 ) {
		if ( argc == 1 ) { // First optional arg is proxy
			proxy = argv[1];
		}
		else {
			errno = E2BIG; // Argument list too long
			err_exit();
		}
	}
}

void err_exit() {
	if ( errno != 0 ) {
		char *msg;
		msg = strerror(errno);
		printf("%s\n", msg);
	}
	// FIXME Libuv clean up
	// FIXME Libcurl clean up
	exit(-1);
}
