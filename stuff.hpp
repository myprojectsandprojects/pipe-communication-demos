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

int32_t c_str_len(const char *c_str);
int32_t forward_find(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len);
int32_t backward_find(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len);
int32_t forward_find_bm(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len);
int32_t backward_find_bm(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len);

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
		static const int32_t MAX_LEN = 1024*1024;
		// static const int32_t MAX_LEN = 1024;
		uint8_t data[MAX_LEN];
		int32_t len;
	};
	void make_BytesArray(BytesArray *arr);

	Error write(int fd, BytesArray *bytes);
	Error write(int fd, const char *data, int32_t data_count);
	Error read(int fd, BytesArray *arr);
	bool is_empty_and_empty(int fd);

	// copy, copy_at, copy_at_end, copy_at_start
	void copy(BytesArray *arr, const char *c_str);
	void copy_end(BytesArray *arr, const char *c_str);
	void copy_end(BytesArray *dest, BytesArray *src);
	// void copy_end(BytesArray *dest, const uint8_t *src, int32_t src_len);
	bool is_equal(BytesArray *arr, const char *c_str);
}


// function implementations:
#ifdef STUFF_HPP_IMPLEMENTATION

int32_t c_str_len(const char *c_str) {
	assert(c_str != NULL);

	int32_t count = 0;

	while(c_str[count] != '\0') {
		count += 1;
	}

	return count;
}

int32_t forward_find(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len) {
	assert(needle_len > 0);

	for(int i = 0; i <= (haystack_len - needle_len); ++i) {
		for(int j = 0; needle[j] == haystack[i+j]; ++j) {
			if(j+1 == needle_len) {
				return i;
			}
		}
	}

	return -1;
}

int32_t backward_find(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len) {
	assert(needle_len > 0);

	for(int i = (haystack_len - needle_len); i > -1; --i) {
		for(int j = 0; needle[j] == haystack[i+j]; ++j) {
			if(j+1 == needle_len) {
				return i;
			}
		}
	}

	return -1;
}

// Boyer-Moore
int32_t forward_find_bm(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len) {
	assert(needle_len > 0);

	int values_table[256];
	for(int i = 0; i < 256; ++i) {
		values_table[i] = needle_len;
	}
	values_table[needle[needle_len-1]] = needle_len; //overwrite if same value appears also before
	for(int i = 0; i < needle_len - 1; ++i) { //exclude last
		values_table[needle[i]] = needle_len - 1 - i;
	}
	// values_table[needle[needle_len-1]] = needle_len; //@ but is this really correct?
	// for(int i = 0; i < 256; ++i) {
	// 	printf("%d (%c): %d\n", i, i, values_table[i]);
	// }

	int i = -1; i += needle_len;
	while(i < haystack_len) {
		// printf("%c\n", haystack[i]);

		for(int j = 0; needle[needle_len-1-j] == haystack[i-j]; ++j) {
			if(j+1 == needle_len) {
				return i - (needle_len - 1);
			}
		}

		i += values_table[haystack[i]];
	}

	return -1;
}

// Boyer-Moore
int32_t backward_find_bm(const uint8_t *haystack, int32_t haystack_len, const uint8_t *needle, int32_t needle_len) {
	assert(needle_len > 0);

	int values_table[256];
	for(int i = 0; i < 256; ++i) {
		values_table[i] = needle_len;
	}
	values_table[needle[0]] = needle_len; //overwrite if same value appears also after
	for(int i = needle_len - 1; i > 0; --i) { //exclude first
		values_table[needle[i]] = i;
	}
	// values_table[needle[needle_len-1]] = needle_len; //@ but is this really correct?
	// for(int i = 0; i < 256; ++i) {
	// 	printf("%d (%c): %d\n", i, i, values_table[i]);
	// }

	int i = haystack_len; i -= needle_len;
	while(i > -1) {
		// printf("%c\n", haystack[i]);

		for(int j = 0; needle[j] == haystack[i+j]; ++j) {
			if(j+1 == needle_len) {
				return i;
			}
		}

		i -= values_table[haystack[i]];
	}

	return -1;
}

typedef int32_t (FindFunc) (const uint8_t *, int32_t, const uint8_t *, int32_t);

void test(FindFunc *func, const char *haystack, const char *needle, int32_t expected) {
	int32_t haystack_len = c_str_len(haystack);
	int32_t needle_len = c_str_len(needle);
	int32_t index = func((const uint8_t *)haystack, haystack_len, (const uint8_t *)needle, needle_len);

	if(index == expected) {
		printf("\"%s\" in \"%s\" @ %d -- OK\n", needle, haystack, expected);
	} else {
		printf("\"%s\" in \"%s\" @ %d (but index given: %d) -- FAILED\n", needle, haystack, expected, index);
	}
}

// for/back-ward agnostic cases
void test_find(FindFunc *func_ptr) {
	test(func_ptr, "defghi", "abc", -1);
	test(func_ptr, "defghi", "ab", -1);
	test(func_ptr, "defghi", "a", -1);
	// test(func_ptr, "defghi", "", -1);

	test(func_ptr, "abcdefghi", "abc", 0);
	test(func_ptr, "dabcefghi", "abc", 1);
	test(func_ptr, "deabcfghi", "abc", 2);
	test(func_ptr, "defabcghi", "abc", 3);
	test(func_ptr, "defgabchi", "abc", 4);
	test(func_ptr, "defghabci", "abc", 5);
	test(func_ptr, "defghiabc", "abc", 6);

	test(func_ptr, "abab", "abab", 0);
	test(func_ptr, "aabab", "abab", 1);
	test(func_ptr, "ababb", "abab", 0);

	test(func_ptr, "", "abc", -1);
	test(func_ptr, "a", "abc", -1);
	test(func_ptr, "ab", "abc", -1);
	test(func_ptr, "abc", "abc", 0);
}

void test_forward_find(FindFunc *func_ptr) {
	test_find(func_ptr);

	// forward specific cases:
	test(func_ptr, "abc123abc", "abc", 0);
	test(func_ptr, "xxxx", "xxx", 0);

	test(func_ptr, "cccbbcabc", "abc", 6);
	test(func_ptr, "bbab", "ab", 2);
}

void test_backward_find(FindFunc *func_ptr) {
	test_find(func_ptr);
	
	// backward specific cases:
	test(func_ptr, "abc123abc", "abc", 6);
	test(func_ptr, "xxxx", "xxx", 1);

	test(func_ptr, "abcabbacc", "abc", 0);
	test(func_ptr, "abaa", "ab", 0);
}

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

void copy_end(BytesArray *dest, BytesArray *src) {
	for(int i = 0, j = dest->len; i < src->len && dest->len < BytesArray::MAX_LEN; ++i, ++j) {
		dest->data[j] = src->data[i];
		dest->len += 1;
	}
}

// void copy_end(BytesArray *dest, const uint8_t *src, int32_t src_len) {
// 	for(int i = 0, j = dest->len; i < src_len && dest->len < BytesArray::MAX_LEN; ++i, ++j) {
// 		dest->data[j] = src[i];
// 		dest->len += 1;
// 	}
// }

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
//@ because the data might not arrive all at once, it's real important to choose a large enough timeout value for poll(), so that we wont leave unread data on the descriptor. on the other hand, because we want to empty the data as quickly as possible, we dont want this value to be unreasonably large either.
bool is_empty_and_empty(int fd) {
	bool was_empty = true;

	struct pollfd pfd = {};
	pfd.fd = fd;
	pfd.events = POLLIN;

	const int timeout = 1000;

	int num_fds = ::poll(&pfd, 1, timeout); assert(num_fds != -1);
	if(pfd.revents & POLLIN) {
		was_empty = false;

		const int32_t BUF_MAX = 1024;
		char buf[BUF_MAX];

		while(pfd.revents & POLLIN) {
			ssize_t bytes_read = ::read(fd, buf, BUF_MAX);
			if(bytes_read == -1) {
				assert(false);
			}

			num_fds = ::poll(&pfd, 1, timeout); assert(num_fds != -1);
		}
	}

	return was_empty;
}

// read from 'fd' into a fixed sized buffer.
// because buffer size is fixed, it may not read everything
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

		int num_fds = ::poll(&pfd, 1, 10); assert(num_fds != -1);
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