// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include <actor/message.h>

namespace actor
{

class Actor;
class Receiver
{
  public:
    Receiver(Actor& actor):
        actor_ { actor }
    {
    }

    std::optional<Message> receive();

    struct iterator
    {
        Actor& actor;
        Message value;
        bool eos = false;

        bool operator==(const iterator& rhs) const noexcept
        {
            return eos == rhs.eos;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return eos != rhs.eos;
        }

        const Message& operator*() const
        {
            return value;
        }
        Message& operator*()
        {
            return value;
        }

        iterator& operator++();
    };

    iterator begin();
    iterator end();

  private:
    Actor& actor_;
};

class Actor
{
  public:
    using Handler = std::function<void(Receiver)>;

    template <typename T>
    Actor(const T& handler);

    template <typename T>
    Actor(T&& handler);

    Actor() = default;
    Actor(const Actor&) = default;
    Actor(Actor&&) = default;
    Actor& operator=(const Actor&) = delete;
    Actor& operator=(Actor&&) = delete;
    ~Actor();

    void send(Message&& message);
    Actor& operator<<(Message&& m);

    bool killing() const noexcept
    {
        return killing_.load();
    }
    std::optional<Message> receive();

  private:
    void main();

  private:
    Handler handler_;
    std::atomic<bool> killing_;
    std::deque<Message> inbox_;
    std::thread thread_;
    std::condition_variable condition_;
    std::mutex lock_;
};

template <typename T>
inline Actor::Actor(const T& handler):
    Actor(T { handler })
{
}

template <typename T>
inline Actor::Actor(T&& handler):
    handler_ { std::move(handler) },
    killing_ { false },
    inbox_ {},
    thread_ { std::bind(&Actor::main, this) }
{
}

inline Actor::~Actor()
{
    killing_.store(true);
    condition_.notify_one();
    thread_.join();
}

inline void Actor::main()
{
    handler_(Receiver { *this });
}

inline std::optional<Message> Actor::receive()
{
    std::unique_lock lock { lock_ };
    condition_.wait(lock, [this]() { return !inbox_.empty() || killing_.load(); });
    if (!inbox_.empty())
    {
        Message m = std::move(inbox_.front());
        inbox_.pop_front();
        return std::move(m);
    }
    return std::nullopt;
}

inline void Actor::send(Message&& message)
{
    std::unique_lock lock { lock_ };
    inbox_.emplace_back(std::move(message));
    condition_.notify_one();
}

inline Actor& Actor::operator<<(Message&& m)
{
    send(std::move(m));
    return *this;
}

inline std::optional<Message> Receiver::receive()
{
    return actor_.receive();
}

inline Receiver::iterator& Receiver::iterator::operator++()
{
    if (std::optional<Message> m = actor.receive())
        value = std::move(*m);
    else
        eos = true;

    return *this;
}

inline Receiver::iterator Receiver::begin()
{
    if (auto mesg = actor_.receive())
        return iterator { actor_, std::move(*mesg), false };
    else
        return end();
}

inline Receiver::iterator Receiver::end()
{
    return iterator { actor_, {}, true };
}

} // namespace actor
