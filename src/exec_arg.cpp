#include <memory>
#include <type_traits>
#include <cstdint>
#include <string>
#include <iostream>
#include "explicit_enum.hpp"
#include "exec_arg.hpp"

namespace ExecArg {
	template < typename T >
	void make_execv_arg( const T pointer, std::string& pathname,
							char**& exec_arg ) {
		std::allocator< char > alloc_ch;
		const int exec_arg_count = 4; //name, pointer, mask, null
		char* pointer_arg = alloc_ch.allocate( 2 * ( sizeof( T ) + sizeof( '\0' ) ) );
		const int mask_offset = sizeof( T ) + sizeof( '\0' );
		for( decltype( sizeof( T ) ) i = StandardValue::array_begin;
		     i < sizeof( T ); ++i) {
			pointer_arg[ i ] = *( reinterpret_cast< const unsigned char* >( &pointer ) + i );
			pointer_arg[ i + mask_offset ] = ExecValue::here_not_zero;
			if( pointer_arg[ i ] == '\0' ) {
				pointer_arg[ i ] = ExecValue::not_zero;
				pointer_arg[ i + mask_offset ] = ExecValue::here_zero;
			}

		}
		pointer_arg[ sizeof( T ) ] = '\0';
		pointer_arg[ mask_offset + sizeof( T ) ] = '\0';

		std::allocator< char* > alloc_chptr;
		exec_arg = alloc_chptr.allocate( exec_arg_count );


		exec_arg[ ExecValue::program_name ] = pathname.data();
		exec_arg[ ExecValue::pointer_arg ] = pointer_arg;
		exec_arg[ ExecValue::mask_pointer_arg ] = pointer_arg + mask_offset;
		exec_arg[ exec_arg_count - 1 ] = ExecValue_last_arg; //last should be null
	}

	template < typename T >
	void get_execv_arg( T& pointer, const char** exec_arg ) {

		char pointer_raw[ sizeof( T ) ];
		for( decltype( sizeof( T ) ) i = 0; i < sizeof( T ); ++i ) {
			pointer_raw[ i ] = exec_arg[ ExecValue::pointer_arg ][ i ];
			if( exec_arg[ ExecValue::mask_pointer_arg ][ i ] == ExecValue::here_zero ) {
				pointer_raw[ i ] = 0;
			}
		}
		pointer = *reinterpret_cast< T* >( pointer_raw );
	}
};
template void ExecArg::make_execv_arg< int >( const int, std::string&, char**& );
template void ExecArg::get_execv_arg< int >( int&, const char** );
