#pragma once
#include <iostream>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <ostream>

namespace Link {
	enum Link {
		left = 0,
		right = 1,
		up = 2,
	};
};
namespace ID {
	enum ID {
		left = Link::left,
		right = Link::right,
		up = Link::up,
		my = 3,
	};
};
class MNode;
std::ostream& operator<<( std::ostream&, MNode& );
class MNode { //amount of links, here 1 link == 4 queue
	using Channel = AmqpClient::Channel;
	public:
		enum class routing_key_e {
			out = 0,
			ping_out = 1,
			err = 2,
	    };
		enum class consumer_e {
		    default_arg = -1,
			in = 0,
			ping_in = 1,
		};
		enum class queue_e {
			in = 0,
			out = 1,
			ping_in = 2,
			ping_out = 3,
		};
		enum class Command {
		    id_c,
			attach_c,
			exec_c,
			ping_c,
			close_c,
			error_c,
			reply_c,
			instant_ping,
			undefined_c,
		};
		enum class ExecCommand {
			stop,
			time,
			start,
			undefined_ec,
		};
		enum class MLError {
			id_unreachable,
			id_doesnt_exist,
			id_already_exist,
		};
		MNode();
		void create_link( int, pid_t, int );
		// void send_message( std::string&, int );
		bool exist( int ) const;
		bool get_message( int, std::string&, bool = false,
		                  MNode::consumer_e = MNode::consumer_e::default_arg );
		void send_message( int, const std::string&, bool = true,
		                   MNode::routing_key_e = MNode::routing_key_e::out );
		bool ping( int );
		// bool worker_reachable( int );
		// MNode& operator<<( MNode& );
		void attach_worker( int, const std::string& );
		void attach_worker( int, int, pid_t );
		void command_attach_worker( int, int, pid_t );
		void command_exec( int, int, const std::string& );
		void command_ping( int, int );
		void command_close( int, int = ID::my );
		void command_close();
		void ping_reply();
		void set_my_id( int );
		void instant_ping_reply( const std::string& );
		~MNode();
		friend std::ostream& operator<<( std::ostream&, MNode& );

		static std::string instant_ping_reply();
		static Command get_command( const std::string& );
		static int get_id( const std::string& );
		static std::string instant_ping_message();
		static std::string id_out_message( int );
		static void get_id( int&, const char** );
		static ExecCommand get_exec_command( const std::string& );
		static std::string exec_reply_message( int, int, ExecCommand );
		static std::string exec_reply_message( int, ExecCommand );
		static std::string ping_reply_message( int );
		static std::string attach_reply_message( const std::string&, int );
		static std::string error_id_doesnt_exist( int, int );
		static std::string error_id_unreachable( int, int );
		static std::string error_id_already_exist( int );
		static std::string parse_reply( const std::string& );
		// static bool instant_ping( const std::string& message );
		// static bool instant_ping_reply( const std::string& message);
		static pid_t create_worker( int );
		static void kill_process( const std::string& );
private:
	enum class Reply {
	    ok = 0,
	    instant_reply = 1,
	};
	const int links_per_node;
	const int queues_per_link;
	const int consumers_per_link;
	std::vector< int > id;//[ 4 ];
	Channel::ptr_t channel;
	// std::string prefix
	std::vector< std::vector < std::string > > queue_name;//[ 3 ][ 4 ];
//	std::string queue_broker_name[ 3 ][ 4 ];
	std::vector< std::vector< std::string > > consumer_tag; //[ 3 ][ 2 ]
											 //so I can't use just vector
											 //because it could have "" in it,
											 //so I should copy all not-empty
											 //tags

	std::string exchange_name;
	std::vector< std::vector< std::string > > routing_key;//[ 3 ][ 2 ];

	std::vector< bool > link_exists;//[ 3 ];
	std::vector< bool > delete_link;//[ 3 ];
	void create_queue( int, MNode::queue_e, const std::string& );
	void create_consumer( int, MNode::queue_e, const std::string& );
	void create_publisher( int, MNode::queue_e, const std::string& );
	void create( bool, const std::string&, const std::string&,
	             pid_t, int, MNode::queue_e, bool, bool );
	void create( int, pid_t, bool = false );
	int get_out_id( int );
	int get_out_id( const std::string& message );
};
