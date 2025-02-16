// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <format>
#include <print>
#include <thread>
#include <tuple>

#include <actor/channel.hpp>

int main()
{
    auto controller = comms::ChannelController {};

    auto channelA = controller.channel<int>(comms::ChannelBufferSize { 1 });
    auto senderA = std::thread { [&channelA] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            channelA.send(i);
        }
        channelA.close();
    } };

    auto channelB = controller.channel<std::string>(comms::ChannelBufferSize { 1 });
    auto senderB = std::thread { [&channelB] {
        for (auto const* name: { "Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace", "Heidi", "Ivan", "Judy" })
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            channelB.send(name);
        }
        channelB.close();
    } };

    auto channelC = controller.channel<double>(comms::ChannelBufferSize { 1 });
    auto senderC = std::thread { [&channelC] {
        for (int i = 1; i <= 5; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(750));
            channelC.send(i * std::numbers::pi);
        }
        channelC.close();
    } };

    while (controller.select(
        []<typename T>(comms::Channel<T>& channel) {
            if (auto const value = channel.try_receive(); value.has_value())
                std::println("Received message from channel: {}", value.value());
        },
        channelA,
        channelB,
        channelC))
    {
        // Do nothing.
    }

    senderA.join();
    senderB.join();
    senderC.join();

    return EXIT_SUCCESS;
}
