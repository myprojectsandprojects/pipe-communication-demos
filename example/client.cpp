#define STUFF_HPP_IMPLEMENTATION
#include "stuff.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <poll.h>
// #include <sys/mman.h>
#include <errno.h>
#include <signal.h>

// #define COMMAND_MAX_LEN 1024
// #define RESPONSE_MAX_LEN 1024

// struct AppData {
// 	uint8_t command_buf[COMMAND_MAX_LEN];
// 	int32_t command_len;
// 	uint8_t response_buf[COMMAND_MAX_LEN];
// 	int32_t response_len;
// };

void sigpipe_handler(int signal_num) {
	assert(signal_num == SIGPIPE);
	fprintf(stderr, "signal number %d\n", signal_num);
}

int main() {
	// default action for SIGPIPE is to close the application. maybe we dont want this.
	struct sigaction a = {}; //@ why is 'struct' necessary?
	a.sa_handler = &sigpipe_handler;
	if(sigaction(SIGPIPE, &a, NULL) == -1) {
		fprintf(stderr, "error: sigaction() %d\n", errno);
	}
	// raise(SIGPIPE);

	// AppData *app_data = (AppData *)mmap(
	// 	NULL,                        // adress
	// 	sizeof(AppData),             // length
	// 	PROT_READ | PROT_WRITE,      // prot
	// 	MAP_PRIVATE | MAP_ANONYMOUS, // flags
	// 	-1,                          // fd
	// 	0);                          // offset

	// BytesArray test
	// fd::BytesArray myBytesArray; fd::make_into_bytes_array(&myBytesArray);

	// fd::copy_end(&myBytesArray, "Hola mundo!\n");
	// fd::write(STDOUT_FILENO, &myBytesArray);
	// fd::write(STDOUT_FILENO, "Hello world!\n", 13);

	// bool result = fd::read(STDIN_FILENO, &myBytesArray);
	// if(!result) {
	// 	fd::write(STDOUT_FILENO, "error: too much data!\n", 22);
	// }
	// fd::write(STDOUT_FILENO, "you wrote: ", 11);
	// fd::write(STDOUT_FILENO, &myBytesArray);


	fd::BytesArray requestBuffer; fd::make_BytesArray(&requestBuffer);
	fd::BytesArray responseBuffer; fd::make_BytesArray(&responseBuffer);

	//@ is this going to work on all architectures?
	/* 2 pipes. 1 is used by the client to send request to the server
	and another is used by the server to send responses back to the client. */
	struct {
		struct {
			int read;
			int write;
		} request;
		struct {
			int read;
			int write;
		} response;
	} pipes;// = {};

	if(pipe((int *)&pipes.request) == -1 || pipe((int *)&pipes.response) == -1) {
		exit(EXIT_FAILURE);
	}

	// printf("%d, %d, %d, %d\n",
	// 	pipes.request.read,
	// 	pipes.request.write,
	// 	pipes.response.read,
	// 	pipes.response.write);

	pid_t pid = fork();
	if(pid == -1) {
		fprintf(stderr, "error: fork()\n"); // errno
		exit(EXIT_FAILURE);
	} else if(pid == 0) {
		// we are the child process

		dup2(pipes.request.read, STDIN_FILENO);
		dup2(pipes.response.write, STDOUT_FILENO);

		close(pipes.request.read);
		close(pipes.request.write);
		close(pipes.response.read);
		close(pipes.response.write);

		// char *const argv[] = {"./stockfish", NULL}; //@
		char *const argv[] = {"./server", NULL}; //@
		char *const envp[] = {NULL};
		execve(argv[0], argv, envp);

		fprintf(stderr, "error: execve()\n"); // errno
		exit(EXIT_FAILURE);
	} else {
		// we are the parent process

		//@ if the child process failed to exec() the server, we are perfectly oblivious about this.

		close(pipes.request.read);
		close(pipes.response.write);
	}

	while(true) {
		fd::Error err;

		err = fd::write(STDOUT_FILENO, "> ", 2);
		if(err.status == fd::ERROR) {
			assert(false);
		}

		err = fd::read(STDIN_FILENO, &requestBuffer);
		if(err.status == fd::ERROR) {
			if(err.type == fd::DIDNT_READ_ALL) {
				fd::write(STDERR_FILENO, "error: command too long!\n", 24+1);
				continue;
			} else {
				assert(false);
			}
		}

		assert(requestBuffer.data[requestBuffer.len - 1] == '\n');
		if(requestBuffer.len == 1) {
			continue;
		}

		// if(fd::is_equal(requestBuffer.data, requestBuffer.len, (const uint8_t*)"quit\n", 5)) {
		// 	fd::write(pipes.request.write, "quit\n", 5);
		// 	fd::write(STDERR_FILENO, "debug: client: sent a 'quit'. quitting...\n", 42);
		// 	break;
		// } else {
		// 	fd::write(pipes.request.write, &requestBuffer);

		// 	err = fd::read(pipes.response.read, &responseBuffer);
		// 	if(err.status == fd::ERROR) {
		// 		if(err.type == fd::DIDNT_READ_ALL) {
		// 			fd::write(STDERR_FILENO, "response: error: didnt read all!\n", 32+1);
		// 			continue;
		// 		} else {
		// 			assert(false);
		// 		}
		// 	}

		// 	// do something with the response here

		// 	fd::write(STDOUT_FILENO, "response: ", 10);
		// 	fd::write(STDOUT_FILENO, &responseBuffer);
		// 	fd::write(STDOUT_FILENO, "\n", 1);

		// 	// continue;
		// }

		fd::write(pipes.request.write, &requestBuffer);

		if(fd::is_equal(&requestBuffer, "quit\n")) {
			fd::write(STDERR_FILENO, "debug: client: sent a 'quit'. quitting....\n", 42+1);
			break;
		}

		err = fd::read(pipes.response.read, &responseBuffer);
		if(err.status == fd::ERROR) {
			if(err.type == fd::DIDNT_READ_ALL) {
				fd::write(STDERR_FILENO, "response: error: didnt read all!\n", 32+1);
				continue;
			} else {
				assert(false);
			}
		}

		// do something with the response here

		fd::write(STDOUT_FILENO, "response: ", 10);
		fd::write(STDOUT_FILENO, &responseBuffer);
		fd::write(STDOUT_FILENO, "\n", 1);
	}

	// munmap(app_data, sizeof(app_data));

	// close(client_write_pipe[WRITE_FD]);
	// close(client_read_pipe[READ_FD]);

	return 0;
}