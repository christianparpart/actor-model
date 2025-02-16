// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <print>
#include <thread>

#include <actor/channel.hpp>

int main()
{
    auto channel = comms::Channel<int> { comms::ChannelBufferSize { 1 } };
    auto sender = std::thread { [&channel] {
        std::println("Sending messages:");
        for (int i = 1; i <= 5; ++i)
        {
            channel.send(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::println("Sending messages done.");
        channel.close();
    } };

    auto receiver = std::thread { [&channel] {
        std::println("Receiving messages:");
        while (auto value = channel.receive())
        {
            std::println("Received message: {}", value.value());
        }
        std::println("Receiving messages done.");
    } };

    sender.join();
    receiver.join();
    return EXIT_SUCCESS;
}
