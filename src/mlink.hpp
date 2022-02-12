#pragma once
#include <iostream>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <ostream>

class MLink;
std::ostream& operator<<( std::ostream&, MLink& );
class MLink {
	using Channel = AmqpClient::Channel;
	public:
		enum Link {
			up = 0,
			left = 1,
			right = 2,
	    };
		enum class Command {
		    id_c,
			attach_c,
			exec_c,
			ping_c,
			close_c,
			error_c,
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
		MLink();
		void create( pid_t, int, bool = false, bool = false );
		void get_message( std::string& );
		// void send_message( std::string&, int );
		void send_message( const std::string&, bool = false );
		bool exist() const;
		bool worker_reachable( int );
		MLink& operator<<( MLink& );
		void attach_worker( int, int, pid_t );
		void attach_worker( const std::string&, int);
		void command_attach_worker( int, pid_t );
		void command_exec( int, const std::string& );
		void command_ping( int );
		void command_close( int );
		void command_close();
		void ping_reply();
		bool ping();
		~MLink();
		friend std::ostream& operator<<( std::ostream&, MLink& );

		static std::string id_out_message( int );
		static Command get_command( const std::string& );
		static void get_id( int&, const char** );
		static int get_id( const std::string& );
		static ExecCommand get_exec_command( const std::string& );
		static std::string exec_reply_message( int, int, ExecCommand );
		static std::string exec_reply_message( int, ExecCommand );
		static std::string ping_reply_message( int );
		static std::string attach_reply_message( const std::string&, int );
		static std::string error_id_doesnt_exist( int, int );
		static std::string error_id_unreachable( int, int );
		static std::string error_id_already_exist( int );
		// static bool instant_ping( const std::string& message );
		// static bool instant_ping_reply( const std::string& message);
		static std::string instant_ping_message();
		static std::string instant_ping_reply();
		static pid_t create_worker( int );
		static void kill_process( const std::string& );
private:
	enum ch {
		in = 0,
		out = 1,
		ping_in = 2,
		ping_out = 3,
    };
	enum class ID {
		my = 0,
		out = 1,
	};
	enum class Reply {
		ok = 0,
	};
	int id[ 2 ];
	Channel::ptr_t channel[ 4 ];
	// std::string prefix
	std::string queue_name[ 4 ];
	std::string queue_broker_name[ 4 ];
	std::string exchange_name[ 4 ];
	std::string consumer_tag;
	bool connection_exists;
	bool delete_queue;
	std::string routing_key;
	void create_queue( MLink::ch direction, const std::string& suffix );
	int get_out_id();
	int get_out_id( const std::string& message );
};
