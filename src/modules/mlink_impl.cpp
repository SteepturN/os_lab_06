#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include "mlink.hpp"
import exec_arg;
import explicit_enum;
import error_string;
/*
** instant_ping == "p::"
** instant_ping_reply = "p::1"
** out id = "[id]"
** command:= "[Command] [id] "
** attach == "[Command::attach_c] [id_whom_to_attach] [pid]"
** attach reply == "[Command::attach_c] [id who sends message] [id who was attached] [pid of attached process] [Reply::ok]"
** exec == "[Command::exec_c] [id] []"
** exec reply == "[Command::exec_c] [id] [ExecCommand::... [if time->time_result]] [Reply::ok]"
** ping == "[Command::ping_c] [id]"
** ping reply == "[Command::ping_c] [id] [Reply::ok]"
** errors: [Command::error_c] [id]
**                                 [MLError::id_unreachable] [id that is unreachable]
**                                 [MLError::id_doesnt_exist] [id that is not exist]
**                                 [MLError::id_already_exist]
**
**
 */

MLink::Mlink()
	: connection_exists( false ), routing_key( "" ) {}

void MLink::create_queue( MLink::ch direction, const std::string& suffix ) {
	queue_name[ direction ] = "q:"
	queue_name[ direction ] += suffix;
	exchange_name[ direction ] = "e:";
	exchange_name[ direction ] += suffix;
	Channel::OpenOpts openopts = Channel::OpenOpts::FromUri( "amqp://localhost" );
	channel[ direction ] = Channel::Open( openopts );
	queue[ direction ] = channel[ direction ]->DeclareQueue( queue_name[ direction ], false, true, false, false );
	channel[ direction ]->DeclareExchange( exchange_name[ direction ] );
	channel[ direction ]->BindQueue( queue_name[ direction ], exchange_name[ direction ], routing_key );
}

void MLink::create( pid_t pid, int _id, bool get_id ) {
	std::stringstream suffix;
	suffix << ":in:" << static_cast< long >( pid );
	create_queue( ch::in, suffix.str() );
	consumer_tag = channel[ ch::in ]->BasicConsume( queue_name );

	suffix.str( "" );
	suffix << ":out:" << static_cast< long >( pid );
	create_queue( ch::out, suffix.str() );

	suffix.str( "" );
	suffix << ":ping_in:" << static_cast< long >( pid );
	create_queue( ch::ping_in, suffix.str() );

	suffix.str( "" );
	suffix << ":ping_out:" << static_cast< long >( pid );
	create_queue( ch::ping_out, suffix.str() );

	connection_exists = true;
	id[ ID::my ] = _id;
	if( get_id ) id[ ID::out ] = get_out_id();
}
int MLink::get_out_id() {
	std::string message;
	get_message( message );
	return get_out_id( message );
}
int MLink::get_out_id( std::string& message ) {
	std::stringstream mssg( message );
	int answ;
	mssg >> answ;
	return answ;
}
static void MLink::get_id( int& id, char** argv ) {
	ExecArg::get_execv_arg< int >( id, argv );
}
MLink& MLink::operator<<( MLink& rhs ) { ??can he
	Envelope::ptr_t message;
	while( rhs.channel[ ch::in ]->BasicGet( message, rhs.queue[ ch::in ] ) ) {
		send_message( message->Message()->Body() );
	}
}
bool MLink::exist() const {
	return connection_exists;
}

void MLink::get_message( std::string& message ) {
	ping_reply();
	message = channel[ ch::in ]->BasicConsumeMessage( consumer_tag )->Message()->Body();
}

void MLink::ping_reply() {
	Envelope::ptr_t message;
	while( channel[ ch::ping_in ]->BasicGet( message, queue[ ch::ping_in ] ) ) {
		channel[ ch::ping_out ]->BasicPublish( exchange_name[ ch::ping_out ], routing_key, MLink::instant_ping_reply() );
	}
}
// static bool MLink::instant_ping( const std::string& message ) {
// 	return message == "p::";
// }
static std::string MLink::instant_ping_reply() {
	return std::move( std::string( "p::1" ) );
}

static MLink::Command MLink::get_command( std::string& message ) {
	std::stringstream mssg;
	mssg.str( message );
	int command;
	mssg >> command;
	return static_cast< Command >( command );
}
static int MLink::get_id( const std::string& message ) {
	std::stringstream mssg( message );
	int command;
	int id;
	mssg >> command;
	mssg >> id;
	return id;
}
void MLink::send_message( const std::string& message ) { //if don't have id
	if( ping() )
		channel[ ch::out ]->BasicPublish( exchange_name[ ch::out ], routing_key, BasicMessage::Create( message ) );
	else {
		std::string error_message = MLink::error_id_unreachable( id[ ID::out ], id[ ID::my ]);
		//throw message somewhere
		channel[ ch::in ]->BasicPublish( exchange_name[ ch::in ], routing_key, BasicMessage::Create( error_message ) );
	}
}
bool MLink::ping() {
	ping_reply(); //
	channel[ ch::ping_out ]->BasicPublish( exchange_name[ ch::ping_out ], routing_key, BasicMessage::Create( MLink::instant_ping_message() ) );
	Envelope::ptr_t message; //maybe wait?
	return channel[ ch::ping_in ]->BasicGet( message, queue[ ch::ping_in ] );
	//i donno if this would work
	//1) if time between sending and receiving would be too big
	//2) if I would already have some ping mesages at queue
}
static std::string MLink::instant_ping_message() {
	return "p::";
}
// static bool MLink::instant_ping_reply( const std::string& message ) {
// 	return message == "p::1";
// }

static std::string MLink::error_id_unreachable( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << Command::error_c << ' ' << id_my << ' ' << MLError::id_unreachable << ' ' << id_out;
	return mssg.str();
}
void MLink::attach_worker( const std::string& command, int& _id ) {
	std::stringstream mssg( command );
	int useless;
	mssg >> useless; //command
	mssg >> id[ ID::out ];

	int pid; // size pid_t > size int ?
	mssg >> pid;
	create( pid, _id, false );
}

std::string MLink::attach_reply_message( const std::string& message, int my_id ) {
	std::stringstream mssg( message );
	std::stringstream replymssg;
	int another_id;
	int pid;
	int command;
	mssg >> command >> another_id >> pid;
	replymssg << Command::attach_c << ' ' << my_id << ' ' << another_id << ' ' << pid << ' ' << Reply::ok;
	return replymssg.str();
}

static std::string error_id_doesnt_exist( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << Command::error_c << ' ' << id_my << MLError::id_doesnt_exist << ' ' << id_out;
	return mssg.str();
}

static std::string id_already_exist( int id ) {
	std::stringstream mssg;
	mssg << Command::error_c << ' ' << id << MLError::id_already_exist;
	return mssg.str();
}

static MLink::ExecCommand get_exec_command( std::string& message ) {
	std::stringstream mssg( message );
	int useless;
	int command;
	mssg >> useless; //command
	mssg >> useless; //id
	mssg >> command;
	return static_cast< ExecCommand >( command );
}
static std::string MLink::exec_reply_message( int time, int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg << Command::exec_c << ' ' << id << ' ' << command << ' ' << time << ' ' << Reply::ok;
	return mssg.str();
}
static std::string MLink::exec_reply_message( int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg << Command::exec_c << ' ' << id << ' ' << command << ' ' << Reply::ok;
	return mssg.str();
}
static std::string MLink::make_ping_reply_message( int id ) {
	std::stringstream mssg;
	mssg << Command::ping_c << ' ' << id << ' ' << Reply::ok;
	return mssg.str();
}
void MLink::command_close() {
	std::stringstream mssg;
	mssg << Command::close_c << ' ' << id[ ID::out ];
	send_message( mssg.str() );
}







static pid_t MLink::create_worker( int _id ) {
	pid_t fork_res = fork();
	if( fork_res == ReturnValue::fork_fail ) {
		std::cerr << Error::fork_fail;
		//exit( ReturnValue::error_fork_fail );
	} else if( fork_res == ReturnValue::fork_child_process ) {
		char prog_name[] = "worker";
		char** exec_arg;
		ExecArg::make_execv_arg< int >( _id, prog_name, exec_arg );
		if( execvp( prog_name, exec_arg ) == ReturnValue::exec_fail ) {
			std::cerr << Error::executing_program( prog_name );
		}
	} else {
		return fork_res;
	}
	return -1;
}

void MLink::command_attach_worker( int id, pid_t pid ) {
	std::stringstream mssg;
	mssg << Command::attach_c << ' ' << id << ' ' << pid;
	send_message( mssg.str() );
}
void MLink::command_exec( int id, const std::string& command ) {
	std::stringstream mssg;
	mssg << Command::exec_c << ' ' << id << ' ';
	if( command == "time" ) {
		mssg << ExecCommand::time;
	} else if( command == "stop" ) {
		mssg << ExecCommand::stop;
	} else if( command == "start" ) {
		mssg << ExecCommand::start;
	} else {
		std::cerr << "error command_exec";
		return;
	}
	send_message( mssg.str() );
}
void MLink::command_ping( int id ) {
	std::stringstream mssg;
	mssg << Command::ping_c << ' ' << id;
	send_message( mssg.str() );
}

MLink::~MLink() {
	command_close()
}










std::ostream& operator<<( std::ostream& cout, MLink& rhs ) {
	ping_reply();
	Envelope::ptr_t message_env;
	std::string message;
	while( rhs.channel[ ch::in ]->BasicGet( message_env, rhs.queue[ ch::in ] ) ) {
		message = message_env->Message()->Body();
		int command;
		int id;
		std::stringstream mssg( message );
		mssg >> command >> id;
		switch( command ) {
			case Command::attach_c:
				int other_id;
				int pid;
				mssg >> other_id >> pid;
				cout << "Ok: added worker with id = " << other_id << ", pid = " << pid << " to worker with id = " << id << '\n';
				break;
			case Command::exec_c:
				int exec_command;
				mssg >> exec_command;
				if( exec_command == ExecCommand::time ) {
					int time;
					mssg >> time;
					cout << "Ok: " << id << " time = " << time << '\n';
				} else if( exec_command == ExecCommand::stop ){
					cout << "Ok: " << id << " timer stopped\n";
				} else if( exec_command == ExecCommand::start ){
					cout << "Ok: " << id << " timer started\n";
				} else {
					cout << "Error: operator<<\n";
				}
				break;
			case Command::ping_c:
				cout << "Ok: worker with id = " << id << " is available\n";
				break;
			case Command::error_c:
				int error_type;
				int other_id;
				mssg >> error_type;
				if( error_type == MLError::id_unreachable ) {
					mssg >> other_id;
					cout << "Error: " << id << ": id = " << other_id << " is unreachable\n";
				} else if( error_type == MLError::id_doesnt_exist ) {
					mssg >> other_id;
					cout << "Error: " << id << ": id = " << other_id << " doesn't exist\n";
				} else if( error_type == MLError::id_already_exist ) {
					cout << "Error: id = " << id << " already exist";
				}
				break;
		}
	}

	return cout;
}
static void MLink::kill_process( const std::string& message ) {
	int command, id, pid; //attach
	std::stringstream mssg( message );
	mssg >> command >> id >> pid;
	kill( pid, SIGKILL );
}
// message out:
//
// a [pid] [id] -- attach worker with pid=[pid] and id=[id]
// e [time|start|stop] [id] -- exec somthing on id=[id]
// p [id|-1] -- ping for id
// p   -- instant ping
// c [id] -- close process
// err [type] [id]
//
// message in
//
// e [ok|time|e error_code] id
// p [ok|e error_code] id

