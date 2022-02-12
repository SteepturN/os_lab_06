#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include "explicit_enum.hpp"
#include "mlink.hpp"

bool wrong_input() {
	if( std::cin.fail() ) {
		// if( std::cin.eof() ) {
		// 	exit( ReturnValue::nice );
		std::cerr << "wrong input\n";
		std::cin.clear();
		return true;
	}
	return false;
}
int main() {
	bool all_good = true;
	MLink link;
	//fork-exec another working process with name of queue as execve-argument
	int my_id = -1;
	int worker_id = 0;
	pid_t pid = MLink::create_worker( worker_id ); //what will happend if I fork or exec in another thread?

	link.attach_worker( my_id, worker_id, pid );

	std::cout << "You can use:\n"
	          << "\t->\t create [id]\n"
	          << "\t->\t exec [id] [command]\n"
	          << "\t->\t ping [id]\n";
	while( all_good ) {
		std::cout << link;

		std::cout << "\t> ";
		std::string command;
		int id;
		std::cin >> command;
		if ( command == "exit" ){
			break;
		} else if( command == "w" ) {
			command.clear();
			continue;
		}
		if( wrong_input() ) {
			command.clear();
			continue;
		}
		std::cin >> id;
		if( wrong_input() ) {
			command.clear();
			continue;
		}
		if( command == "create" ) {
			pid_t pid = MLink::create_worker( id );
			link.command_attach_worker( id, pid );
		} else if ( command == "exec" ) {
			std::string time_param;
			std::cin >> time_param;
			if( wrong_input() ) {
				command.clear();
				continue;
			}
			link.command_exec( id, time_param );
		} else if ( command == "ping" ) {
			link.command_ping( id ); //even first could be broken tho'
		} else {
			std::cout << "wrong command" << std::endl;
		}
		command.clear();
	}
	//while-loop: create [id]; exec [id] [start|stop|time]; ping [id]; exit

	//create -> create process and message working node with: create id pid ----------------new node and balance
	//exec -> message working node with: exec id  -----------------taking into account id
	//ping ->
	return 0;
}
