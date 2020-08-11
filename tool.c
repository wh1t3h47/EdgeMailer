#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <uv.h>
#include <curl/curl.h>

char *proxy = NULL; // CURLOPT_PROXY
void err_exit();

typedef struct socket_controller {
	/* TODO poll_handle description
	 * socket_fd selects a specific socket in CURL Multi handle
	 * to monitor it for any state change with libuv
	 */
	uv_poll_t poll_handle;
	curl_socket_t socket_fd;
} socket_controller_t;

socket_controller_t *create_controller() {
	socket_controller_t *controller;
	controller = malloc( sizeof(*controller) );
	if ( controller == NULL ) { // malloc returns NULL on fail or when malloc(0)
		err_exit("Malloc failed: Out of memory");
	}
	return controller;
}

void free_controller(void *arg) { // we cast to void ptr because this is *ALSO* a callback
	socket_controller_t *controller = (socket_controller_t *) arg;
	free(controller);
}

int main( int argc, char *argv[] ) {
	if ( argc > 0 ) {
		if ( argc == 1 ) { // First optional arg is proxy
			proxy = argv[1];
		}
		else {
			errno = E2BIG; // Argument list too long
			err_exit(NULL);
		}
	}
}

void err_exit(char *message) {
	if ( message != NULL ) { // if caller specified custom message
		printf("%s\n", message);
	}
	else if ( errno != 0 ) { // we don't wanne print "Success"
		char *msg;
		msg = strerror(errno); // fetch error message
		printf("%s\n", msg);
	}
	// FIXME Libuv clean up
	// FIXME Libcurl clean up
	exit(-1); // exit frees memory automagically
}
