#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <functional>
#include <thread>
#include "explicit_enum.hpp"
#include "mlink.hpp"

void manage_reply( const std::string& reply, MNode& node, int link );
bool wrong_input( std::basic_istream< char >& cin = std::cin ) {
	if( cin.fail() ) {
		// if( std::cin.eof() ) {
		// 	exit( ReturnValue::nice );
		std::cerr << "wrong input\n";
		cin.clear();
		return true;
	}
	return false;
}
int main() {
	bool all_good = true;
	MNode node;
	int my_id = -1;
	node.set_my_id( my_id );
	//fork-exec another working process with name of queue as execve-argument
	int worker_id = 0;
	pid_t pid = MNode::create_worker( worker_id ); //what will happend if I fork or exec in another thread?
	int link = Link::right;
	node.attach_worker( link, worker_id, pid );
	std::string message;
	// std::function< void() > ping_reply = [ &stop_thread, &node, link ](){
	// 	while( !stop_thread ) {
	// 		std::string message;
	// 		bool wait;
	// 		node.get_message( link, message, wait = true, consumer::ping_in );
	// 		if( MNode::instant_ping( message ) ) {
	// 			bool check_connection;
	// 			node.send_message( link, MNode::instant_ping_reply(), check_connection = false, routing_key::ping_out );
	// 		} else {
	// 			std::cerr << "something went wrong with ping_reply\n";
	// 		}
	// 	}
	// };
	bool stop_thread = false;
	bool got_message = false;
	std::function< void() > get_message = [ &message, &got_message, &stop_thread ](){
		while( true ) {
			while( got_message && !stop_thread )
				sleep( 1 );
			if( stop_thread ) return;
			std::getline( std::cin, message, '\n' );
			got_message = true;
		}
	};
	std::thread message_handling( get_message );
	std::cerr << "You can use:\n"
	          << "\t->\tcreate [id]\n"
	          << "\t->\texec [id] [command]\n"
	          << "\t->\tping [id]\n";
	std::string command;
	int id;
	while( all_good ) {
		command.clear();
		id = -1;
		bool wait;
		std::string reply;
		bool got_reply = true;
		while( !got_message || got_reply ) {
			got_reply = node.get_message( link, reply, wait = false );
			if( got_reply ) {
				manage_reply( reply, node, link );
				reply.clear();
			}
		}
		//I should remove ping when sending
												   //from the first worker or make ping replier in another thread
		// std::cerr << NLink::parse_message( message );
		std::stringstream mssg( message );
		if( wrong_input() ) {
			got_message = false;
			continue;
		}
		mssg >> command;
		if( command == "exit" ) {
			stop_thread = true;
			message_handling.join();
			break;
		} else if( command == "w" ) { //wait for messages
			continue;
		}
		mssg >> id;
		if( wrong_input( mssg ) ) {
			got_message = false;
			continue;
		}

		if( command == "create" ) {
			pid_t pid = MNode::create_worker( id );
			node.command_attach_worker( link, id, pid );
		} else if ( command == "exec" ) {
			std::string time_param;
			mssg >> time_param;
			if( wrong_input( mssg ) ) {
				continue;
			}
			node.command_exec( link, id, time_param );
		} else if ( command == "ping" ) {
			node.command_ping( link, id ); //even first could be broken tho'
		} else {
			std::cerr << "wrong command" << std::endl;
		}
		got_message = false;
		std::cerr << "\t> ";
	}
	//while-loop: create [id]; exec [id] [start|stop|time]; ping [id]; exit

	//create -> create process and message working node with: create id pid ----------------new node and balance
	//exec -> message working node with: exec id  -----------------taking into account id
	//ping ->
	return 0;
}

void manage_reply( const std::string& reply, MNode& node, int link ) {
	MNode::Command command = MNode::get_command( reply );
	if( command == MNode::Command::instant_ping ) {
		node.send_instant_ping_reply( reply );
	} else if( command == MNode::Command::error_c ) {
		std::cerr << "initial worker is anavailable";
		return;
	} else if( command == MNode::Command::reply_c ) {
		std::cerr << MNode::parse_reply( reply );
	} else {
		std::cerr << "received wrong message:" << reply << '\n';
	}
}
