#define STUFF_HPP_IMPLEMENTATION
#include "stuff.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>

void sigpipe_handler(int signal_num) {
	// assert(signal_num == SIGPIPE);
	// fprintf(stderr, "signal number %d\n", signal_num);
	assert(false);
}

// const int TotalMemory = 1024;
const int TotalMemory = 1024*1024*1024;
uint8_t Memory[TotalMemory];
int32_t FirstFreeByte = 0;

void *allocate(uint64_t NumBytes) {
	void *ResultPtr = &Memory[FirstFreeByte];
	FirstFreeByte += NumBytes;
	assert(FirstFreeByte <= TotalMemory);
	return ResultPtr;
}

struct book {
	const char *Author;
	const char *Title;
};

int main() {
	// test allocator
	// printf("test allocator\n");

	// uint8_t buffer[1024*1024*7];// = {0};

	// char *mem = (char *)allocate(5);
	// mem[0] = 'e';
	// mem[1] = 'e';
	// mem[2] = 'r';
	// mem[3] = '0';
	// mem[4] = '\0';
	// printf("mem: %s\n", mem);

	// const long unsigned BookSize = sizeof(book);
	// // printf("BookSize: %lu\n", BookSize);
	// book *GoodBook = (book *)allocate(BookSize);
	// GoodBook->Title = "Death Ship";
	// GoodBook->Author = "Bruno Traven";
	// printf("good book: \"%s\" by %s\n", GoodBook->Title, GoodBook->Author);

	// printf("mem: %s\n", mem);

	// // for(int i = 0; i < 1024*1024; ++i) {
	// // 	mem[i] = i;
	// // }

	uint8_t *Ptr = (uint8_t *)allocate(1024);

	// ::read(STDOUT_FILENO, mem, 1);

	return 0;

	// fd::BytesArray arr1; make_BytesArray(&arr1);
	// fd::BytesArray arr2; make_BytesArray(&arr2);

	// fd::copy_end(&arr1, "abc");
	// fd::copy_end(&arr2, &arr1);

	// fd::write(STDOUT_FILENO, "arr1: ", 6);
	// fd::write(STDOUT_FILENO, &arr1);
	// fd::write(STDOUT_FILENO, "\n", 1);

	// fd::write(STDOUT_FILENO, "arr2: ", 6);
	// fd::write(STDOUT_FILENO, &arr2);
	// fd::write(STDOUT_FILENO, "\n", 1);

	// return 0;

	// default action for SIGPIPE is to close the application. maybe we dont want this.
	struct sigaction a = {}; //@ why is 'struct' necessary?
	a.sa_handler = &sigpipe_handler;
	if(sigaction(SIGPIPE, &a, NULL) == -1) {
		fprintf(stderr, "error: sigaction() %d\n", errno);
	}
	// raise(SIGPIPE);

	// const int REQUEST_MAX_LENGTH = 1024;
	// uint8_t request_buffer[REQUEST_MAX_LENGTH] = {0};
	// int32_t request_length = 0;

	// const int RESPONSE_MAX_LENGTH = 1024;
	// uint8_t response_buffer[RESPONSE_MAX_LENGTH];
	// int32_t response_length = 0;

	fd::BytesArray requestBuffer; make_BytesArray(&requestBuffer);
	fd::BytesArray responseBuffer; make_BytesArray(&responseBuffer);
	fd::BytesArray inputFen; make_BytesArray(&inputFen);
	fd::BytesArray finalOutput; make_BytesArray(&finalOutput);

	int requestPipe[2];
	int responsePipe[2];

	if(pipe(requestPipe) == -1 || pipe(responsePipe) == -1) {
		assert(false);
	}

	const int requestReadFd = requestPipe[0];
	const int requestWriteFd = requestPipe[1];
	const int responseReadFd = responsePipe[0];
	const int responseWriteFd = responsePipe[1];

	// printf("%d, %d, %d, %d\n",
	// 	requestReadFd,
	// 	requestWriteFd,
	// 	responseReadFd,
	// 	responseWriteFd);

	pid_t pid = fork();
	if(pid == -1) {
		assert(false);
	} else if(pid == 0) {
		// child process

		dup2(requestReadFd, STDIN_FILENO);
		dup2(responseWriteFd, STDOUT_FILENO);

		close(requestReadFd);
		close(requestWriteFd);
		close(responseReadFd);
		close(responseWriteFd);

		char *const argv[] = {"./stockfish", NULL}; //@
		char *const envp[] = {NULL};
		execve(argv[0], argv, envp);

		assert(false);
	} else {
		// parent process

		close(requestReadFd);
		close(responseWriteFd);
	}

	fd::Error err;

	// read initial message from stockfish
	fd::read(responseReadFd, &responseBuffer);

	// fd::write(STDOUT_FILENO, "stockfish: ", 11);
	fd::write(STDOUT_FILENO, &responseBuffer);
	
	fd::write(requestWriteFd, "setoption name MultiPV value 500\n", 33);

	while(true) {
		fd::write(STDOUT_FILENO, "fen: ", 5);

		fd::read(STDIN_FILENO, &inputFen);

		// assert(inputFen.data[inputFen.len - 1] == '\n');
		// if(inputFen.len == 1) {
		// 	continue;
		// }

		fd::copy(&requestBuffer, "position fen ");
		fd::copy_end(&requestBuffer, &inputFen);
		assert(requestBuffer.data[requestBuffer.len-1] == '\n');
		fd::write(requestWriteFd, &requestBuffer);

		// fd::copy(&requestBuffer, "go depth 12\n");
		fd::copy(&requestBuffer, "go depth 6\n");
		// fd::copy(&requestBuffer, "go movetime 12000\n");
		// fd::copy(&requestBuffer, "go movetime 1000\n");
		fd::write(requestWriteFd, &requestBuffer);

		// sleep(12+1);

		// //@ this is a terrible way to read. there must be a better way than poll() to determine if we got everything
		// err = fd::read(responseReadFd, &responseBuffer);
		// if(err.status == fd::ERROR) {
		// 	if(err.type == fd::DIDNT_READ_ALL) {
		// 		fd::write(STDERR_FILENO, "error: couldnt get the whole reponse!\n", 37+1);
		// 		fprintf(stderr, "read %d bytes\n", responseBuffer.len);
		// 		continue;
		// 	} else {
		// 		fd::write(STDERR_FILENO, "error: response: unknown error\n", 30+1);
		// 		exit(EXIT_FAILURE);
		// 	}
		// }
		bool got_all = false;
		const char *read_until = "bestmove";
		int32_t read_until_len = c_str_len(read_until);

		ssize_t total_read = 0;
		do {
			ssize_t bytes_read = ::read(responseReadFd, (responseBuffer.data + total_read), (responseBuffer.MAX_LEN - total_read));
			assert(bytes_read != 0); // we get this if we ignore the pipe-signal from the kernel and read from the pipe that has no other end (file descriptors are all closed).
			assert(bytes_read != -1); // errno

			total_read += bytes_read;
			fprintf(stderr, "total read: %ld\n", total_read);

			if(forward_find_bm(responseBuffer.data, total_read, (const uint8_t *)read_until, read_until_len) != -1) {
				got_all = true;
			}
		} while((total_read < responseBuffer.MAX_LEN) && !got_all);

		responseBuffer.len = total_read; //@ int32_t and ssize_t

		// fd::write(STDOUT_FILENO, &responseBuffer);

		if(!got_all) {
			fprintf(stderr, "didnt got all\n");

			assert(!fd::is_empty_and_empty(responseReadFd));

			// fd::copy(&requestBuffer, "isready\n");
			// fd::write(requestWriteFd, &requestBuffer);
			// fd::read(responseReadFd, &responseBuffer);

			// fd::write(STDOUT_FILENO, "stockfish responded with: ", 26);
			// fd::write(STDOUT_FILENO, &responseBuffer);

			continue;
		} else {
			fprintf(stderr, "read all\n");
		}

		const char *s = "info multipv 1 ";
		int32_t s_len = c_str_len(s);
		int r = backward_find_bm(responseBuffer.data, responseBuffer.len, (const uint8_t *)s, s_len);
		if(r == -1) {
			assert(false);
		}
		int32_t index = r;

		// fd::write(STDOUT_FILENO, "last chunk: \n", 13);
		// fd::write(STDOUT_FILENO, (const char *)(responseBuffer.data + index), responseBuffer.len - index);

		while(true) {
			const char *score_str = "score cp ";
			int32_t score_str_len = c_str_len(score_str);
			r = forward_find_bm(responseBuffer.data + index, responseBuffer.len - index, (const uint8_t *)score_str, score_str_len);
			if(r == -1) {
				break;
			}
			index += r;
			index += score_str_len;

			char score_buffer[16];
			int score_buffer_index = 0;
			while(responseBuffer.data[index] != ' ') {
				assert(score_buffer_index < 16);
				score_buffer[score_buffer_index] = responseBuffer.data[index];
				score_buffer_index += 1;
				index += 1;
			}
			score_buffer[score_buffer_index] = '\0';
			// fprintf(stderr, "score buffer: %s\n", score_buffer);
			fd::copy_end(&finalOutput, score_buffer);
			fd::copy_end(&finalOutput, "\t");

			r = forward_find_bm(responseBuffer.data + index, responseBuffer.len - index, (const uint8_t *)"pv ", c_str_len("pv "));
			if(r == -1) {
				assert(false);
			}
			index += r;
			index += c_str_len("pv ");

			char move_buffer[16];
			int move_buffer_index = 0;
			while(responseBuffer.data[index] != ' ') {
				assert(move_buffer_index < 16);
				move_buffer[move_buffer_index] = responseBuffer.data[index];
				move_buffer_index += 1;
				index += 1;
			}
			move_buffer[move_buffer_index] = '\0';
			// fprintf(stderr, "move: %s\n", move_buffer);
			fd::copy_end(&finalOutput, move_buffer);
			fd::copy_end(&finalOutput, "\n");
		}

		// fd::write(STDOUT_FILENO, "final output: \n", 15);
		fd::write(STDOUT_FILENO, &finalOutput);
		finalOutput.len = 0;
		// // // fprintf(stderr, "final output again: %s\n", finalOutput.data);

		// fprintf(stderr, "read %d bytes\n", responseBuffer.len);


		// if(is_equal(request_buffer, request_length, (const uint8_t*)"quit\n", 5)) {
		// 	//@ the server should be able to detect that we left
		// 	fd::write(pipes.request.write, (const uint8_t*)"quit\n", 5);
		// 	write(STDERR_FILENO, "debug: client: sent a 'quit'. quitting...\n", 42);
		// 	break;
		// } else {
		// 	fd::write(pipes.request.write, request_buffer, request_length);
		// 	if(!fd::read(pipes.response.read, response_buffer, RESPONSE_MAX_LENGTH, &response_length)) {
		// 		fd::write(STDERR_FILENO, (const uint8_t *)"response: couldnt read all!\n", 28);
		// 		fd::write(STDERR_FILENO, response_buffer, response_length);
		// 	}

		// 	// do something with the response here

		// 	fd::write(STDOUT_FILENO, (const uint8_t *)"response: ", 10);
		// 	fd::write(STDOUT_FILENO, response_buffer, response_length);
		// 	fd::write(STDOUT_FILENO, (const uint8_t *)"\n", 1);

		// 	continue;
		// }
	}
	

	// munmap(app_data, sizeof(app_data));

	// close(client_write_pipe[WRITE_FD]);
	// close(client_read_pipe[READ_FD]);

	return 0;
}