// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <actor/actor.h>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
using namespace std::chrono_literals;

void ilog(const char* prefix, int n)
{
	static mutex m;
	lock_guard lock{m};
	cout << prefix << n << endl;
}

int main(int argc, const char* argv[])
{
	actor::Actor<int> a = [](actor::Receiver<int> receive) {
		while (auto x = receive())
			ilog("a: ", *x);
	};

	actor::Actor<int> b = [&](actor::Receiver<int> receive) {
		while (auto x = receive())
		{
			ilog("b: ", *x);
			a << *x * 10;
		}
	};

	for (int i = 1; i < 10; ++i)
	{
		ilog("send: ", i);
		b << i;
		std::this_thread::sleep_for(10ms);
	}

	return EXIT_SUCCESS;
}

// vim:ts=4:sw=4
