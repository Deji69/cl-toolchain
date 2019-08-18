#pragma once
#include <CLARA/Common.h>
#include <CLARA/Source.h>
#include <CLARA/Token.h>

namespace CLARA::CLASM {

class TokenStream {
public:
	template<typename T>
	struct iter : public std::random_access_iterator_tag {
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = T;
		using difference_type   = size_t;
		using pointer           = value_type*;
		using reference         = value_type&;

		T& stream;
		size_t index;

		constexpr iter(T& stream) :
			stream(stream), index(stream.size())
		{ }

		constexpr iter(T& stream, size_t index) :
			stream(stream), index(index)
		{ }

		constexpr auto ok()
		{
			return index < stream.size();
		}

		constexpr operator bool()
		{
			return ok();
		}

		constexpr auto& operator=(const iter& other)
		{
			assert(&stream == &other.stream);
			index = other.index;
			return *this;
		}

		constexpr auto& operator++()
		{
			++index;
			return *this;
		}

		constexpr auto& operator++(int)
		{
			auto tmp = *this;
			++index;
			return tmp;
		}

		constexpr auto& operator--()
		{
			if (index > 0) --index;
			return *this;
		}

		constexpr auto& operator--(int)
		{
			auto tmp = *this;
			if (index > 0) --index;
			return tmp;
		}

		constexpr auto& operator+=(difference_type diff)
		{
			index += diff;
			return *this;
		}

		constexpr auto& operator-=(difference_type diff)
		{
			index -= diff;
			return *this;
		}

		constexpr auto operator==(const iter& other) const
		{
			assert(&stream == &other.stream);
			return index == other.index;
		}

		constexpr auto operator!=(const iter& other) const
		{
			return !(*this == other);
		}

		constexpr auto& operator*()
		{
			return stream[index];
		}

		constexpr auto& operator*() const
		{
			return stream[index];
		}

		constexpr auto* operator->()
		{
			return &stream[index];
		}

		constexpr auto* operator->() const
		{
			return &stream[index];
		}
	};

	using iterator = iter<TokenStream>;
	using const_iterator = iter<const TokenStream>;

public:
	TokenStream(size_t reserve = 4096)
	{
		tokens.reserve(reserve);
	}

	TokenStream(shared_ptr<const Source> source, size_t reserve = 4096) : source(source)
	{
		tokens.reserve(reserve);
	}

	auto operator[](size_t index) const->const Token&
	{
		return tokens[index];
	}
	
	constexpr auto all() const->const vector<Token>&
	{
		return tokens;
	}

	auto at(size_t index) const->const Token&
	{
		return tokens.at(index);
	}

	auto& back()
	{
		return tokens.back();
	}
	
	auto& back() const
	{
		return tokens.back();
	}

	constexpr auto begin()
	{
		return iterator{*this, 0};
	}

	constexpr auto begin() const
	{
		return const_iterator{*this, 0};
	}

	constexpr auto end()
	{
		return iterator{*this};
	}

	constexpr auto end() const
	{
		return const_iterator{*this};
	}

	auto empty() const noexcept
	{
		return tokens.empty();
	}

	auto size() const noexcept->size_t
	{
		return tokens.size();
	}

	auto& push(Token&& token)
	{
		return tokens.emplace_back(token);
	}

	template<typename... TArgs>
	auto& push(TArgs&&... args)
	{
		return tokens.emplace_back(forward<TArgs>(args)...);
	}

private:
	shared_ptr<const Source> source;
	vector<Token> tokens;
};

}