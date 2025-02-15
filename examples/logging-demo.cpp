// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cstdlib>
#include <iostream>
#include <string>

#include <actor/actor.hpp>

int main()
{
    auto logger = actor::Actor { [](actor::Receiver receiver) {
        for (actor::Message& mesg: receiver)
            mesg.match<std::string>([](auto&& s) { std::cout << "LOG(str): " << s << '\n'; })
                .match<char const*>([](auto&& val) { std::cout << "LOG(cstr): " << val << '\n'; })
                .match<int>([](auto val) { std::cout << "LOG(num): " << val << '\n'; })
                .match<float>([](auto val) { std::cout << "LOG(float): " << val << '\n'; })
                .match<bool>([](auto val) { std::cout << "LOG(bool): " << std::boolalpha << val << '\n'; })
                .otherwise([] { std::cout << "LOG(?): Unhandled!\n"; });
    } };

    logger << std::string("Hello, World");
    logger << "Hello, World";
    logger << 42;
    logger << true;
    logger << 2.81;
    logger << 3.14F;

    return EXIT_SUCCESS;
}

// vim:ts=4:sw=4
