module;
#include <memory>
#include <type_traits>
#include <cstdint>
#include <string>

#include <iostream>
export module exec_arg;
import explicit_enum;

export namespace ExecArg {
	template < typename T >
	void make_execv_arg( T pointer, std::string& pathname,
							char**& exec_arg ) {
		std::allocator< char > alloc_ch;
		// std::cout << "making pointer: |" << pointer << "|\n";
		const int exec_arg_count = 4; //name, pointer, mask, null
		char* pointer_arg = alloc_ch.allocate( 2 * ( sizeof( T ) + sizeof( '\0' ) ) );
		const int mask_offset = sizeof( T ) + sizeof( '\0' );
		for( decltype( sizeof( T ) ) i = StandardValue::array_begin;
		     i < sizeof( T ); ++i) {
			// std::cout << i << " byte: " <<  *( reinterpret_cast< unsigned char* >( &pointer ) + i );
			pointer_arg[ i ] = *( reinterpret_cast< unsigned char* >( &pointer ) + i );
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

	    // std::cout << "have pointer intptr_t: " << *reinterpret_cast<std::intptr_t*>(&pointer) << '\n';
	    // std::cout << "throw pointer intptr_t: " << *reinterpret_cast<std::intptr_t*>(pointer_arg) << '\n';
	    // std::cout << "pointer throw: ";
	    // for(int i = 0; i < sizeof(void*); ++i) {
		//     std::cout << static_cast<int>(static_cast<unsigned char>(pointer_arg[i])) << ' ';
	    // }
	    // std::cout <<'\n';
	    // std::cout << "mask throw: ";
	    // for(int i = 0; i < sizeof(void*); ++i) {
		//     std::cout << static_cast<int>(pointer_arg[i + mask_offset]) << ' ';
	    // }
	    // std::cout <<'\n';

		exec_arg[ ExecValue::mask_pointer_arg ] = pointer_arg + mask_offset;
		exec_arg[ exec_arg_count - 1 ] = ExecValue::last_arg; //last should be null
	}

	template <typename T>
	void get_execv_arg( T& pointer, char** exec_arg ) {
	    // std::cout << "catch pointer intptr_t: " << *reinterpret_cast<std::intptr_t*>(exec_arg[ExecValue::pointer_arg]) << '\n';
	    // std::cout << "pointer catch: ";
	    // for(int i = 0; i < sizeof(void*); ++i) {
		//     std::cout << static_cast<int>(static_cast<unsigned char>(exec_arg[ExecValue::pointer_arg][i])) << ' ';
	    // }
	    // std::cout <<'\n';
	    // std::cout << "mask catch: ";
	    // for(int i = 0; i < sizeof(void*); ++i) {
		//     std::cout << static_cast<int>(static_cast<unsigned char>(exec_arg[ExecValue::mask_pointer_arg][i])) << ' ';
	    // }
	    // std::cout <<'\n';

	    //std::intptr_t pointer_raw = 0;

		//1 var
		// for( decltype( sizeof( void* ) ) i = StandardValue::array_begin;
		//      i < sizeof( void* ); ++i ) {
		// 	pointer_raw <<= StandardValue::bits_in_byte;
		// 	if( exec_arg[ ExecValue::mask_pointer_arg ][ i ] != ExecValue::here_zero ) {
		// 		pointer_raw += static_cast< unsigned char >( exec_arg[ ExecValue::pointer_arg ][ i ] );
		// 	}
		// 	std::cout << *reinterpret_cast< T** >( &pointer_raw ) << '\n';
		// }

		// 2 var
	    // pointer_raw = *reinterpret_cast<std::intptr_t*>(exec_arg[ExecValue::pointer_arg]);
	    // for( decltype( sizeof( void* ) ) i = StandardValue::array_begin;
		//      i < sizeof( void* ); ++i ) {
		// 	if( exec_arg[ ExecValue::mask_pointer_arg ][ i ] == ExecValue::here_zero ) {
		// 		*(reinterpret_cast<unsigned char*>( &pointer_raw ) + i ) = 0;
		// 	}
		// }
		// pointer = *reinterpret_cast< T** >( &pointer_raw );
		//

		char pointer_raw[ sizeof( T ) ];
		for( decltype( sizeof( T ) ) i = 0; i < sizeof( T ); ++i ) {
			pointer_raw[ i ] = exec_arg[ ExecValue::pointer_arg ][ i ];
			if( exec_arg[ ExecValue::mask_pointer_arg ][ i ] == ExecValue::here_zero ) {
				pointer_raw[ i ] = 0;
			}
		}
		pointer = *reinterpret_cast< T* >( pointer_raw );
		// std::cout << "have got pointer: |" << pointer << "|\n";
	    // std::cout << "pointer intptr_t: " << pointer_raw << '\n';
	}
};
