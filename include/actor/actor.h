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

namespace actor {

template <typename Message>
using Receiver = std::function<std::optional<Message>()>;

template <typename Message>
class Actor {
  public:
	using Handler = std::function<void(Receiver<Message>)>;

	template <typename T>
	Actor(const T& handler);

	template <typename T>
	Actor(T&& handler);

	Actor() = default;
	Actor(const Actor<Message>&) = default;
	Actor(Actor<Message>&&) = default;
	Actor& operator=(const Actor<Message>&) = delete;
	Actor& operator=(Actor<Message>&&) = delete;
	~Actor();

	void send(const Message& message);
	void send(Message&& message);

	Actor<Message>& operator<<(const Message& m);
	Actor<Message>& operator<<(Message&& m);

  private:
	void main();
	std::optional<Message> receive();

  private:
	Handler handler_;
	std::atomic<bool> killing_;
	std::deque<Message> inbox_;
	std::thread thread_;
	std::condition_variable condition_;
	std::mutex lock_;
};

}  // namespace actor

#include "actor-impl.h"

// vim:ts=4:sw=4
