#pragma once
#include <ctime>

class Timer {
	public:
		Timer() = default;
		int time() const;
		void stop();
		void start();
	private:
		time_t start_time;
		int add_time = 0;
		bool timer_is_on = false;
};
