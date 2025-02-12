// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <chrono>
#include <iostream>
#include <thread>

#include <actor/actor.hpp>
#include <actor/message.hpp>

using namespace std;
using namespace std::chrono_literals;

template <typename T>
void ilog(const char* prefix, const T& n)
{
    static mutex m;
    lock_guard lock { m };
    cout << prefix << n << '\n';
}

int main(int argc, const char* argv[])
{
    actor::Actor a = [](actor::Receiver inbox) {
        for (actor::Message& mesg: inbox)
            mesg.match<int>([](int value) { ilog("a: ", value); })
                .match<bool>([](bool b) { ilog("a: ", b); })
                .match<string>([](const string& s) { ilog("a: ", s); })
                // yeah, just to demo the capabilities ;-D
                ;
    };

    actor::Actor b = [&](actor::Receiver receive) {
        for (actor::Message& mesg: receive)
            mesg.match<int>([&](int value) {
                ilog("b: ", value);
                a << value * 10;
            });
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
