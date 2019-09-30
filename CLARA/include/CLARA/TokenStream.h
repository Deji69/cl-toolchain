#pragma once
#include <CLARA/Common.h>
#include <CLARA/Source.h>
#include <CLARA/Token.h>

namespace CLARA::CLASM {

using TokenVec = vector<Token>;

class TokenStream {
public:
	template<typename T>
	struct iter : public std::random_access_iterator_tag {
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = Token;
		using difference_type   = size_t;
		using size_type         = size_t;
		using pointer           = value_type*;
		using reference         = value_type&;

		T* stream = nullptr;
		ptrdiff_t index = -1;

		constexpr iter() noexcept = default;

		constexpr iter(T& stream) noexcept :
			stream(&stream), index(-1)
		{ }

		constexpr iter(T& stream, ptrdiff_t index) noexcept :
			stream(&stream), index(index)
		{ }

		constexpr iter(const iter<std::remove_const_t<T>>& other) noexcept :
			stream(other.stream), index(other.index)
		{ }

		[[nodiscard]] constexpr auto ok() noexcept
		{
			return index < stream->size();
		}

		[[nodiscard]] constexpr operator bool() noexcept
		{
			return ok();
		}

		constexpr auto& operator=(const iter& other) noexcept
		{
			stream = other.stream;
			index = other.index;
			return *this;
		}

		constexpr auto& operator++() noexcept
		{
			if (index >= 0) {
				++index;
				if (index >= static_cast<ptrdiff_t>(stream->size()))
					index = -1;
			}
			return *this;
		}

		constexpr auto operator++(int) noexcept
		{
			auto tmp = *this;
			if (index >= 0) {
				++index;
				if (index >= stream->size())
					index = -1;
			}
			return tmp;
		}

		constexpr auto& operator--() noexcept
		{
			if (index > 0)
				--index;
			return *this;
		}

		constexpr auto operator--(int) noexcept
		{
			auto tmp = *this;
			if (index > 0)
				--index;
			return tmp;
		}

		constexpr auto& operator+=(difference_type diff) noexcept
		{
			index += diff;
			if (index >= stream->size())
				index = -1;
			return *this;
		}

		constexpr auto& operator-=(difference_type diff) noexcept
		{
			index -= diff;
			return *this;
		}

		[[nodiscard]] constexpr auto operator==(const iter& other) const noexcept
		{
			assert(stream == other.stream);
			return index == other.index;
		}

		[[nodiscard]] constexpr auto operator!=(const iter& other) const noexcept
		{
			return !(*this == other);
		}

		[[nodiscard]] constexpr auto& operator*() noexcept
		{
			return (*stream)[index];
		}

		[[nodiscard]] constexpr auto& operator*() const noexcept
		{
			return (*stream)[index];
		}

		[[nodiscard]] constexpr auto* operator->() noexcept
		{
			return &(*stream)[index];
		}

		[[nodiscard]] constexpr auto* operator->() const noexcept
		{
			return &(*stream)[index];
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

	TokenStream(initializer_list<tuple<TokenType, TokenAnnotation>> tokenInits)
	{
		tokens.reserve(tokenInits.size());

		for (auto& tokenInit : tokenInits) {
			std::apply(
				&TokenStream::push<TokenType, TokenAnnotation>,
				std::tuple_cat(std::make_tuple(this), tokenInit)
			);
		}
	}

	[[nodiscard]] auto operator[](size_t index) noexcept->Token&
	{
		return tokens[index];
	}

	[[nodiscard]] auto operator[](size_t index) const noexcept->const Token&
	{
		return tokens[index];
	}
	
	[[nodiscard]] constexpr auto all() const noexcept->const TokenVec&
	{
		return tokens;
	}

	[[nodiscard]] auto at(size_t index) const->const Token&
	{
		return tokens.at(index);
	}

	[[nodiscard]] auto& back() noexcept
	{
		return tokens.back();
	}
	
	[[nodiscard]] auto& back() const noexcept
	{
		return tokens.back();
	}

	[[nodiscard]] constexpr auto begin() noexcept
	{
		return iterator{*this, 0};
	}

	[[nodiscard]] constexpr auto begin() const noexcept
	{
		return const_iterator{*this, 0};
	}

	[[nodiscard]] constexpr auto end() noexcept
	{
		return iterator{*this};
	}

	[[nodiscard]] constexpr auto end() const noexcept
	{
		return const_iterator{*this};
	}

	[[nodiscard]] auto empty() const noexcept
	{
		return tokens.empty();
	}

	[[nodiscard]] auto size() const noexcept->size_t
	{
		return tokens.size();
	}

	template<typename... TArgs>
	auto push(TArgs&&... args)->iterator
	{
		tokens.emplace_back(forward<TArgs>(args)...);
		return iterator(*this, size() - 1);
	}

private:
	shared_ptr<const Source> source;
	TokenVec tokens;
};

}