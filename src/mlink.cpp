#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <time.h>
#include "mlink.hpp"
#include "exec_arg.hpp"
#include "explicit_enum.hpp"
#include "error_string.hpp"
/*
** instant_ping == "p::"
** instant_ping_reply = "p::1"
** out id = "[Command::id_c] [id]" // I have to do that because worker can't know who will be at the top channel[ ch::up ].id[ ID::out ]
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

MLink::MLink()
	: connection_exists( false ), routing_key( "" ) {}

void MLink::create_queue( MLink::ch dir, const std::string& suffix ) {
	try {
		int direction = static_cast< int >( dir );
		queue_name[ direction ] = "q:";
		queue_name[ direction ] += suffix;
		exchange_name[ direction ] = "e:";
		exchange_name[ direction ] += suffix;
		Channel::OpenOpts openopts; //= Channel::OpenOpts::FromUri( "amqp://guest:guest@localhost:5672/" );
		openopts.host = "localhost";
		openopts.vhost = "myvhost";
		openopts.port = 5672;
		openopts.auth = Channel::OpenOpts::BasicAuth( "newuser", "newuser" );
		channel[ direction ] = Channel::Open( openopts );
		queue_broker_name[ direction ] = channel[ direction ]->
			DeclareQueue( queue_name[ direction ], false, false, false, false );
		channel[ direction ]->DeclareExchange( exchange_name[ direction ] );
		channel[ direction ]->BindQueue( queue_name[ direction ], exchange_name[ direction ], routing_key );
	} catch( AmqpClient::AmqpException& e ) {
		std::cerr << "yesss" << e.reply_text() << '\n' << std::endl;
	}
}

void MLink::create( pid_t pid, int my_id, bool get_id, bool swap ) {
	std::stringstream suffix;
	suffix << "in:" << static_cast< long >( pid );
	MLink::ch in = ch::in;
	MLink::ch out = ch::out;
	MLink::ch ping_in = ch::ping_in;
	MLink::ch ping_out = ch::ping_out;
	if( swap ) {
		in = ch::out;
		out = ch::in;
		ping_in = ch::ping_out;
		ping_out = ch::ping_in;
	}
	create_queue( in, suffix.str() );

	// channel[ in ]->CheckExchangeExists( exch_n );


	if( !swap ) {
		consumer_tag = channel[ in ]->BasicConsume( queue_name[ in ], "", true, true, false );
	}

	suffix.str( "" );
	suffix << "out:" << static_cast< long >( pid );
	create_queue( out, suffix.str() );

	if( swap ) {
		consumer_tag = channel[ out ]->BasicConsume( queue_name[ out ], "", true, true, false );
	}
	suffix.str( "" );
	suffix << "ping_in:" << static_cast< long >( pid );
	create_queue( ping_in, suffix.str() );

	suffix.str( "" );
	suffix << "ping_out:" << static_cast< long >( pid );
	create_queue( ping_out, suffix.str() );

	connection_exists = true;
	int _my_id = static_cast< int >( ID::my );
	int out_id = static_cast< int >( ID::out );
	id[ _my_id ] = my_id;
	delete_queue = false;
	if( get_id ) {
		id[ out_id ] = get_out_id();
		delete_queue = true;
	}
}
int MLink::get_out_id() {
	std::string message;
	get_message( message );
	return get_out_id( message );
}
int MLink::get_out_id( const std::string& message ) {
	std::stringstream mssg( message );
	int answ;
	int command;
	mssg >> command >> answ;
	return answ;
}
void MLink::get_id( int& id, const char** argv ) {
	ExecArg::get_execv_arg< int >( id, argv );
}
MLink& MLink::operator<<( MLink& rhs ) { //??can he
	AmqpClient::Envelope::ptr_t message;
	const int timeout = 10;
	while( rhs.channel[ ch::in ]->BasicConsumeMessage( rhs.consumer_tag, message, timeout ) ) {
		send_message( message->Message()->Body() );
	}
	return *this;
}
bool MLink::exist() const {
	return connection_exists;
}

void MLink::get_message( std::string& message ) {
	ping_reply();
	message = channel[ ch::in ]->BasicConsumeMessage( consumer_tag )->Message()->Body();
}

void MLink::ping_reply() {
	AmqpClient::Envelope::ptr_t message;
	while( channel[ ch::ping_in ]->BasicGet( message, queue_name[ ch::ping_in ] ) ) {
		channel[ ch::ping_out ]->BasicPublish( exchange_name[ ch::ping_out ],
		                                       routing_key, AmqpClient::BasicMessage::Create( MLink::instant_ping_reply() ) );
	}
}
// bool MLink::instant_ping( const std::string& message ) {
// 	return message == "p::";
// }
std::string MLink::instant_ping_reply() {
	return std::move( std::string( "p::1" ) );
}

MLink::Command MLink::get_command( const std::string& message ) {
	std::stringstream mssg;
	mssg.str( message );
	int command;
	mssg >> command;
	return static_cast< Command >( command );
}
int MLink::get_id( const std::string& message ) {
	std::stringstream mssg( message );
	int command;
	int id;
	mssg >> command;
	mssg >> id;
	return id;
}
void MLink::send_message( const std::string& message, bool check_connection ) { //if don't have id
	if( !check_connection ) {
		std::cout << "id " << id[ static_cast< int >( ID::my ) ]
		          << ": sending message to id " << id[ static_cast< int >( ID::out ) ] << ":\"" << message << "\"" << std::endl;
		channel[ ch::out ]->BasicPublish( exchange_name[ ch::out ], routing_key, AmqpClient::BasicMessage::Create( message ) );
	} else if( ping() ) {
		channel[ ch::out ]->BasicPublish( exchange_name[ ch::out ], routing_key, AmqpClient::BasicMessage::Create( message ) );
	} else {
		int my = static_cast< int >( ID::my );
		int out = static_cast< int >( ID::out );
		std::string error_message = MLink::error_id_unreachable( id[ out ], id[ my ]);
		//throw message somewhere
		channel[ ch::in ]->BasicPublish( exchange_name[ ch::in ], routing_key, AmqpClient::BasicMessage::Create( error_message ) );
	}
}
bool MLink::ping() {
	ping_reply(); //
	channel[ ch::ping_out ]->BasicPublish( exchange_name[ ch::ping_out ], routing_key, AmqpClient::BasicMessage::Create( MLink::instant_ping_message() ) );
	AmqpClient::Envelope::ptr_t message; //maybe wait?
	sleep( 5 );
	return channel[ ch::ping_in ]->BasicGet( message, queue_name[ ch::ping_in ] );
	//i donno if this would work
	//1) if time between sending and receiving would be too big
	//2) if I would already have some ping mesages at queue
}
std::string MLink::instant_ping_message() {
	return "p::";
}
// bool MLink::instant_ping_reply( const std::string& message ) {
// 	return message == "p::1";
// }

std::string MLink::error_id_unreachable( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' ' <<
		id_my << ' ' << static_cast< int >( MLError::id_unreachable ) << ' ' << id_out;
	return mssg.str();
}
void MLink::attach_worker( const std::string& command, int _id ) {
	std::stringstream mssg( command );
	int useless;
	int out = static_cast< int >( ID::out );
	mssg >> useless; //command
	mssg >> id[ out ];

	int pid; // size pid_t > size int ?
	mssg >> pid;
	create( pid, _id, false, true );
	std::string mssg_id = MLink::id_out_message( id[ static_cast< int >( ID::my ) ] );
	send_message( mssg_id );
}

void MLink::attach_worker( int my_id, int out_id, pid_t pid ) {
	id[ static_cast< int >( ID::out ) ] = out_id;
	create( pid, my_id, false, true );
	std::string mssg_id = MLink::id_out_message( id[ static_cast< int >( ID::my ) ] );
	send_message( mssg_id );
}
std::string MLink::id_out_message( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::id_c ) << ' ' << id;
	return mssg.str();
}


std::string MLink::attach_reply_message( const std::string& message, int my_id ) {
	std::stringstream mssg( message );
	std::stringstream replymssg;
	int another_id;
	int pid;
	int command;
	mssg >> command >> another_id >> pid;
	replymssg << static_cast< int >( Command::attach_c )
	          << ' ' << my_id << ' ' << another_id << ' ' << pid << ' ' << static_cast< int >( Reply::ok );
	return replymssg.str();
}

std::string MLink::error_id_doesnt_exist( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' '
	     << id_my << ' ' << static_cast< int >( MLError::id_doesnt_exist ) << ' ' << id_out;
	return mssg.str();
}

std::string MLink::error_id_already_exist( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' ' << id << static_cast< int >( MLError::id_already_exist );
	return mssg.str();
}

MLink::ExecCommand MLink::get_exec_command( const std::string& message ) {
	std::stringstream mssg( message );
	int useless;
	int command;
	mssg >> useless; //command
	mssg >> useless; //id
	mssg >> command;
	return static_cast< ExecCommand >( command );
}

std::string MLink::exec_reply_message( int time, int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::exec_c ) << ' ' << id << ' ' << static_cast< int >( command ) << ' ' << time << ' ' << static_cast< int >( Reply::ok );
	return mssg.str();
}

std::string MLink::exec_reply_message( int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::exec_c ) << ' ' << id << ' ' << static_cast< int >( command ) << ' ' << static_cast< int >( Reply::ok );
	return mssg.str();
}

std::string MLink::ping_reply_message( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::ping_c ) << ' ' << id << ' ' << static_cast< int >( Reply::ok );
	return mssg.str();
}

void MLink::command_close() {
	std::stringstream mssg;
	int out = static_cast< int >( ID::out );
	mssg << static_cast< int >( Command::close_c ) << ' ' << id[ out ];
	send_message( mssg.str() );
}







pid_t MLink::create_worker( int _id ) {
	pid_t fork_res = fork();
	if( fork_res == ReturnValue::fork_fail ) {
		std::cerr << Error::forking_process( getpid() );
		//exit( ReturnValue::error_fork_fail );
	} else if( fork_res == ReturnValue::fork_child_process ) {
		std::string prog_name = "worker";
		char** exec_arg;
		ExecArg::make_execv_arg< int >( _id, prog_name, exec_arg );
		if( execv( prog_name.c_str(), exec_arg ) == ReturnValue::exec_fail ) {
			std::cerr << Error::executing_program( prog_name );
		}
	} else {
		return fork_res;
	}
	return -1;
}

void MLink::command_attach_worker( int id, pid_t pid ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::attach_c ) << ' ' << id << ' ' << pid;
	send_message( mssg.str() );
}
void MLink::command_exec( int id, const std::string& command ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::exec_c ) << ' ' << id << ' ';
	if( command == "time" ) {
		mssg << static_cast< int >( ExecCommand::time );
	} else if( command == "stop" ) {
		mssg << static_cast< int >( ExecCommand::stop );
	} else if( command == "start" ) {
		mssg << static_cast< int >( ExecCommand::start );
	} else {
		std::cerr << "error command_exec";
		return;
	}
	send_message( mssg.str() );
}
void MLink::command_ping( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::ping_c ) << ' ' << id;
	send_message( mssg.str() );
}

MLink::~MLink() {
	if( connection_exists ) {
		if( !delete_queue ) {
			command_close();
		} else {
			std::cout << "id " << id[ static_cast< int >( ID::my ) ] << "->"
						<< id[ static_cast< int >( ID::out ) ] << " start cancelling...";
			channel[ ch::in ]->BasicCancel( consumer_tag );
			std::cout << "success; " << std::endl;
			for( int i = 0; i < 4; ++i ) {
				std::cout << "id " << id[ static_cast< int >( ID::my ) ] << "->"
						<< id[ static_cast< int >( ID::out ) ]<< " start deleting exchange...";
				// if( channel[ i ]->CheckExchangeExists( exchange_name[ i ] ) )
				channel[ i ]->DeleteExchange( exchange_name[ i ] );
				std::cout << "success; "<< std::endl;
				std::cout << "id " << id[ static_cast< int >( ID::my ) ] << "->"
						<< id[ static_cast< int >( ID::out ) ] << " start deleting queue...";
				// if( channel[ i ]->CheckQueueExists( queue_name[ i ] ) )
				channel[ i ]->DeleteQueue( queue_name[ i ] );
				std::cout << "success; " << std::endl;
			}
		}
	}
}










std::ostream& operator<<( std::ostream& cout, MLink& rhs ) {
	using Command = MLink::Command;
	using ExecCommand = MLink::ExecCommand;
	using MLError = MLink::MLError;
	using ch = MLink::ch;
	rhs.ping_reply();
	AmqpClient::Envelope::ptr_t message_env;
	std::string message;
	const int timeout = 10;
	while( rhs.channel[ ch::in ]->BasicConsumeMessage( rhs.consumer_tag, message_env, timeout ) ) {
		message = message_env->Message()->Body();
		int command_i;
		int id;
		int other_id;
		ExecCommand exec_command;
		std::stringstream mssg( message );
		mssg >> command_i >> id;
		Command command = static_cast< Command >( command_i );
		switch( command ) {
			case Command::attach_c:
				int pid;
				mssg >> other_id >> pid;
				cout << "Ok: added worker with id = " << other_id << ", pid = " << pid << " to worker with id = " << id << '\n';
				break;
			case Command::exec_c:
				int exec_command_i;
				mssg >> exec_command_i;
				exec_command = static_cast< ExecCommand >( exec_command_i );
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
				int error_type_i;
				mssg >> error_type_i;
				MLError error_type = static_cast< MLError >( error_type_i );
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
void MLink::kill_process( const std::string& message ) {
	int command, id, pid; //attach
	std::stringstream mssg( message );
	mssg >> command >> id >> pid;
	kill( pid, SIGKILL );
}
