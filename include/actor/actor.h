// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <actor/message.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace actor {

class Actor;
class Receiver {
  public:
	Receiver(Actor& actor) : actor_{actor} {}

	std::optional<Message> receive();

	struct iterator {
		Actor& actor;
		Message value;
		bool eos = false;

		bool operator==(const iterator& rhs) const noexcept { return eos == rhs.eos; }
		bool operator!=(const iterator& rhs) const noexcept { return eos != rhs.eos; }

		const Message& operator*() const { return value; }
		Message& operator*() { return value; }

		iterator& operator++();
	};

	iterator begin();
	iterator end();

  private:
	Actor& actor_;
};

class Actor {
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

	bool killing() const noexcept { return killing_.load(); }
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

}  // namespace actor

#include "actor-impl.h"

// vim:ts=4:sw=4
