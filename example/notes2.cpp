

/*
struct PageAllocator {
	const int32_t TOTAL_NUM_PAGES;
	int32_t total_mem = 4096 * TOTAL_NUM_PAGES;

};

init_allocator(BlockAllocator *alloc) {
	pagesize = get_pagesize();
	alloc->mem = mmap(TOTAL_NUM_PAGES * pagesize);
}

void *allocator_get_block() {
	// pick first empty block
}

allocator_free_block(void *block) {
	// find block and mark as free
}

allocator manages "a big chunk" of memory, chops it into smaller (page sized pieces) and hands it out to static buffers. maybe an allocator has some "piece size" idea that can be set when an allocator is initialized, but let's make it a multiple of page size because mmap()'s memory is allocated in pages.


dynamic buffer vs static buffer

struct Buffer {
	BlockAllocator *alloc;

	const int32_t BUFFER_LEN_MAX;

	const uint8_t *data;
	uint8_t data[BUFFER_LEN_MAX];

	int32_t len;
};

Buffer make_buffer(num_pages, allocator) {
	void *mem = alloc.get_pages(size)
	StaticBuffer buffer;
	buffer.data = mem;
	buffer.len = 0;
	buffer.max_len = size;

	return buffer;
}

But, hey, make_buffer() could fail, couldnt it?! What if we dont get the memory?
enum Status {
	OK,
	ERROR,
};
enum ErrorType {
	SYS_ERROR,
	OUR_SPECIAL_ERROR,
	//...
};
struct SysError {
	int errno;
	char *msg;
};
struct OurSpecialError {};
struct Error {
	ErrorType type;
	union {
		SysError sys_err;
		OurSpecialError our_err;
		//...
	};
};
struct Result {
	Status status;
	Error error;
};
Buffer buffer;
Result res = make_buffer(num_pages, allocator, &buffer);
if(res.status == ERROR) {
	if(res.error.type == OUR_SPECIAL_ERROR) {
		// handle special error
	} else {
		handle_error(res.error);
	}
}

void handle_error(Error err, bool is_recoverable) {
	switch(err.type) {
		case  SYSCALL: {
			fprintf(stderr, "error: %s\n", err.syscall.msg);
		}
	}

	if(!is_recoverable) {
		exit(EXIT_FAILURE);
	}
}

Allocator my_allocator = MakeBlockAllocator();
StaticBuffer my_s_buffer = make_static_buffer(1024, my_allocator);
DynamicBuffer my_d_buffer = make_dynamic_buffer(1024, my_allocator); // grows if necessary

my_buffer.append(data)
data = my_buffer.append()

fd::Buffer my_buffer = {.data="Hello, hello!\n", .len=13+1};

fd::Buffer input = {};
fd::read(STDIN_FILENO, &my_buffer)
*/

/*
errno
char *strerror(int errnum)
void perror(const char *)

my_func3() {
	ssize_t r = write();
	if(r == -1) {
		Result res;
		// fills in
		prepend("my_func3(): ", res.msg);
		return res;
	}
}

my_func2() {
	Result res = my_func3();
	if(res.status == SYS_ERROR) {
		prepend("my_func2(): ", res.msg);
		return res; // forward to the caller
	}
}

my_func1() {
	Result res = my_func2();
	if(res.status == SYS_ERROR) {
		fprintf(stderr, "my_func1(): %s\n", res.sys_error.msg);
		exit(EXIT_FAILURE);
	}
}
*/

fd::StaticBuffer request_buffer = {}; request_buffer.capacity = 1024
fd::DynamicBuffer request_buffer = {};

// using namespace fd;

fd::Result res = fd::read_all(STDIN_FILENO, &request_buffer);
if(res.status == fd::Result::Status::ERROR) {
	// error handling
	fd::Buffer out = {.data="got an error when reading from STDIN_FILENO: ", .len=45};
	fd::write(STDERR_FILENO, out);
	fd::write(STDERR_FILENO, res.error);
} else if(res.status == fd::Result::Status::TOO_MUCH_DATA) {
	fd::Buffer out = {.data = "error: couldnt read all!\n", .len = 24+1};
	fd::write(STDOUT_FILENO, out);
} else {
	fd::write(STDOUT_FILENO, request_buffer.data, request_buffer.len)
}

fd::Result res = fd::read(STDIN_FILENO, request_buffer, REQUEST_MAX_LENGTH, &request_length);
if (res.status != fd::Status::OK) {
	if (res.status == fd::Status::TOO_MUCH_DATA) {
		fd::write(STDOUT_FILENO, "didnt read all!\n", 15+1);

		const char *msg = "didnt read all!\n";
		fd::Buffer output;
		output.data = msg;
		output.length = strlen(msg);

		fd::write(STDOUT_FILENO, output, output_length);
	} else {
		if (res == fd::Status::ERROR_READ) {
			fd::write(STDERR, "read error\n", 11);
		} else if(res == fd::Status::ERROR_POLL) {
			fd::write(STDERR, "poll error\n", 11);
		} else {
			assert(false);
			fd::write(STDERR, "unknown error\n", 14);
		}
	}
}

---
/*
how much memory in total?
things need memory for a limited timespan:
	program runtime (global?)
	function scope (stack)
	something else

alloc() and dealloc() -- re-using memory

sometimes we dont know at compile time how much memory we want, so we allocate at runtime

but we always know how much is available in total and this is a finite number

even if we are swapping pages out to disk, at some point we are going to run out of disk space
and possibly we might never want to use that much memory because the performance becomes terrible

so why not set a hard limit: this is how much memory we are ever going to need at any given time
*/