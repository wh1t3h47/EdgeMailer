#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <uv.h>
#include <curl/curl.h>

void err_exit();

char *proxy = NULL; // CURLOPT_PROXY
uv_loop_t *event_loop;
CURLM *multi_handle; // allow multiple easy handles to be processed in batch
uv_timer_t timeout; // timer handle user to schedule callbacks to be called in the future
/* First you do a HELO handshake, then you just wait until timeout to check
 * how long it takes for a SMTP inactive connection to timeout and store.
 * This is just you verify if a bye was received *only* after N seconds, it
 * may be because edgeMailer took too long and we should not rely on the 
 * "fail" indicative
 */

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

void free_controller( void *arg ) { // we cast to void ptr because this is called by a callback
	socket_controller_t *controller = (socket_controller_t *) arg;
	free(controller);
	return;
}

void free_controller_callback( uv_handle_t *handle ) {
	free_controller(handle->data);
	return;
}

void dispose_controller( socket_controller_t *controller ) {
	uv_handle_t *uv_handle = (uv_handle_t *) &(controller->poll_handle);
	uv_close(uv_handle, free_controller_callback);
}


void parse_state() {
        // Start parsing the message based on a state machine
        int pending;
        CURLMsg *message;
	message = curl_multi_info_read(multi_handle, &pending);
	if (message == NULL) {
		return;
	}
	do {
		switch (message->msg) {
			case CURLMSG_DONE :;
			/* Do not use message data after calling 
			 * curl_multi_remove_handle() and curl_easy_cleanup().
			 * As per curl_multi_info_read() docs:
			 * "WARNING: The data the returned pointer points to
			 * will not survive calling curl_multi_cleanup,
			 * curl_multi_remove_handle or curl_easy_cleanup."
			 * You may want to copy the value into another place
			 */
					   CURL *easy = message->easy_handle;
					   // We can call curl_easy_getinfo() to check which handle was done
					   curl_multi_remove_handle(multi_handle, easy);
					   easy = NULL;
					   break;
			case CURLMSG_LAST :
					   printf("Last Msg\n"); //FIXME
					   break;
			case CURLMSG_NONE :
					   printf("CurlMSg none\n"); //FIXME
					   break;
		}
		message = curl_multi_info_read(multi_handle, &pending);
	} while (message != NULL);
	return;
}

void do_request( curl_socket_t socket_fd, short event, void *arg ) {
        // this function will kick in events and parse result states
	// Input: arg -> socket_controller having information associated
	// with a particular socket
	socket_controller_t *controller = (socket_controller_t *) arg;
        int running_handles;
        int flags = 0;

        if (event & UV_READABLE) { // translate UV flags to curl
                flags |= CURL_CSELECT_IN;
        }
        if (event & UV_WRITABLE) { // signaling to and updating curl about each fd state change
                flags |= CURL_CSELECT_OUT;
        } // this is necessary in order to have the right flags in parse_state()
	// tell libcurl what to do (as in event loop) based on each sockfd state
        curl_multi_socket_action(multi_handle, controller->socket_fd, flags, &running_handles);
        parse_state(multi_handle);
}

int timer_callback( CURLM *multi, long timeout, void *userp ) {
	// function to be registered with CURLMOPT_TIMERFUNCTION
	if(timeout_ms < 0) { // stop timer on timeout
		uv_timer_stop(&timeout);
	}
	else {
		if(timeout_ms == 0) {
			timeout_ms = 10; // 0 means directly call socket_action
		}                        // which is done a bit later
		uv_timer_start(&timeout, on_timeout, timeout_ms, 0);
	}
	return 0;
}

void on_timeout_callback( evutil_socket_t socket_fd, short events, void *arg ) {
        int running_handles;
	// indicate to parse_state that we were timed out
        curl_multi_socket_action(multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
        parse_state(multi_handle);
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

void err_exit( char *message ) {
	if ( message != NULL ) { // if caller specified custom message
		printf("%s\n", message);
	}
	else if ( errno != 0 ) { // we don't want to print "Success"
		char *msg;
		msg = strerror(errno); // fetch error message
		printf("%s\n", msg);
	}
	// FIXME Libuv clean up
	// FIXME Libcurl clean up
	exit(-1); // NOTE: exit frees memory automagically
}
