// SPDX-License-Identifier: Apache-2.0
#include <chrono>
#include <print>
#include <thread>

#include <actor/channel.hpp>

int main()
{
    auto controller = comms::ChannelController {};
    auto channel = controller.channel<int>(comms::ChannelBufferSize { 1 });

    auto sender = std::thread { [&channel] {
        std::println("Sending messages:");
        for (int i = 1; i <= 5; ++i)
        {
            if (i > 1)
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            channel.send(i);
        }
        std::println("Sending messages done.");
        channel.close();
    } };

    auto receiver = std::thread { [&channel] {
        std::println("Receiving messages:");
        while (auto message = channel.receive())
            std::println("Received message: {}", message.value());
        std::println("Receiving messages done.");
    } };

    sender.join();
    receiver.join();

    return EXIT_SUCCESS;
}
