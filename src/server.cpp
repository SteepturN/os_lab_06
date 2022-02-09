#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <unistd.h>
#include <iostream>

import mlink;
import

int main() {
	bool all_good = true;
	MLink link;
	//fork-exec another working process with name of queue as execve-argument
	int id;
	pid_t pid = MLink::create_worker( id ); //what will happend if I fork or exec in another thread?

	link.attach_worker( id(), pid );

	while( all_good ) {
		std::cout << link;

		std::cout << '>';
		std::string command;
		std::cin >> command;
		if( command == "create" ) {
			int id;
			std::cin >> id;
			pid_t pid = MLink::create_worker( id );
			link.command_attach_worker( id, pid );
		} else if ( command == "exec" ) {
			int id;
			std::string time_param;
			std::cin >> id >> time_param;
			link.command_exec( id, time_param );
		} else if ( command == "ping" ) {
			int id;
			std::cin >> id;
			link.command_ping( id ); //even first could be broken tho'
		} else {
			all_good = false;
		}
	}
	//while-loop: create [id]; exec [id] [start|stop|time]; ping [id]; exit

	//create -> create process and message working node with: create id pid ----------------new node and balance
	//exec -> message working node with: exec id  -----------------taking into account id
	//ping ->
	return 0;
}
