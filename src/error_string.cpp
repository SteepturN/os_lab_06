module;
#include <string>

export module error_string;

export namespace Error {
	std::string forking_process( const int pid ) {
		std::string return_str = "error: forking process: ";
		return return_str + std::to_string( pid );
	}
	std::string executing_program( const std::string& pathname ) {
		std::string return_str = "error: executing program: ";
		return return_str + pathname;
	}
	std::string mmap_failed( const int mapping_length ) {
		std::string return_str = "error: mmap memory with length: ";
		return return_str + std::to_string( mapping_length );
	}
	std::string kill_failed( pid_t pid, int signal ) {
		std::string return_str = "error: kill failed : pid = ";
		return return_str + std::to_string( static_cast< int >( pid ) ) +
			" and signal = " + std::to_string( signal );
	}
	std::string opening_file( const std::string& file ) {
		std::string return_str = "error: open failed, file: ";
		return return_str + file;
	}
	std::string writing_to_file( const int fd, const std::string& file ) {
		std::string return_str = "error: writing to file with fd: ";
		return return_str + std::to_string( fd ) + " and sentence: " + file;
	}
	std::string dlclose_fail() {
		return "error: dlclose fail";
	}
};
