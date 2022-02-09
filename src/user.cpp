#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <unistd.h>
#include <iostream>

int main() {
	using Channel = AmqpClient::Channel;
	Channel::OpenOpts openopts = Channel::OpenOpts::FromUri( "amqp://localhost" );
	Channel::ptr_t channel = Channel::Open( openopts );
	std::string queue = channel->DeclareQueue( "myqueue", true, true, false, false );
	channel->DeclareExchange( "myexchange" );
	channel->BindQueue( "myqueue", "myexchange" );
	std::string consumer_tag = channel->BasicConsume( "myqueue" );
	std::string message = channel->BasicConsumeMessage( consumer_tag )->Message()->Body();
	std::cout << message<< std::endl;
	return 0;
}
