#pragma once
namespace StandardValue {
	enum {
		bits_in_byte = 8,
		array_begin = 0,
	};
};
namespace ReturnValue {
	enum standard {
	    nice = 0,
	};
    enum exec {
	    exec_fail = -1,
	};
    enum mmap {
		mmap_failed,
	};
    enum fork {
		fork_fail = -1,
		fork_child_process = 0,
	};
	enum kill {
		bad_kill = -1,
	};
	enum open {
		open_fail = -1,
	};
	enum write {
		write_fail = -1,
	};
	enum dl {
		dlclose_success = 0,
#define ReturnValue_dlerror_no_error nullptr;
    };
	enum error {
	   error_executing_program = 1,
	   error_forking_process,
	   error_opening_file,
	   error_not_enough_args,
	   error_writing_to_file,
	   error_dlclose_fail,
	   error_dlsym_fail,
	   error_dlopen_fail,
	   error_unavailable_node,
    };

};
namespace MmapValue {
	enum flags { //exactly
		anon_fd = -1,
		anon_offset = 0,
		no_offset = 0,
	};
};
namespace ExecValue {
	enum args {
		here_not_zero = 1,
		here_zero = 2,
		not_zero = 1,
    };
#define ExecValue_last_arg nullptr
	enum pointer {
		program_name = 0,
		pointer_arg = 1,
		mask_pointer_arg = 2,
	};
};
// enum {
	// 	read_end=0,
	// 	write_end=1,
	// };
	// enum {
	// 	stdin = 0,
	// 	stdout = 1,
	// 	stderr = 2,
	// };
	// error_creating_pipe,
		// error_child_args,
		// error_creating_process,
		// error_reading_input,
		// error_writing_pipe,
		// error_reading_pipe,
		// error_writing_err,
	// };
