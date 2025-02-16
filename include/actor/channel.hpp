// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace comms
{

/// The buffer size of the channel.
struct ChannelBufferSize
{
    size_t value;
};

/// Thread-safe channel for sending and receiving messages.
///
/// @code
/// auto channel = Channel<int> { ChannelBufferSize { 1 } };
/// std::thread { [&channel] { channel.send(42); } }.detach();
/// std::thread { [&channel] { std::cout << channel.receive().value() << std::endl; } }.detach();
/// @endcode
template <typename T>
class Channel
{
  public:
    using value_type = T;

    explicit Channel(ChannelBufferSize = { .value = 1 });

    Channel(Channel&&) = default;
    Channel(Channel const&) = delete;
    Channel& operator=(Channel&&) = default;
    Channel& operator=(Channel const&) = delete;
    ~Channel() = default;

    /// Sends a message to the channel.
    ///
    /// If the channel is full, the caller will be blocked until the message can be sent.
    template <typename U>
        requires std::convertible_to<U, T>
    void send(U&& value);

    /// Receives a message from the channel.
    ///
    /// If the channel is empty, the caller will be blocked until a message is available.
    [[nodiscard]] std::optional<T> receive();

    [[nodiscard]] bool empty() const noexcept;

    /// Returns the current buffer size of the channel.
    [[nodiscard]] size_t size() const noexcept;

    /// Returns the maximum buffer size of the channel.
    [[nodiscard]] size_t capacity() const noexcept;

    /// Closes the channel.
    void close() noexcept;

  private:
    ChannelBufferSize _maxBufferSize;
    std::deque<T> _queue;
    std::mutex _lock;
    std::condition_variable _condition;
    std::atomic<bool> _terminating = false;
};

template <typename T>
Channel<T>::Channel(ChannelBufferSize maxBufferSize):
    _maxBufferSize { maxBufferSize }
{
}

template <typename T>
template <typename U>
    requires std::convertible_to<U, T>
void Channel<T>::send(U&& value)
{
    auto lock = std::unique_lock { _lock };
    _condition.wait(lock, [this]() { return _queue.size() < _maxBufferSize.value || _terminating.load(); });
    _queue.emplace_back(std::forward<U>(value));
    _condition.notify_one();
}

template <typename T>
std::optional<T> Channel<T>::receive()
{
    auto lock = std::unique_lock { _lock };
    _condition.wait(lock, [this]() { return !_queue.empty() || _terminating.load(); });

    if (_queue.empty())
        return std::nullopt;

    auto value = std::move(_queue.front());
    _queue.pop_front();
    _condition.notify_one();
    return value;
}

template <typename T>
inline bool Channel<T>::empty() const noexcept
{
    auto lock = std::unique_lock { _lock };
    return _queue.empty();
}

template <typename T>
inline size_t Channel<T>::size() const noexcept
{
    auto lock = std::unique_lock { _lock };
    return _queue.size();
}

template <typename T>
inline size_t Channel<T>::capacity() const noexcept
{
    auto lock = std::unique_lock { _lock };
    return _maxBufferSize.value;
}

template <typename T>
inline void Channel<T>::close() noexcept
{
    _terminating.store(true);
    _condition.notify_all();
}

} // namespace comms
