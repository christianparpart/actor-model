// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <chrono>
#include <iomanip>
#include <iostream>

#include <actor/actor.h>

using namespace std;

using Time = std::chrono::time_point<std::chrono::system_clock>;

struct time_request
{
    actor::Actor& sender;
};

int main(int argc, const char* argv[])
{
    actor::Actor client = [&](actor::Receiver receiver) {
        if (auto mesg = receiver.receive(); mesg.has_value())
            mesg->expect<Time>([](const Time& ti) {
                const time_t t = chrono::system_clock::to_time_t(ti);
                cout << "Response: " << put_time(localtime(&t), "%F %T") << '\n';
            });
    };

    actor::Actor server = [](actor::Receiver receiver) {
        for (actor::Message& mesg: receiver)
            mesg.expect<time_request>([](time_request tr) { tr.sender << chrono::system_clock::now(); });
    };

    server << time_request { client };
}
