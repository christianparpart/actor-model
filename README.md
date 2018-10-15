# C++17 Actor Model header-only library

This is a C++17 header-only Actor Model library. It was written purely out of fun.

### Example

```cpp
#include <actor/actor.h>
#include <cstdlib>
#include <iostream>

using namespace std;

int main() {
    actor::Actor logger = [](actor::Receiver receiver) {
        for (actor::Message& mesg : receiver)
            mesg.match<string>([](string const& s) { cout << "LOG(str): " << s << endl; })
                .match<int>([](int val) { cout << "LOG(num): " << val << endl; })
                .match<bool>([](bool val) { cout << "LOG(bool): " << boolalpha << val << endl; })
                .otherwise([] { cout << "LOG(?): Unhandled!\n"; });
    };

    logger << string("Hello, World");
    logger << 42;
    logger << true;
    logger << 2.81;

    return EXIT_SUCCESS;
}
```

### License

```
The MIT License (MIT)

Copyright (c) 2018 Christian Parpart <christian@parpart.family>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

### References

* https://www.brianstorti.com/the-actor-model/
* https://www.justsoftwaresolutions.co.uk/threading/all-the-worlds-a-stage.html
