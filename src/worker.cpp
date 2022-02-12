#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "mlink.hpp"
#include "explicit_enum.hpp"
#include "timer.hpp"

void manage_command( MLink* link, int direction, const std::string& message, MLink::Command command, int another_id, int my_id );
int main( int argc, const char** argv ) {
	using Link = MLink::Link;
	int my_id; // my left right
	pid_t pid = getpid();
	Timer timer;
	MLink link[ 3 ]; //up, left, rigth

	MLink::get_id( my_id, argv );
	std::cout << "worker " << pid << ": got id = " << my_id << std::endl;
	link[ Link::up ].create( pid, my_id, true, false );
	bool all_good = true;
	std::string message;
	while( all_good ) {
		if( link[ Link::left ].exist() )
			link[ Link::up ] << link[ MLink::left ];
		if( link[ Link::right ].exist() )
			link[ Link::up ] << link[ MLink::right ];

		link[ Link::up ].get_message( message );

		std::cout << "worker " << pid << ": got message: " << message << std::endl;

		MLink::Command command = MLink::get_command( message );
		if( command == MLink::Command::error_c ) {

			std::cout << "and its an error" << std::endl;

			std::cerr << my_id << " can't send message\n";
			exit( ReturnValue::error_unavailable_node );
		} //shit commands aren't allowed
		int another_id = MLink::get_id( message );
		if( another_id > my_id ) {

			std::cout << "and its to id = " << another_id << std::endl;

			manage_command( link, Link::right, message, command, another_id, my_id );
		} else if ( another_id < my_id ) {

			std::cout << "and its to id = " << another_id << std::endl;

			manage_command( link, Link::left, message, command, another_id, my_id );
		} else {
			MLink::ExecCommand ecommand;
			std::string reply;
			switch( command ) {
				case MLink::Command::attach_c:

					std::cout << "and its attach with duplication of my id" << std::endl;

					MLink::kill_process( message );
					link[ Link::up ].send_message( MLink::error_id_already_exist( my_id ) );
					break;
				case MLink::Command::exec_c:

					std::cout << "and its exec" << std::endl;

					ecommand = MLink::get_exec_command( message );
					if( ecommand == MLink::ExecCommand::time ) {
						reply = MLink::exec_reply_message( timer.time(), my_id, ecommand );
					} else {
						reply = MLink::exec_reply_message( my_id, ecommand );
						if( ecommand == MLink::ExecCommand::stop ) {
							timer.stop();
						} else if( ecommand == MLink::ExecCommand::start ) {
							timer.start();
						}
					}
					link[ MLink::up ].send_message( reply );
					break;
				case MLink::Command::ping_c:

					std::cout << "and its ping" << std::endl;

					reply = MLink::ping_reply_message( my_id );
					link[ Link::up ].send_message( reply );
					break;
				case MLink::Command::close_c:

					std::cout << "and its close" << std::endl;

					all_good = false;
					break;
			}
		}
	}
	return ReturnValue::nice;
}

void manage_command( MLink* link, int direction, const std::string& message, MLink::Command command, int another_id, int my_id ) {
	if( link[ direction ].exist() ) {

		std::cout << "and I'm sending it to this id" << std::endl;

		link[ direction ].send_message( message ); // check when sending is better overall
	} else if( command == MLink::Command::attach_c ) {

		std::cout << "and it's attach, so I will attach it" << std::endl;

		link[ direction ].attach_worker( message, my_id ); //errors?
		std::string reply = MLink::attach_reply_message( message, my_id );
		link[ MLink::Link::up ].send_message( reply );
	} else {

		std::cout << "and it was an error, no one is attached and connection doesn't exist" << std::endl;

		std::string error_reply = MLink::error_id_doesnt_exist( another_id, my_id );
		link[ MLink::Link::up ].send_message( error_reply ); //errors?
	}
}
