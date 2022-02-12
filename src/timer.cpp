#include "timer.hpp"
#include <ctime>
void Timer::start() {
	if( timer_is_on ) return;
	::time( &start_time );
	timer_is_on = true;
}

void Timer::stop() {
	if( !timer_is_on ) return;
	time_t end_time;
	::time( &end_time );
	add_time += difftime( end_time, start_time );
	timer_is_on = false;
}
int Timer::time() const {
	if( !timer_is_on ) return add_time;
	time_t end_time;
	::time( &end_time );
	return add_time + difftime( end_time, start_time );
}
