#pragma once
#include <vector>
#include <memory>
#include <variant>

#if _DEBUG
#define NODEBUG_NOEXCEPT
#else
#define NODEBUG_NOEXCEPT noexcept
#endif

namespace CLARA {
namespace detail {
    template<typename T>
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        constexpr iterator() = default;
		constexpr iterator(const iterator<std::remove_const_t<T>>& iter) : m_ptr(iter.m_ptr), m_idx(iter.m_idx) {}
        constexpr explicit iterator(pointer ptr, size_t idx = 0) : m_ptr(ptr), m_idx(idx) {}

        [[nodiscard]] constexpr auto operator*() const->reference {
            return *operator->();
        }

        [[nodiscard]] constexpr auto operator->() const->pointer {
            return m_ptr + m_idx;
        }

        constexpr auto& operator++() {
            ++m_idx;
            return *this;
        }

        constexpr auto operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr auto& operator--() {
            --m_idx;
            return *this;
        }

        constexpr auto operator--(int) {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr auto& operator+=(const difference_type off) {
            m_idx += off;
            return *this;
        }

        constexpr auto& operator-=(const difference_type off) {
            m_idx -= off;
            return *this;
        }

        [[nodiscard]] constexpr auto operator+(const difference_type off) const {
            auto tmp = *this;
            return tmp += off;
        }

        [[nodiscard]] constexpr auto operator-(const difference_type off) const {
            auto tmp = *this;
            return tmp -= off;
        }

        [[nodiscard]] constexpr auto operator-(const iterator& other) const {
            return static_cast<difference_type>(m_idx - other.m_idx);
        }

        [[nodiscard]] constexpr auto& operator[](const difference_type off) const {
            return *(*this + off);
        }

        [[nodiscard]] constexpr auto operator==(const iterator& other) const {
            return m_idx == other.m_idx;
        }

        [[nodiscard]] constexpr auto operator!=(const iterator& other) const {
            return !(*this == other);
        }

        [[nodiscard]] constexpr auto operator<(const iterator& other) const {
            return m_idx < other.m_idx;
        }

        [[nodiscard]] constexpr auto operator>(const iterator& other) const {
            return other < *this;
        }

        [[nodiscard]] constexpr auto operator<=(const iterator& other) const {
            return !(other < *this);
        }

	public:
        pointer m_ptr;
        std::size_t m_idx;
    };

	template<typename T, typename = void>
	struct is_iterator {
		static constexpr bool value = false;
	};

	template<typename T>
	struct is_iterator<T, typename std::enable_if<!std::is_same<typename std::iterator_traits<T>::value_type, void>::value>::type> {
		static constexpr bool value = true;
	};

	template<typename T>
	constexpr bool is_iterator_v = is_iterator<T>::value;
}

template<typename T, std::size_t Capacity>
class static_vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = detail::iterator<T>;
    using const_iterator = detail::iterator<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	constexpr static_vector() = default;
	constexpr static_vector(static_vector&&) noexcept = default;
	constexpr static_vector(const static_vector&) = default;
	constexpr static_vector(size_type count, const value_type& value) {
		assign(count, value);
	}
	template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
	constexpr static_vector(InputIt first, InputIt last) {
		assign(first, last);
	}
	constexpr static_vector(std::initializer_list<value_type> init) : static_vector(init.begin(), init.end()) {}

    // operations

	constexpr auto& operator=(static_vector&& other) noexcept {
		swap(other);
		return *this;
	}

	constexpr auto& operator=(const static_vector& other) {
		assign(other.begin(), other.end());
		return *this;
	}

	constexpr auto& operator=(std::initializer_list<value_type> ilist) {
		assign(ilist.begin(), ilist.end());
		return *this;
	}

	constexpr auto assign(size_type count, const value_type& value) {
		m_size = count;
		std::fill_n(data(), count, value);
	}

	template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
	constexpr auto assign(InputIt first, InputIt last) {
		auto count = static_cast<size_type>(last - first);
		if (count < m_size) {
			std::copy_n(first, count, data());
			destruct_elements(count);
		}
		else if (count == m_size) {
			std::copy_n(first, count, data());
		}
		else if (count > m_size) {
			auto dest = std::copy_n(first, m_size, begin());
			insert(dest, first + m_size, last);
		}
		m_size = count;
	}

	constexpr auto assign(std::initializer_list<value_type> ilist) {
		assign(ilist.begin(), ilist.end());
	}

	constexpr auto fill(const value_type& value) {
        m_size = Capacity;
        std::fill_n(data(), Capacity, value);
    }

    // iterators

    [[nodiscard]] constexpr auto begin() noexcept {
        return iterator(data(), 0);
    }

    [[nodiscard]] constexpr auto begin() const noexcept {
        return const_iterator(data(), 0);
    }

    [[nodiscard]] constexpr auto end() noexcept {
        return iterator(data(), m_size);
    }

    [[nodiscard]] constexpr auto end() const noexcept {
        return const_iterator(data(), m_size);
    }

    [[nodiscard]] constexpr auto rbegin() noexcept {
        return reverse_iterator(end());
    }

    [[nodiscard]] constexpr auto rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    [[nodiscard]] constexpr auto rend() noexcept {
        return reverse_iterator(begin());
    }

    [[nodiscard]] constexpr auto rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] constexpr auto cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] constexpr auto cend() const noexcept {
        return end();
    }

    [[nodiscard]] constexpr auto crbegin() const noexcept {
        return rbegin();
    }

    [[nodiscard]] constexpr auto crend() const noexcept {
        return rend();
    }

    // element access

    [[nodiscard]] constexpr auto at(size_type pos)->reference {
        if (m_size <= pos) {
            throw std::out_of_range("invalid static_vector<T, N> subscript");
        }

        return data()[pos];
    }
    
    [[nodiscard]] constexpr auto at(size_type pos) const->const_reference {
        if (m_size <= pos) {
            throw std::out_of_range("invalid static_vector<T, N> subscript");
        }

        return data()[pos];
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) NODEBUG_NOEXCEPT->reference {
        #if _DEBUG
            if (pos >= m_size)
                throw std::out_of_range("static_vector subscript out of range");
        #endif

        return data()[pos];
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) const NODEBUG_NOEXCEPT->const_reference {
        #if _DEBUG
            if (pos >= m_size)
                throw std::out_of_range("static_vector subscript out of range");
        #endif

        return data()[pos];
    }

    [[nodiscard]] constexpr auto front() noexcept->reference {
        return *data();
    }

    [[nodiscard]] constexpr auto front() const noexcept->const_reference {
        return *data();
    }

    [[nodiscard]] constexpr auto back() noexcept->reference {
        return *(data() + (m_size - 1));
    }

    [[nodiscard]] constexpr auto back() const noexcept->const_reference {
        return *(data() + (m_size - 1));
    }

    [[nodiscard]] constexpr auto data() noexcept->pointer {
        return std::launder(reinterpret_cast<pointer>(&m_data));
    }

    [[nodiscard]] constexpr auto data() const noexcept->const_pointer {
        return std::launder(reinterpret_cast<const_pointer>(&m_data));
    }

    // capacity

    [[nodiscard]] constexpr auto capacity() const noexcept {
        return Capacity;
    }

    [[nodiscard]] constexpr auto size() const noexcept {
        return m_size;
    }

    [[nodiscard]] constexpr auto max_size() const noexcept {
        return capacity();
    }

    [[nodiscard]] constexpr auto empty() const noexcept {
        return size() == 0;
    }

    // modifiers

	constexpr auto insert(const_iterator pos, const value_type& val)->iterator {
        return emplace(pos, val);
    }

	constexpr auto insert(const_iterator pos, value_type&& val)->iterator {
        return emplace(pos, std::move(val));
    }

	constexpr auto insert(const_iterator pos, const size_type count, const value_type& val) {
		if (!count) return begin() + pos.m_idx;

		auto myfirst = begin();
		const auto off = static_cast<size_type>(pos - begin());
		const auto oldsize = size();

		std::fill(end(), end() + count, val);
		m_size = oldsize + count;

		if (pos != end()) {
			std::rotate(myfirst + off, myfirst + oldsize, end());
		}
		return myfirst + off;
    }

	template<typename Iter, typename = std::enable_if_t<detail::is_iterator_v<Iter>>>
	auto insert(const_iterator pos, Iter first, Iter last)->iterator {
		if (first == last) return begin() + pos.m_idx;
		
		auto myfirst = begin();
		const auto off = static_cast<size_type>(pos - myfirst);
		const auto oldsize = size();

		for (; first != last; ++first) {
			emplace_back(*first);
		}

		std::rotate(myfirst + off, myfirst + oldsize, end());
		return myfirst + off;
	}

	constexpr auto insert(const_iterator pos, std::initializer_list<value_type> list) {
		return insert(pos, list.begin(), list.end());
	}

    template<typename... Args>
	auto emplace(const_iterator pos, Args&&... args)->iterator {
		emplace_back(std::forward<Args>(args)...);
		auto first = begin() + pos.m_idx;
		std::rotate<iterator>(first, iterator(data(), size() - 1), end());
		return begin() + pos.m_idx;
    }

	constexpr auto erase(const_iterator pos)->iterator {
		auto first = begin() + pos.m_idx;
		std::rotate(first, first + 1, end());
		destruct_elements(m_size - 1);
		--m_size;
		return first + 1;
    }

	constexpr auto erase(const_iterator first, const_iterator last)->const_iterator {
		if (first != last) {
			const auto beg = begin();
			const auto myfirst = beg + first.m_idx;
			const auto mylast = beg + last.m_idx;
			const auto count = last.m_idx - first.m_idx;
			std::rotate(myfirst, mylast, end());
			destruct_elements(m_size - count);
			m_size -= count;
			return myfirst;
		}
        return last;
    }

    template<typename... Args>
	auto& emplace_back(Args&&... args) {
		auto ptr = new (&data()[m_size++]) value_type{std::forward<Args>(args)...};
        return *ptr;
    }

	constexpr auto resize(size_type count) {
		if (count == m_size) return;
		if (count < m_size) {
			erase(begin() + count, end());
		}
		else {
			insert(end(), count - m_size, value_type());
		}
	}

	constexpr auto resize(size_type count, const value_type& value) {
		if (count <= m_size) return resize(count);
		insert(end(), count - m_size, value);
	}

	auto push_back(const value_type& val) {
        emplace_back(val);
    }

	auto push_back(value_type&& _Val) {
        emplace_back(std::move(_Val));
    }

	constexpr auto pop_back() {
        #if _DEBUG
		if (empty())
            throw std::out_of_range("static_vector empty on pop_back");
        #endif

		destruct_elements(m_size - 1);
        --m_size;
    }

    constexpr auto clear() {
		destruct_elements(0);
        m_size = 0;
    }

	constexpr auto swap(static_vector& other) noexcept(std::is_nothrow_swappable_v<T>)->void {
		if (this == std::addressof(other)) return;
		auto ptr = data();
		auto otherPtr = other.data();
		for (auto swapSize = std::min(m_size, other.m_size); swapSize; --swapSize) {
			std::swap(*ptr++, *otherPtr++);
		}
		if (other.m_size > m_size) {
			for (auto left = m_size; left < other.m_size; ++left) {
				*ptr++ = std::move(*otherPtr++);
			}

			other.destruct_elements(m_size);
		}
		if (m_size > other.m_size) {
			for (auto left = other.m_size; left < m_size; ++left) {
				*otherPtr++ = std::move(*ptr++);
			}

			destruct_elements(other.m_size);
		}

		std::swap(m_size, other.m_size);
	}

private:
	constexpr auto destruct_elements(size_type from) {
		auto base = data();
		for (; from != m_size; ++from) {
			(base + from)->~value_type();
		}
	}

private:
	union {
		typename std::aligned_storage<sizeof(T), alignof(T)>::type m_data[Capacity];
		char donotuse_;
	};
    std::size_t m_size = 0;
};

// StaticSize = how much static/stack space to use before dynamically allocating
// MinDynamicSize = if a dynamic allocation has to be made, reserve space for at least this many elements
template<typename T, std::size_t StaticSize = 32, std::size_t MinDynamicSize = 64, typename Allocator = std::allocator<T>>
class auto_vector {
    using static_vec = static_vector<T, StaticSize>;
    using dynamic_vec = std::vector<T, Allocator>;
    constexpr static auto el_size = sizeof(T);
    
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = typename dynamic_vec::size_type;
    using difference_type = typename dynamic_vec::difference_type;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
    using iterator = typename dynamic_vec::iterator;
    using const_iterator = typename dynamic_vec::const_iterator;
    using reverse_iterator = std::reverse_iterator<typename dynamic_vec::iterator>;
    using const_reverse_iterator = std::reverse_iterator<typename dynamic_vec::const_iterator>;

public:
    auto_vector() : m_data(static_vec()) {}

    constexpr auto capacity() const {
        if (is_static()) {
            return StaticSize;
        }
        return get_dynamic_vec().capacity();
    }

    constexpr auto size() const {
        return is_static() ? get_static_vec().size() : get_dynamic_vec().size();
    }

    constexpr auto is_static() const {
        return std::holds_alternative<static_vec>(m_data);
    }
    
    constexpr auto data() {
        return is_static() ? get_static_vec().data() : get_dynamic_vec().data();
    }

    constexpr auto data() const {
        return is_static() ? get_static_vec().data() : get_dynamic_vec().data();
    }

    auto push_back(const value_type& val) {
        ensure_space_for_elements(1);
        if (is_static()) {
            return get_static_vec().push_back(val);
        }
        return get_dynamic_vec().push_back(val);
    }

    [[nodiscard]] constexpr auto operator[](size_type pos) const NODEBUG_NOEXCEPT->const_reference {
        #if _DEBUG
            if (pos >= size())
                throw std::out_of_range("static_vector subscript out of range");
        #endif

        return data()[pos];
    }

private:
    auto ensure_space_for_elements(size_type count) {
        if (is_static()) {
            auto new_size = size() + count;
            if (new_size > StaticSize) {
                auto& myvec = get_static_vec();
				auto vec = dynamic_vec();
                vec.reserve(new_size > MinDynamicSize ? new_size : MinDynamicSize);
                vec.assign(std::make_move_iterator(myvec.begin()), std::make_move_iterator(myvec.end()));
				m_data = std::move(vec);
            }
        }
    }

    [[nodiscard]] constexpr auto& get_static_vec() noexcept {
        return std::get<static_vec>(m_data);
    }

    [[nodiscard]] constexpr auto& get_static_vec() const noexcept {
        return std::get<static_vec>(m_data);
    }

    [[nodiscard]] constexpr auto& get_dynamic_vec() noexcept {
        return std::get<dynamic_vec>(m_data);
    }

    [[nodiscard]] constexpr auto& get_dynamic_vec() const noexcept {
        return std::get<dynamic_vec>(m_data);
    }

private:
    std::variant<static_vec, dynamic_vec> m_data;
};

}