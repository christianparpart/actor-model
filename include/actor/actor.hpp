// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <any>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace actor
{

/// A message that can be sent to an actor.
class Message
{
  public:
    template <typename T>
        requires(!std::same_as<std::decay_t<T>, Message>)
    Message(T&& val):
        _value { std::forward<T>(val) }
    {
    }

    Message() = default;
    Message(Message&&) = default;
    Message(Message const&) = delete;
    Message& operator=(Message&&) = default;
    Message& operator=(Message const&) = delete;
    ~Message() = default;

    template <typename T>
    [[nodiscard]] bool is() const noexcept
    {
        return typeid(T) == _value.type();
    }

    template <typename T>
    decltype(auto) get()
    {
        return std::any_cast<T>(_value);
    }

    /// Tests if underlying value is of type @p T and invokes @p f if so.
    template <typename T, typename U>
    Message& match(U f)
    {
        if (is<T>() && !_matched)
        {
            _matched = true;
            f(get<T>());
        }

        return *this;
    }

    /// Invokes given handler function @p f when no prior match<>() function matched the underlying type.
    template <typename U>
    void otherwise(U f)
    {
        if (!_matched)
            f();
    }

    /// Expects given type @p T and invokes handler function @p f on underlying value.
    ///
    /// @throw std::bad_any_cast as get<T>() throws because of std::any_cast<T>().
    template <typename T, typename U>
    void expect(U f)
    {
        f(get<T>());
    }

    // TODO: add Visitor pattern
    // message.visit(Vistor{ .... });

  private:
    std::any _value;
    bool _matched = false;
};

class Actor;

class Receiver
{
  public:
    Receiver(Actor& actor):
        _actor { actor }
    {
    }

    std::optional<Message> receive();

    struct iterator
    {
        Actor& actor;
        Message value;
        bool eos = false;

        bool operator==(iterator const& rhs) const noexcept
        {
            return eos == rhs.eos;
        }

        bool operator!=(iterator const& rhs) const noexcept
        {
            return eos != rhs.eos;
        }

        Message const& operator*() const
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
    Actor& _actor;
};

/// An actor that can receive messages.
///
/// An actor is a lightweight object that can receive messages and process them in a separate thread.
/// The actor is created with a handler function that is invoked in the actor's thread.
class Actor
{
  public:
    using Handler = std::function<void(Receiver)>;

    template <typename T>
    explicit Actor(T&& handler);

    Actor() = delete;
    Actor(Actor&&) = delete;
    Actor(Actor const&) = delete;
    Actor& operator=(Actor&&) = delete;
    Actor& operator=(Actor const&) = delete;
    ~Actor();

    void send(Message&& message);
    Actor& operator<<(Message&& message);

    [[nodiscard]] bool killing() const noexcept
    {
        return _killing.load();
    }

    std::optional<Message> receive();

  private:
    void main();

  private:
    Handler _handler;
    std::atomic<bool> _killing;
    std::deque<Message> _inbox;
    std::thread _thread;
    std::condition_variable _condition;
    std::mutex _lock;
};

template <typename T>
inline Actor::Actor(T&& handler):
    _handler { std::forward<T>(handler) },
    _killing { false },
    _inbox {},
    _thread { std::bind(&Actor::main, this) }
{
}

inline Actor::~Actor()
{
    _killing.store(true);
    _condition.notify_one();
    _thread.join();
}

inline void Actor::main()
{
    _handler(Receiver { *this });
}

inline std::optional<Message> Actor::receive()
{
    std::unique_lock lock { _lock };
    _condition.wait(lock, [this]() { return !_inbox.empty() || _killing.load(); });
    if (!_inbox.empty())
    {
        Message m = std::move(_inbox.front());
        _inbox.pop_front();
        return { std::move(m) };
    }
    return std::nullopt;
}

inline void Actor::send(Message&& message)
{
    std::unique_lock lock { _lock };
    _inbox.emplace_back(std::move(message));
    _condition.notify_one();
}

inline Actor& Actor::operator<<(Message&& message)
{
    send(std::move(message));
    return *this;
}

inline std::optional<Message> Receiver::receive()
{
    return _actor.receive();
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
    if (auto message = _actor.receive())
        return iterator {
            .actor = _actor,
            .value = std::move(*message),
            .eos = false,
        };
    else
        return end();
}

inline Receiver::iterator Receiver::end()
{
    return iterator { .actor = _actor, .value = {}, .eos = true };
}

} // namespace actor
