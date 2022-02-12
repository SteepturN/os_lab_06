
yess:
	g++ -g -Wall -pedantic mlink.cpp error_string.cpp exec_arg.cpp timer.cpp worker.cpp -lSimpleAmqpClient -lboost_chrono -Wl,-rpath,/usr/local/lib -o worker
	g++ -g -Wall -pedantic mlink.cpp error_string.cpp exec_arg.cpp server.cpp -lSimpleAmqpClient -lboost_chrono -Wl,-rpath,/usr/local/lib -o server
ld:
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
	export LD_LIBRARY_PATH
reset:
	sudo rabbitmqctl list_queues --vhost myvhost
	sudo rabbitmqctl stop_app
	sudo rabbitmqctl reset
	sudo rabbitmqctl start_app
	sudo rabbitmqctl add_user newuser newuser
	sudo rabbitmqctl add_vhost myvhost
	sudo rabbitmqctl set_permissions -p "myvhost" "newuser" '.*' '.*' '.*'
	sudo rabbitmqctl list_queues --vhost myvhost
