// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cstdlib>
#include <iostream>
#include <string>

#include <actor/actor.hpp>

using namespace std;

int main()
{
    actor::Actor logger = [](actor::Receiver receiver) {
        for (actor::Message& mesg: receiver)
            mesg.match<string>([](string const& s) { cout << "LOG(str): " << s << '\n'; })
                .match<char const*>([](char const* val) { cout << "LOG(cstr): " << val << '\n'; })
                .match<int>([](int val) { cout << "LOG(num): " << val << '\n'; })
                .match<float>([](float val) { cout << "LOG(float): " << val << '\n'; })
                .match<bool>([](bool val) { cout << "LOG(bool): " << boolalpha << val << '\n'; })
                .otherwise([] { cout << "LOG(?): Unhandled!\n"; });
    };

    logger << string("Hello, World");
    logger << "Hello, World";
    logger << 42;
    logger << true;
    logger << 2.81;
    logger << 3.14f;

    return EXIT_SUCCESS;
}

// vim:ts=4:sw=4
