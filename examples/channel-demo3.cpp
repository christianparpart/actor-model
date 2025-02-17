// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <print>
#include <thread>

#include <actor/channel.hpp>

void join(auto&... threads)
{
    (threads.join(), ...);
}

void repeat_until(auto target, auto const& callable)
{
    while (callable() != target)
    {
        ;
    }
}

int main()
{
    auto controller = comms::ChannelController {};

    auto channelA = controller.channel<int>(comms::ChannelBufferSize { 1 }, "channelA");
    auto senderA = std::thread { [&channelA] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            channelA.send(i);
        }
        channelA.close();
    } };

    auto channelB = controller.channel<std::string>(comms::ChannelBufferSize { 1 }, "channelB");
    auto senderB = std::thread { [&channelB] {
        for (auto const* name: { "Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace", "Heidi", "Ivan", "Judy" })
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(125));
            channelB.send(name);
        }
        channelB.close();
    } };

    auto channelC = controller.channel<double>(comms::ChannelBufferSize { 1 }, "channelC");
    auto senderC = std::thread { [&channelC] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            channelC.send(i * std::numbers::pi);
        }
        channelC.close();
    } };

    repeat_until(false, [&] {
        return controller.select(
            []<typename T>(comms::Channel<T>& channel) {
                if (auto const message = channel.try_receive(); message.has_value())
                    std::println("Received message from {}: {}", channel.name(), message.value());
            },
            channelA,
            channelB,
            channelC);
    });

    join(senderA, senderB, senderC);

    return EXIT_SUCCESS;
}
