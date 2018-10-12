// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <actor/actor.h>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace std;

using Time = std::chrono::time_point<std::chrono::system_clock>;

struct time_request {
	actor::Actor<Time>& sender;
};

int main(int argc, const char* argv[])
{
	actor::Actor<Time> client = [&](actor::Receiver<Time> receive) {
		const time_t t = chrono::system_clock::to_time_t(*receive());
		cout << "Response: " << put_time(localtime(&t), "%F %T") << '\n';
	};

	actor::Actor<time_request> server = [](actor::Receiver<time_request> receive) {
		while (optional<time_request> x = receive())
			x->sender << chrono::system_clock::now();
	};

	server << time_request{client};
}
