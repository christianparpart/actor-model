// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <print>
#include <thread>

#include <actor/channel.hpp>

template <typename... Ts>
void join(Ts&... threads)
{
    (threads.join(), ...);
}

int main()
{
    auto controller = comms::ChannelController {};

    auto channelA = controller.channel<int>(comms::ChannelBufferSize { 1 });
    auto senderA = std::thread { [&channelA] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            channelA.send(i);
        }
        channelA.close();
    } };

    auto channelB = controller.channel<std::string>(comms::ChannelBufferSize { 1 });
    auto senderB = std::thread { [&channelB] {
        for (auto const* name: { "Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace", "Heidi", "Ivan", "Judy" })
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(125));
            channelB.send(name);
        }
        channelB.close();
    } };

    auto channelC = controller.channel<double>(comms::ChannelBufferSize { 1 });
    auto senderC = std::thread { [&channelC] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            channelC.send(i * std::numbers::pi);
        }
        channelC.close();
    } };

    while (controller.select(
        []<typename T>(comms::Channel<T>& channel) {
            if (auto const message = channel.try_receive(); message.has_value())
                std::println("Received message from channel: {}", message.value());
        },
        channelA,
        channelB,
        channelC))
    {
        // Do nothing.
    }

    join(senderA, senderB, senderC);

    return EXIT_SUCCESS;
}
