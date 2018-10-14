// (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <any>
#include <cstddef>
#include <vector>

namespace actor {

class Message {
  public:
	template <typename T>
	Message(const T& val) : value_{val}
	{
	}

	template <typename T>
	Message(T&& val) : value_{std::move(val)}
	{
	}

	Message() = default;
	Message(const Message&) = default;
	Message(Message&&) = default;
	Message& operator=(const Message&) = default;
	Message& operator=(Message&&) = default;

	template <typename T>
	bool is() const noexcept
	{
		return typeid(T) == value_.type();
	}

	template <typename T>
	auto get()
	{
		return std::any_cast<T>(value_);
	}

	template <typename T>
	const auto get() const
	{
		return std::any_cast<T>(value_);
	}

	template <typename T, typename U>
	Message& match(U f)
	{
		if (is<T>())
			f(get<T>());

		return *this;
	}

	// TODO: add Visitor pattern
	// message.visit(Vistor{ .... });

  private:
	std::any value_;
};

}  // namespace actor
