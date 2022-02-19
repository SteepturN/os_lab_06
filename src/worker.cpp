#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "mlink.hpp"
#include "explicit_enum.hpp"
#include "timer.hpp"

void manage_command( MNode&, int, const std::string&, MNode::Command, int, int );
int main( int argc, const char** argv ) {
	int my_id; // my left right
	pid_t pid = getpid();
	Timer timer;
	MNode node; //up, left, rigth

	MNode::get_id( my_id, argv );
	std::cerr << "worker " << pid << ": got id = " << my_id << std::endl;
	node.create_link( Link::up, pid, my_id );
	bool all_good = true;
	std::string message;
	while( all_good ) {
		node.get_message( -1, message, true );

		std::cerr << "worker " << pid << ": got message: " << message << std::endl;

		MNode::Command command = MNode::get_command( message );
		if( command == MNode::Command::instant_ping ) {
			std::cerr  << "worker " << pid << ": and it is an instant ping" << std::endl;
			node.send_instant_ping_reply( message );
			continue;
		} else if( command == MNode::Command::error_c ) {
// I can go straight to the worker, which id would fit for attaching or find out, that sended id already exists
			std::cerr  << "worker " << pid << ": and it is an error" << std::endl;

			std::cerr  << "worker " << pid << my_id << ": can't send message\n";
			return ReturnValue::error_unavailable_node;
		} else if( command == MNode::Command::reply_c ) {
			node.send_message( Link::up, message );
			continue;
		}

		int another_id = MNode::get_id( message );
		if( another_id > my_id ) {

			std::cerr  << "worker " << pid << ": and it's to id = " << another_id << std::endl;

			manage_command( node, Link::right, message, command, another_id, my_id );
		} else if ( another_id < my_id ) {

			std::cerr  << "worker " << pid << ": and it's to id = " << another_id << std::endl;

			manage_command( node, Link::left, message, command, another_id, my_id );
		} else {
			MNode::ExecCommand ecommand;
			std::string reply;
			switch( command ) {
				case MNode::Command::attach_c:

					std::cerr << "and its attach with duplication of my id" << std::endl;

					MNode::kill_process( message );
					node.send_message( Link::up, MNode::make_reply( MNode::error_id_already_exist( my_id ) ) );
					break;
				case MNode::Command::exec_c:

					std::cerr << "and its exec" << std::endl;

					ecommand = MNode::get_exec_command( message );
					if( ecommand == MNode::ExecCommand::time ) {
						reply = MNode::exec_reply_message( timer.time(), my_id, ecommand );
					} else {
						reply = MNode::exec_reply_message( my_id, ecommand );
						if( ecommand == MNode::ExecCommand::stop ) {
							timer.stop();
						} else if( ecommand == MNode::ExecCommand::start ) {
							timer.start();
						}
					}
					node.send_message( Link::up, reply );
					break;
				case MNode::Command::ping_c:

					std::cerr << "and its ping" << std::endl;

					reply = MNode::ping_reply_message( my_id );
					node.send_message( Link::up, reply );
					break;
				case MNode::Command::close_c:

					std::cerr << "and its close" << std::endl;

					all_good = false;
					break;
				default:
					std::cerr << "worker's command handler failed\n";
					break;
			}
		}
	}
	return ReturnValue::nice;
}

void manage_command( MNode& node, int link, const std::string& message, MNode::Command command, int another_id, int my_id ) {
	if( node.exist( link ) ) {

		std::cerr << "and I'm sending it to this id" << std::endl;

		node.send_message( link, message ); // check when sending is better overall
	} else if( command == MNode::Command::attach_c ) {

		std::cerr << "and it's attach, so I will attach it" << std::endl;

		node.attach_worker( link, message ); //errors?
		std::string reply = MNode::attach_reply_message( message, my_id );
		node.send_message( Link::up, reply );
	} else {

		std::cerr  << "worker id=" << my_id << ": and it was an error, no one is attached and connection doesn't exist" << std::endl;

		std::string error_reply = MNode::make_reply( MNode::error_id_doesnt_exist( another_id, my_id ) );
		node.send_message( Link::up, error_reply ); //errors?
	}
}
