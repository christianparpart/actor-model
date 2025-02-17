// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <format>
#include <print>
#include <thread>

#include <actor/channel.hpp>

int main()
{
    auto controller = channel::Controller {};

    auto channelA = controller.channel<int>(channel::MessageBufferSize { 1 });
    auto senderA = std::thread { [&channelA] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            channelA.send(i);
        }
        channelA.close();
    } };

    auto channelB = controller.channel<std::string>(channel::MessageBufferSize { 1 });
    auto senderB = std::thread { [&channelB] {
        for (auto const* name: { "Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace", "Heidi", "Ivan", "Judy" })
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            channelB.send(name);
        }
        channelB.close();
    } };

    while (true)
    {
        if (auto const selected = controller.select(channelA, channelB); !selected.empty())
        {
            for (auto const i: selected)
            {
                switch (i)
                {
                    case 0:
                        std::println("Received message from channel 1: {}", channelA.receive().value());
                        break;
                    case 1:
                        std::println("Received message from channel 2: {}", channelB.receive().value());
                        break;
                    default:
                        break;
                }
            }
        }
        else
            break;
    }

    senderA.join();
    senderB.join();

    return EXIT_SUCCESS;
}
