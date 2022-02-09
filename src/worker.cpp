#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
import mlink;
import explicit_enum;
import timer;
import exec_arg;

int main( int argc, char** argv) {

	int my_id; // my left right
	pid_t pid = getpid();
	Timer timer:
	MLink link[ 3 ]; //up, left, rigth

	MLink::get_id( my_id, argv );
	link[ MLink::up ].create( pid, my_id );
	bool all_good = true;
	std::string message;
	while( all_good ) {
		if( link[ MLink::left ].exist() )
			link[ MLink::up ] << link[ MLink::left ];
		if( link[ MLink::rigth ].exist() )
			link[ MLink::up ] << link[ MLink::right ];

		link[ MLink::up ].get_message( message );
		MLink::Command command = MLink::get_command( message );
		if( command == MLink::error_c ) {
			std::cerr << my_id << " can't send message\n";
			pause();
		} //shit commands aren't allowed
		int another_id = MLink::get_id( message );
		if( another_id > id[ ID::my ] ) {
			manage_command( link, MLink::right, message, command, another_id, my_id );
		} else if ( another_id < id[ ID::my ] ) {
			manage_command( link, MLink::left, message, command, another_id, my_id );
		} else {
			switch( command ) {
				case MLink::attach_c:
					MLink::kill_process( message );
					link[ MLink::up ].send_message( MLink::error_id_already_exist( id_my ) );
					break;
				case MLink::exec_c:
					MLink::ExecCommand ecommand = MLink::get_exec_command( message );
					std::string reply;
					if( ecommand == MLink::time ) {
						reply = MLink::exec_reply_message( timer.time(), id[ ID::my ], ecommand );
					} else {
						reply = MLink::exec_reply_message( id[ ID::my ], ecommand );
						if( ecommand == MLink::stop ) {
							timer.stop();
						} else if( ecommand == Mlink::start ) {
							timer.start();
						}
					}
					link[ MLink::up ].send_message( reply );
					break;
				case MLink::ping_c:
					std::string reply = MLink::ping_reply_message( id[ ID::my ] );
					link[ MLink::up ].send_message( reply );
					break;
				case Mlink::close_c:
					all_good = false;
					if( link[ MLink::left ].exist() ) link[ MLink::left ].command_close();
					if( link[ MLink::right ].exist() ) link[ MLink::right ].command_close();
					break;
			}
		}
	}
	return ReturnValue::nice;
}

void manage_command( MLink* link, int direction, const std::string& message, MLink::Command command, int another_id, int my_id ) {
	if( link[ direction ].exist() ) {
		link[ direction ].send_message( message ); // check when sending is better overall
	} else if( command == MLink::attach ) {
		link[ direction ].attach_worker( message, my_id ); //errors?
		std::string reply = MLink::attach_reply_message( message, my_id );
		link[ MLink::up ].send_message( reply );
	} else {
		std::string error_reply = MLink::error_id_doesnt_exist( another_id, my_id );
		link[ MLink::up ].send_error( error_reply ); //errors?
	}
}
