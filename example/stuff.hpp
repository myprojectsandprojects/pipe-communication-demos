#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>

// inline int min(int a, int b) {
// 	return (a < b) ? a : b;
// }

// type definitions and function declarations:

namespace fd {

	enum Status {
		OK, ERROR
	};

	enum ErrorType {
		UNKNOWN,
		DIDNT_READ_ALL,
		SYS_ERROR
		// ...
	};

	// struct MySpecialError {
	// 	ErrorType type;
	// 	int code;
	// 	char *msg;
	// };

	struct Error {
		Status status;
		ErrorType type;
		/*
		union {
			ErrorType type; // minimal
			MySpecialError mySpecialError;
		}
		*/
	};

	//@ probably not a good idea to store the data inside 'BytesArray'
	struct BytesArray {
		// static const int32_t MAX_LEN = 9;
		static const int32_t MAX_LEN = 1024;
		uint8_t data[MAX_LEN];
		int32_t len;
	};
	void make_BytesArray(BytesArray *arr);

	Error write(int fd, BytesArray *bytes);
	Error write(int fd, const char *data, int32_t data_count);
	Error read(int fd, BytesArray *arr);
	void copy(BytesArray *arr, const char *c_str);
	void copy_end(BytesArray *arr, const char *c_str);
	bool is_equal(BytesArray *arr, const char *c_str);
}


// function implementations:
#ifdef STUFF_HPP_IMPLEMENTATION

namespace fd {

void make_BytesArray(BytesArray *arr) {
	arr->len = 0;
}

void copy(BytesArray *arr, const char *c_str) {
	arr->len = 0;

	for(int i = 0; c_str[i] != '\0' && i < BytesArray::MAX_LEN; ++i) {
		arr->data[i] = c_str[i];
		arr->len += 1;
	}
}

void copy_end(BytesArray *arr, const char *c_str) {
	for(int i = 0, j = arr->len; c_str[i] != '\0' && j < BytesArray::MAX_LEN; ++i, ++j) {
		arr->data[j] = c_str[i];
		arr->len += 1;
	}
}

// int32_t c_str_len(const char *c_str) {
// 	assert(c_str != NULL);

// 	int32_t count = 0;

// 	while(c_str[count] != '\0') {
// 		count += 1;
// 	}

// 	return count;
// }

//@ if c_str is longer than arr, we are reading past the end of the array.
bool is_equal(BytesArray *arr, const char *c_str) {
	int i = 0;

	while(c_str[i] != '\0') {
		if(arr->data[i] != c_str[i]) {
			return false;
		}
		i += 1;
	}

	if(arr->len != i) {
		return false;
	}

	return true;
}

Error write(int fd, BytesArray *bytes) {
	return write(fd, (const char *)bytes->data, bytes->len);
}

Error write(int fd, const char *data, int32_t data_count) {
	Error err;
	err.status = OK;

	int32_t total_written = 0;

	while(total_written < data_count) {
		int unwritten = data_count - total_written;

		ssize_t written = ::write(fd, data + total_written, unwritten);
		// assert(written != -1); // errno
		if(written == -1) {
			err.status = ERROR;
			return err;
		}

		total_written += written;
	}

	return err;
}

// reads bytes from a given fd until none left.
// if fd had no bytes to begin with, returns true, otherwise false.
bool is_empty_and_empty(int fd) {
	bool was_empty = true;

	struct pollfd pfd = {};
	pfd.fd = fd;
	pfd.events = POLLIN;

	int num_fds = poll(&pfd, 1, 0); assert(num_fds != -1);
	if(pfd.revents & POLLIN) {
		was_empty = false;

		const int32_t BUF_MAX = 1024;
		char buf[BUF_MAX];
		
		while(pfd.revents & POLLIN) {
			ssize_t bytes_read = ::read(fd, buf, BUF_MAX);
			if(bytes_read == -1) {
				assert(false);
			}

			num_fds = poll(&pfd, 1, 0); assert(num_fds != -1);
		}
	}

	return was_empty;
}

// read from 'fd' into a fixed sized buffer.
// because buffer size is fixed, it may not read everything (returns false)
Error read(int fd, BytesArray *arr) {
	Error err;
	err.status = OK;

	struct pollfd pfd = {};
	pfd.fd = fd;
	pfd.events = POLLIN;

	ssize_t total_read = 0;
	do {
		ssize_t bytes_read = ::read(fd, (arr->data + total_read), (arr->MAX_LEN - total_read));
		assert(bytes_read != 0); // we get this if we ignore the pipe-signal from the kernel and read from the pipe that has no other end (file descriptors are all closed).
		assert(bytes_read != -1); // errno

		total_read += bytes_read;

		int num_fds = poll(&pfd, 1, 0); assert(num_fds != -1);
	} while((pfd.revents & POLLIN) && (total_read < arr->MAX_LEN));

	arr->len = total_read;

	if(!is_empty_and_empty(fd)) {
		err.status = ERROR;
		err.type = DIDNT_READ_ALL;
	}

	return err;
}
} // namespace fd

#endif