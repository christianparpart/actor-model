// SPDX-License-Identifier: Apache-2.0
#include <reflection-cpp/reflection.hpp>

#include <chrono>
#include <future>
#include <print>

#include <actor/channel.hpp>

void join(auto&... threads)
{
    (threads.join(), ...);
}

void repeat(auto const& callable)
{
    while (callable())
    {
        ;
    }
}

// clang-format off
struct Plus { double a; double b; };
struct Minus { double a; double b; };
struct Multiply { double a; double b; };
struct Divide { double a; double b; };
struct Quit {};
// clang-format on

struct Calculator
{
    channel::Controller& controller;

    void operator()(channel::Channel<Plus>& channel)
    {
        if (auto const expr = channel.try_receive(); expr.has_value())
            std::println("{} ({}) ~> {}", channel.name(), Reflection::Inspect(*expr), expr->a + expr->b);
    }

    void operator()(channel::Channel<Minus>& channel)
    {
        if (auto const expr = channel.try_receive(); expr.has_value())
            std::println("{} ({}) ~> {}", channel.name(), Reflection::Inspect(*expr), expr->a - expr->b);
    }

    void operator()(channel::Channel<Multiply>& channel)
    {
        if (auto const expr = channel.try_receive(); expr.has_value())
            std::println("{} ({}) ~> {}", channel.name(), Reflection::Inspect(*expr), expr->a * expr->b);
    }

    void operator()(channel::Channel<Divide>& channel)
    {
        if (auto const expr = channel.try_receive(); expr.has_value())
            std::println("{} ({}) ~> {:.2f}",
                         channel.name(),
                         Reflection::Inspect(*expr),
                         std::abs(expr->b) <= std::numeric_limits<double>::epsilon()
                             ? std::numeric_limits<double>::infinity()
                             : expr->a / expr->b);
    }

    void operator()(channel::Channel<Quit>& /*channel*/)
    {
        std::println("Quit received");
        controller.terminate();
    }
};

int main()
{
    std::srand(std::time(nullptr));

    auto controller = channel::Controller {};

    auto plusChan = controller.channel<Plus>(channel::MessageBufferSize { 1 }, "Plus");
    auto minusChan = controller.channel<Minus>(channel::MessageBufferSize { 1 }, "Minus");
    auto multiplyChan = controller.channel<Multiply>(channel::MessageBufferSize { 1 }, "Multiply");
    auto divideChan = controller.channel<Divide>(channel::MessageBufferSize { 1 }, "Divide");
    auto quitChan = controller.channel<Quit>(channel::MessageBufferSize { 1 }, "Quit");

    auto exampleProducer = std::async(std::launch::async, [&] {
        for (int i = 1; i <= 10; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(125));
            auto const randomA = static_cast<double>(std::rand() % 100);
            auto const randomB = static_cast<double>(std::rand() % 100);
            switch (std::rand() % 4) // NOLINT(bugprone-switch-missing-default-case)
            {
                case 0:
                    plusChan.send(Plus { .a = randomA, .b = randomB });
                    break;
                case 1:
                    minusChan.send(Minus { .a = randomA, .b = randomB });
                    break;
                case 2:
                    multiplyChan.send(Multiply { .a = randomA, .b = randomB });
                    break;
                case 3:
                    divideChan.send(Divide { .a = randomA, .b = randomB });
                    break;
            }
        }
        quitChan.send(Quit {});
    });

    auto calc = Calculator { controller };
    repeat([&] { return controller.select(calc, plusChan, minusChan, multiplyChan, divideChan, quitChan); });

    exampleProducer.wait();

    return EXIT_SUCCESS;
}
