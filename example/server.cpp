#define STUFF_HPP_IMPLEMENTATION
#include "stuff.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

using namespace fd;

int main() {
	// default action for SIGPIPE is to close the application. maybe we dont want this.
	struct sigaction act = {}; //@ why 'struct' and '{0}' ?
	act.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &act, NULL) == -1) {
		fprintf(stderr, "error: sigaction() %d\n", errno);
		exit(EXIT_FAILURE);
	}

	fd::BytesArray requestBuffer; fd::make_BytesArray(&requestBuffer);

	// const char *str;
	// str = "abc"; printf("%s -- %d\n", str, c_str_len(str));
	// str = "Hello world!"; printf("%s -- %d\n", str, c_str_len(str));
	// str = ""; printf("%s -- %d\n", str, c_str_len(str));
	// str = NULL; printf("%s -- %d\n", str, c_str_len(str));

	// copy(&requestBuffer, "abc"); if(is_equal(&requestBuffer, "abc")) {printf("correct\n");} else {printf("not correct\n");}
	// copy(&requestBuffer, "123"); if(is_equal(&requestBuffer, "abc")) {printf("not correct\n");} else {printf("correct\n");}
	// copy(&requestBuffer, "1234"); if(is_equal(&requestBuffer, "123")) {printf("not correct\n");} else {printf("correct\n");}
	// copy(&requestBuffer, "123"); if(is_equal(&requestBuffer, "1234")) {printf("not correct\n");} else {printf("correct\n");}
	// copy(&requestBuffer, ""); if(is_equal(&requestBuffer, "1234")) {printf("not correct\n");} else {printf("correct\n");}
	// copy(&requestBuffer, ""); if(is_equal(&requestBuffer, "")) {printf("correct\n");} else {printf("not correct\n");}

	while(true) {
		Error err;
		
		// if(!read(STDIN_FILENO, &requestBuffer)) {
		// 	write(STDOUT_FILENO, "request too long\n", 17);
		// 	continue;
		// }
		err = fd::read(STDIN_FILENO, &requestBuffer);
		if(err.status == fd::ERROR) {
			if(err.type == fd::DIDNT_READ_ALL) {
				fd::write(STDERR_FILENO, "request: error: didnt read all!\n", 32);
				continue;
			} else {
				assert(false);
			}
		}

		if(is_equal(&requestBuffer, "meaning\n")) {
			write(STDOUT_FILENO, "fourty two\n", 11);
		} else if(is_equal(&requestBuffer, "quit\n")) {
			write(STDERR_FILENO, "debug: server: received a 'quit'. quitting...\n", 46);
			break;
		} else if(is_equal(&requestBuffer, "help\n")) {
			//@ on the client end we SOMETIMES have a problem receiving this
			// we are not reading the whole thing at once
			// come to think of it, I think its only the 2nd write
			// and I dont think we should handle them as 1 message at all
			write(STDOUT_FILENO, "\n.:: HELP ::.\n\nquit -- quits the program\nmeaning --", 51);
			write(STDOUT_FILENO, "responds with 'fourty two'\n", 27);
		} else {
			write(STDOUT_FILENO, "unknown command\n", 16);
		}
	}

	return 0;
}