# C++17 Actor Model header-only library

This is a C++17 header-only Actor Model library. It was written purely out of fun.

### Example

```cpp
#include <actor/actor.hpp>
#include <cstdlib>
#include <iostream>

int main()
{
    auto logger = actor::Actor([](actor::Receiver receiver) {
        for (actor::Message& mesg : receiver)
            mesg.match<string>([](string const& s) { std::cout << "LOG(str): " << s << '\n'; })
                .match<int>([](int val) { std::cout << "LOG(num): " << val << '\n'; })
                .match<bool>([](bool val) { std::cout << "LOG(bool): " << boolalpha << val << '\n'; })
                .otherwise([] { std::cout << "LOG(?): Unhandled!\n"; });
    });

    logger << std::string("Hello, World");
    logger << 42;
    logger << true;
    logger << 2.81;

    return EXIT_SUCCESS;
}
```

### References

* https://www.brianstorti.com/the-actor-model/
* https://www.justsoftwaresolutions.co.uk/threading/all-the-worlds-a-stage.html
