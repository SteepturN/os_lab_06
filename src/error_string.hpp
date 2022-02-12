#pragma once
#include <string>
namespace Error {
	std::string forking_process( const int pid );
	std::string executing_program( const std::string& pathname );
	std::string mmap_failed( const int mapping_length );
	std::string kill_failed( pid_t pid, int signal );
	std::string opening_file( const std::string& file );
	std::string writing_to_file( const int fd, const std::string& file );
	std::string dlclose_fail();
};
