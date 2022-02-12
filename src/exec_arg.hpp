#pragma once

namespace ExecArg {
	template < typename T >
	void make_execv_arg( const T, std::string&, char**& );
	template <typename T>
	void get_execv_arg( T& , const char** );
}
