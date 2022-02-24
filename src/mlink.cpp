#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include "mlink.hpp"
#include "exec_arg.hpp"
#include "explicit_enum.hpp"
#include "error_string.hpp"
/*
** instant_ping == "[Command::instant_ping] [id: who sends it]"
** instant_ping_reply = "[Command::instant_ping_reply] [id: who should get it]"
** out id = "[Command::id_c] [id]" // I have to do that because worker can't know who will be at the top channel[ queue::up ].id[ ID::out ]
** command:= "[Command] [id] "
** attach == "[Command::attach_c] [id_whom_to_attach] [pid]"
** exec == "[Command::exec_c] [id] []"
** ping == "[Command::ping_c] [id]"
** reply == "[Command::reply_c] [id to whom send it] [reply]"
	** attach reply == "[Command::attach_c] [id who sends message] [id who was attached] [pid of attached process] [Reply::ok]"
	** ping reply == "[Command::ping_c] [id] [Reply::ok]"
	** exec reply == "[Command::exec_c] [id] [ExecCommand::... [if time->time_result]] [Reply::ok]"
	** errors: [Command::error_c] [id]
	**                                 [MLError::id_unreachable] [id that is unreachable]
	**                                 [MLError::id_doesnt_exist] [id that is not exist]
	**                                 [MLError::id_already_exist]
	**
*/

MNode::MNode()
	: links_per_node( 3 ), queues_per_link( 4 ),
	  consumers_per_link( queues_per_link / 2 ), id( links_per_node + 1 /* my */ ),
	  queue_name( links_per_node, std::vector< std::string >( queues_per_link ) ),
	  consumer_tag( links_per_node, std::vector< std::string >( consumers_per_link ) ),
	  routing_key( links_per_node, std::vector< std::string >( queues_per_link - 1 ) ),
	  link_exists( links_per_node, false ), delete_link( links_per_node, false )  {}

void MNode::create_queue( int link, MNode::queue_e _queue, const std::string& suffix ) {
	int queue = static_cast< int >( _queue );
	queue_name[ link ][ queue ] = "q:";
	queue_name[ link ][ queue ] += suffix;
	/*queue_broker_name = */ channel->
		DeclareQueue( queue_name[ link ][ queue ], false, false, false, false );
}
void MNode::create_consumer( int link, MNode::queue_e _queue, const std::string& suffix ) {
	int queue = static_cast< int >( _queue );
	if( _queue == queue_e::in ) {
		std::cerr << "id " << id[ ID::my ] << ": creating consumer: <" << queue_name[ link ][ queue ] << '>'
		          << " in consumer_tag[" << link << "][" << static_cast< int >( consumer_e::in ) << "]\n";
		consumer_tag[ link ][ static_cast< int >( consumer_e::in ) ] =
			channel->BasicConsume( queue_name[ link ][ queue ],
			                       queue_name[ link ][ queue ], true, false, true, 100 );

	} else if( _queue == queue_e::ping_in ) {
		std::cerr << "id " << id[ ID::my ] << ": creating consumer: <" << queue_name[ link ][ queue ] << '>'
		          << " in consumer_tag[" << link << "][" << static_cast< int >( consumer_e::ping_in ) << "]\n";
		consumer_tag[ link ][ static_cast< int >( consumer_e::ping_in ) ] =
			channel->BasicConsume( queue_name[ link ][ queue ],
			                       queue_name[ link ][ queue ], true, false, true, 100 );
	} else {
		std::cerr << "create_consumer has failed\n";
	}
}
void MNode::create_publisher( int link, MNode::queue_e queue, const std::string& suffix ) {
	MNode::routing_key_e rk;
	routing_key[ link ][ static_cast< int >( rk = routing_key_e::err ) ] = suffix;
	if( queue == queue_e::in ) {
	} else if( queue == queue_e::out ) {
		routing_key[ link ][ static_cast< int >( rk = routing_key_e::out ) ] = suffix;
	} else if( queue == queue_e::ping_out ) {
		routing_key[ link ][ static_cast< int >( rk = routing_key_e::ping_out ) ] = suffix;
	} else {
		std::cerr << "create_publisher fail\n";
	}
	channel->BindQueue( queue_name[ link ][ static_cast< int >( queue ) ],
	                    exchange_name, routing_key[ link ][ static_cast< int >( rk ) ] );
}

void MNode::create( bool swap, const std::string& one, const std::string& two,
                    pid_t pid, int link, MNode::queue_e q, bool publish, bool consume ) {
	std::stringstream suffix;
	if( !swap )
		suffix << one;
	else
		suffix << two;
	suffix << pid;
	// channel[ in ]->CheckExchangeExists( exch_n );
	create_queue( link, q, suffix.str() );
	if( publish ) create_publisher( link, q, suffix.str() );
	if( consume ) create_consumer( link, q, suffix.str() );
}




void MNode::create( int link, pid_t pid, bool swap ) {
	Channel::OpenOpts openopts; //= Channel::OpenOpts::FromUri( "amqp://guest:guest@localhost:5672/" );
	openopts.host = "localhost";
	openopts.vhost = "myvhost";
	openopts.port = 5672;
	openopts.auth = Channel::OpenOpts::BasicAuth( "newuser", "newuser" );
	channel = Channel::Open( openopts );

	std::stringstream suffix;
	suffix << "e:" << pid;
	exchange_name = suffix.str();
	channel->DeclareExchange( exchange_name );

	create( swap, "in:", "out:", pid, link, queue_e::in, true, true );
	create( swap, "out:", "in:", pid, link, queue_e::out, true, false );
	create( swap, "ping_in:", "ping_out:", pid, link, queue_e::ping_in, false, true );
	create( swap, "ping_out:", "ping_in:", pid, link, queue_e::ping_out, true, false );

	link_exists[ link ] = true;
}
void MNode::create_link( int link, pid_t pid, int my_id ) {
	id[ ID::my ] = my_id;
	create( link, pid, false );
	id[ link ] = get_out_id( link );
	delete_link[ link ] = true;
}
int MNode::get_out_id( int link ) {
	std::string message;
	bool wait;
	get_message( link, message, wait = true, consumer_e::in );
	return get_out_id( message );
}
int MNode::get_out_id( const std::string& message ) {
	std::stringstream mssg( message );
	int answ;
	int command;
	mssg >> command >> answ;
	return answ;
}
void MNode::get_id( int& id, const char** argv ) {
	ExecArg::get_execv_arg< int >( id, argv );
}
// MNode& MNode::operator<<( MNode& rhs ) { //??can he
// 	AmqpClient::Envelope::ptr_t message;
// 	const int timeout = 10;
// 	while( rhs.channel[ queue::in ]->BasicConsumeMessage( rhs.consumer_tag, message, timeout ) ) {
// 		send_message( message->Message()->Body() );
// 	}
// 	return *this;
// }
bool MNode::exist( int link ) const {
	return link_exists[ link ];
}



static int __time = 0;
bool MNode::get_message( int link /* = -1 */, std::string& message,
                         bool wait /* = true */, MNode::consumer_e _consumer /* = default_arg */ ) {
	//I could make variant with consumer != -1, but I don't need to
	AmqpClient::Envelope::ptr_t envelope;
	std::vector< std::string > consumers;
	const int in = static_cast< int >( consumer_e::in );
	const int ping_in = static_cast< int >( consumer_e::ping_in );
	const int consumer = static_cast< int >( _consumer );
	if( link == -1 ) {
		if( _consumer == MNode::consumer_e::default_arg ) { //all available
			for( int i = 0; i < links_per_node; ++i ) {
				if( link_exists[ i ] ) {
					consumers.push_back( consumer_tag[ i ][ in ] );
					consumers.push_back( consumer_tag[ i ][ ping_in ] );
				}
			}
		} else {
			for( int i = 0; i < links_per_node; ++i ) {
				if( link_exists[ i ] ) {
					consumers.push_back( consumer_tag[ i ][ consumer ] );
				}
			}
		}
	} else { //link != -1
		if( _consumer == MNode::consumer_e::default_arg ) { //all available
			if( link_exists[ link ] ) {
				consumers.push_back( consumer_tag[ link ][ in ] );
				consumers.push_back( consumer_tag[ link ][ ping_in ] );
			}
		} else {
			if( link_exists[ link ] ) {
				consumers.push_back( consumer_tag[ link ][ consumer ] );
			}
		}
	}
	// for( const auto& i : consumers ) {
	// 	try {
	// 		channel->BasicConsume( i, i, true, false, true, 100 );
	// 	} catch ( ... ) {}
	// }
	if( consumers.empty() ) {
		return false;
	}
	if( __time == 0 ) {
		std::cerr << "id " << id[ ID::my ] << ": consumers";
		for( auto i : consumers ) std::cerr << " id " << id[ ID::my ] << ": <" << i << '>';
		std::cerr << '\n';
		__time = 1;
	}
	try {
		if( wait ) {

			envelope = channel->BasicConsumeMessage( consumers ); //error? what if consumer_tag[ i ] == ""?
			channel->BasicAck( envelope );
			message = envelope->Message()->Body();
			__time = 0;
			return true;
		} else {
			const int timeout = 100;
			if ( channel->BasicConsumeMessage( consumers, envelope, timeout ) ) {
				channel->BasicAck( envelope );
				message = envelope->Message()->Body();
				__time = 0;
				return true;
			} else {
				return false;
			}
		}
	} catch ( AmqpClient::ConsumerTagNotFoundException exc ) {
		std::cerr << "id " << id[ ID::my ] << ": " << exc.what() << '\n';
		return false;
	}
}

// void MNode::ping_reply() {
// 	AmqpClient::Envelope::ptr_t message;
// 	while( channel[ queue::ping_in ]->BasicGet( message, queue_name[ queue::ping_in ] ) ) {
// 		channel[ queue::ping_out ]->BasicPublish( exchange_name[ queue::ping_out ],
// 		                                       routing_key, AmqpClient::BasicMessage::Create( MNode::instant_ping_reply() ) );
// 	}
// }
// bool MNode::instant_ping( const std::string& message ) {
// 	return message == "p::";
// }

void MNode::send_instant_ping_reply( const std::string& command ) {
	std::stringstream mssg( command );
	int useless;
	int _id;
	bool check_connection;
	mssg >> useless >> _id;
	std::stringstream send_mssg; // why can't I just use mssg( "" )??
	send_mssg << static_cast< int >( Command::instant_ping_reply ) << ' ' << _id;
	for( int link = 0; link < links_per_node; ++link ) {
		if( link_exists[ link ] && ( id[ link ] == _id ) ) {
			send_message( link, send_mssg.str(),
			              check_connection = false, routing_key_e::ping_out );
			return;
		}
	}
	std::cerr << "instant_ping_reply failed\n";
}
// std::string MNode::instant_ping_reply( const std::string& message ) {
// 	std::stringstream mssg;
// 	mssg << static_cast< int >( Command::reply_c ) << ' ' << message;
// 	return mssg.str();
// }

MNode::Command MNode::get_command( const std::string& message ) {
	std::stringstream mssg;
	mssg.str( message );
	int command;
	mssg >> command;
	return static_cast< MNode::Command >( command );
}
int MNode::get_id( const std::string& message ) {
	std::stringstream mssg( message );
	int command;
	int id;
	mssg >> command;
	mssg >> id;
	return id;
}
void MNode::send_message( int link, const std::string& message,
                          bool check_connection, MNode::routing_key_e _rout /* =routing_key_e::out */ ) { //if don't have id
	const int rout = static_cast< int >( _rout );
	std::cerr << "id " << id[ static_cast< int >( ID::my ) ]
	          << ": sending message to id " << id[ link ] << ":\"" << message << "\"\n";
	if( !check_connection ) {
		channel->BasicPublish( exchange_name,
		                       routing_key[ link ][ rout ],
		                       AmqpClient::BasicMessage::Create( message ) );
	} else if( ping( link ) ) {
		channel->BasicPublish( exchange_name, routing_key[ link ][ rout ],
		                       AmqpClient::BasicMessage::Create( message ) );
	} else {
		std::string error_message = MNode::error_id_unreachable( id[ link ], id[ ID::my ]);
		//throw message somewhere
		const int err = static_cast< int >( routing_key_e::err );
		channel->BasicPublish( exchange_name, routing_key[ link ][ err ],
		                       AmqpClient::BasicMessage::Create( error_message ) );
	}
}
bool MNode::ping( int link ) {
	bool check_connection;
	send_message( link, instant_ping_message(),
	              check_connection = false, routing_key_e::ping_out );
	// channel->BasicPublish( exchange_name, routing_key[ link ][ routkeyping_out ],
	//                        AmqpClient::BasicMessage::Create( MNode::instant_ping_message() ) );
	AmqpClient::Envelope::ptr_t message; //maybe wait?
//	sleep( 5 );
	bool wait;
	std::string _message;
	return get_message( link, _message, wait = false, consumer_e::ping_in );

//	return channel[ queue::ping_in ]->BasicGet( message, queue_name[ queue::ping_in ] );
	//i donno if this would work
	//1) if time between sending and receiving would be too big
	//2) if I would already have some ping mesages at queue
}
std::string MNode::instant_ping_message() {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::instant_ping ) << ' ' << id[ ID::my ];
	return mssg.str();
}
// bool MNode::instant_ping_reply( const std::string& message ) {
// 	return message == "p::1";
// }

std::string MNode::error_id_unreachable( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' ' <<
		id_my << ' ' << static_cast< int >( MLError::id_unreachable ) << ' ' << id_out;
	return mssg.str();
}
void MNode::attach_worker( int link, const std::string& command ) {
	std::stringstream mssg( command );
	int useless;
	int _id;
	mssg >> useless; //command
	mssg >> _id;
	int pid; // size( pid_t ) > size( int ) ?
	mssg >> pid;
	attach_worker( link, _id, pid );
}

void MNode::attach_worker( int link, int out_id, pid_t pid ) {
	id[ link ] = out_id;
	bool swap;
	create( link, pid, swap = true );
	delete_link[ link ] = false;
	std::string mssg_id = MNode::id_out_message( id[ static_cast< int >( ID::my ) ] );
	bool check_connection;
	send_message( link, mssg_id, check_connection = false );
}
std::string MNode::id_out_message( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::id_c ) << ' ' << id;
	return mssg.str();
}


std::string MNode::attach_reply_message( const std::string& message, int my_id ) {
	std::stringstream mssg( message );
	std::stringstream replymssg;
	int another_id;
	int pid;
	int command;
	mssg >> command >> another_id >> pid;
	replymssg << static_cast< int >( Command::reply_c ) << ' '<< static_cast< int >( Command::attach_c )
	          << ' ' << my_id << ' ' << another_id << ' ' << pid << ' ' << static_cast< int >( Reply::ok );
	return replymssg.str();
}

std::string MNode::error_id_doesnt_exist( int id_out, int id_my ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' '
	     << id_my << ' ' << static_cast< int >( MLError::id_doesnt_exist ) << ' ' << id_out;
	return mssg.str();
}

std::string MNode::error_id_already_exist( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::error_c ) << ' ' << id << static_cast< int >( MLError::id_already_exist );
	return mssg.str();
}

MNode::ExecCommand MNode::get_exec_command( const std::string& message ) {
	std::stringstream mssg( message );
	int useless;
	int command;
	mssg >> useless; //command
	mssg >> useless; //id
	mssg >> command;
	return static_cast< ExecCommand >( command );
}

std::string MNode::exec_reply_message( int time, int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::reply_c ) << ' '
	     << static_cast< int >( Command::exec_c ) << ' '
	     << id << ' ' << static_cast< int >( command ) << ' '
	     << time << ' ' << static_cast< int >( Reply::ok );
	return mssg.str();
}

std::string MNode::exec_reply_message( int id, ExecCommand command ) {
	std::stringstream mssg;
	mssg  << static_cast< int >( Command::reply_c ) << ' '
	      << static_cast< int >( Command::exec_c ) << ' '
	      << id << ' ' << static_cast< int >( command ) << ' '
	      << static_cast< int >( Reply::ok );
	return mssg.str();
}

std::string MNode::ping_reply_message( int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::reply_c ) << ' '
	     << static_cast< int >( Command::ping_c ) << ' '
	     << id << ' ' << static_cast< int >( Reply::ok );
	return mssg.str();
}

std::string MNode::make_reply( const std::string& reply ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::reply_c ) << ' ' << reply;
	return mssg.str();
}






void MNode::command_close( int link, int _id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::close_c ) << ' ' << id[ link ];
	bool check_connection;
	send_message( link, mssg.str(), check_connection = false );
}
pid_t MNode::create_worker( int _id ) {
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

void MNode::command_attach_worker( int link, int id, pid_t pid ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::attach_c ) << ' ' << id << ' ' << pid;
	bool check_connection;
	send_message( link, mssg.str(), check_connection = true );
}
void MNode::command_exec( int link, int id, const std::string& command ) {
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
	bool check_connection;
	send_message( link, mssg.str(), check_connection = true );
}
void MNode::command_ping( int link, int id ) {
	std::stringstream mssg;
	mssg << static_cast< int >( Command::ping_c ) << ' ' << id;
	bool check_connection;
	send_message( link, mssg.str(), check_connection = true );
}

MNode::~MNode() {
	for( int link = 0; link < links_per_node; ++link ) {
		if( link_exists[ link ] ) {
			if( !delete_link[ link ] ) {
				command_close( link );
			} else {
				// std::cerr << "id " << id[ static_cast< int >( ID::my ) ] << "->"
				//           << id[ link ] << " start cancelling...";

				for( int consumer = 0; consumer < consumers_per_link /* = 3 */; ++consumer )
					channel->BasicCancel( consumer_tag[ link ][ consumer ] );

				// std::cerr << "success; " << std::endl;
				//unbind?
				channel->DeleteExchange( exchange_name );
				for( int i = 0; i < queues_per_link; ++i ) {
					std::cerr << "id " << id[ static_cast< int >( ID::my ) ] << "->"
					          << id[ link ]<< " start deleting queue...";

					channel->DeleteQueue( queue_name[ link ][ i ] );

					std::cerr << "success; " << std::endl;
				}
			}
		}
	}
}










std::string MNode::parse_reply( const std::string& message ) {
	int command_reply, command_i, id, other_id;
	ExecCommand exec_command;
	std::stringstream mssg( message );
	std::stringstream return_mssg;
	mssg >> command_reply;
	if( command_reply != static_cast< int >( Command::reply_c ) ) {
		std::cerr << "parse_reply fail\n";
		return "";
	}
	mssg >> command_i >> id;
	Command command = static_cast< Command >( command_i );
	int error_type_i;
	MLError error_type;
	switch( command ) {
		case Command::attach_c:
			int pid;
			mssg >> other_id >> pid;
			return_mssg << "Ok: added worker with id = " << other_id << ", pid = " << pid << " to worker with id = " << id << '\n';
			break;
		case Command::exec_c:
			int exec_command_i;
			mssg >> exec_command_i;
			exec_command = static_cast< ExecCommand >( exec_command_i );
			if( exec_command == ExecCommand::time ) {
				int time;
				mssg >> time;
				return_mssg << "Ok: " << id << " time = " << time << '\n';
			} else if( exec_command == ExecCommand::stop ){
				return_mssg << "Ok: " << id << " timer stopped\n";
			} else if( exec_command == ExecCommand::start ){
				return_mssg << "Ok: " << id << " timer started\n";
			} else {
				return_mssg << "Error: operator<<\n";
			}
			break;
		case Command::ping_c:
			return_mssg << "Ok: worker with id = " << id << " is available\n";
			break;
		case Command::error_c:
			mssg >> error_type_i;
			error_type = static_cast< MLError >( error_type_i );
			if( error_type == MLError::id_unreachable ) {
				mssg >> other_id;
				return_mssg << "Error: " << id << ": id = " << other_id << " is unreachable\n";
			} else if( error_type == MLError::id_doesnt_exist ) {
				mssg >> other_id;
				return_mssg << "Error: " << id << ": id = " << other_id << " doesn't exist\n";
			} else if( error_type == MLError::id_already_exist ) {
				return_mssg << "Error: id = " << id << " already exist";
			}
			break;
		default:
			std::cerr << "parse_reply fail, unknown reply\n";
			break;
	}
	return return_mssg.str();
}
void MNode::kill_process( const std::string& message ) {
	int command, id, pid; //attach
	std::stringstream mssg( message );
	mssg >> command >> id >> pid;
	kill( pid, SIGKILL );
}

void MNode::set_my_id( int _id ) {
	id[ ID::my ] = _id;
}
