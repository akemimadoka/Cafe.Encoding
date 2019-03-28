#pragma once

#include "Encode.h"
#include <cstring>
#include <gsl/span>
#include <memory>

namespace Cafe::Encoding
{
	namespace Detail
	{
		static constexpr std::size_t DefaultSsoThresholdSize = 32;

		struct DefaultGrowPolicy
		{
			static constexpr float GrowFactor = 1.5f;

			// 结果保证大于 to
			[[nodiscard]] static constexpr std::size_t Grow(std::size_t from, std::size_t to)
			// [[expects: to > from]]
			// [[ensures result : result >= to]]
			{
				const auto newValue = static_cast<std::size_t>(from * GrowFactor);
				return std::max(newValue, to);
			}
		};

		// 管理字符串的存储，不保证 0 结尾
		// 仅提供会导致分配去配及提供必要信息的操作，其他操作由外部实现
		template <typename CharType, typename Allocator,
		          std::size_t SsoThresholdSize = DefaultSsoThresholdSize,
		          typename GrowPolicy = DefaultGrowPolicy>
		class StringStorage
		{
		public:
			constexpr StringStorage() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
			    : m_Allocator{}, m_Size{}, m_Capacity{ SsoThresholdSize }
			{
			}

			constexpr explicit StringStorage(Allocator const& allocator) noexcept
			    : m_Allocator{ allocator }, m_Size{}, m_Capacity{ SsoThresholdSize }
			{
			}

			template <std::ptrdiff_t Extent>
			constexpr StringStorage(gsl::span<const CharType, Extent> const& src,
			                        Allocator const& allocator = Allocator{})
			    : m_Allocator{ allocator }, m_Size{ src.size() }
			{
				if (m_Size > SsoThresholdSize)
				{
					m_DynamicStorage = std::allocator_traits<Allocator>::allocate(m_Allocator, m_Capacity);
					m_Capacity = m_Size;
					std::memcpy(m_DynamicStorage, src.data(), m_Size * sizeof(CharType));
				}
				else
				{
					m_Capacity = SsoThresholdSize;
					std::memcpy(m_SsoStorage, src.data(), m_Size * sizeof(CharType));
				}
			}

			template <typename OtherAllocator, std::size_t OtherSsoThresholdSize,
			          typename OtherGrowPolicy>
			constexpr StringStorage(StringStorage<CharType, OtherAllocator, OtherSsoThresholdSize,
			                                      OtherGrowPolicy> const& other)
			    : StringStorage(gsl::make_span(other.GetStorage(), other.GetSize()))
			{
			}

			// 若 Allocator 类型不同则直接 fallback 到复制，因为无法重用
			template <std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
			constexpr StringStorage(
			    StringStorage<CharType, Allocator, OtherSsoThresholdSize, OtherGrowPolicy>&&
			        other) noexcept(SsoThresholdSize >= OtherSsoThresholdSize)
			    : m_Allocator{ std::move(other.m_Allocator) }
			{
				if constexpr (SsoThresholdSize >= OtherSsoThresholdSize)
				{
					if (other.m_Capacity > SsoThresholdSize) // 允许直接重用动态存储
					{
						m_Size = std::exchange(other.m_Size, 0);
						m_Capacity = std::exchange(other.m_Capacity, OtherSsoThresholdSize);
						m_DynamicStorage = other.m_DynamicStorage;
						return;
					}
				}
				else
				{
					// 当 SsoThresholdSize < other.m_Size && other.m_Capacity <= OtherSsoThresholdSize
					// 之时，仅能复制
					if (other.m_Size > SsoThresholdSize)
					{
						if (other.m_Capacity <= OtherSsoThresholdSize)
						{
							new (static_cast<void*>(this))
							    StringStorage(gsl::make_span(other.GetStorage(), other.GetSize()), m_Allocator);
						}
						else
						{
							// other.m_Capacity > OtherSsoThresholdSize 时，由于都是动态存储所以允许重用
							m_Size = std::exchange(other.m_Size, 0);
							m_Capacity = std::exchange(other.m_Capacity, OtherSsoThresholdSize);
							m_DynamicStorage = other.m_DynamicStorage;
						}

						return;
					}
				}

				m_Size = other.m_Size;
				m_Capacity = other.m_Capacity;
				std::memcpy(m_SsoStorage, other.GetStorage(), m_Size);
			}

			~StringStorage()
			{
				if (IsDynamicAllocated())
				{
					std::allocator_traits<Allocator>::deallocate(m_Allocator, m_DynamicStorage, m_Capacity);
				}
			}

			template <typename OtherAllocator, std::size_t OtherSsoThresholdSize,
			          typename OtherGrowPolicy>
			constexpr StringStorage&
			operator=(StringStorage<CharType, OtherAllocator, OtherSsoThresholdSize,
			                        OtherGrowPolicy> const& other)
			{
				if constexpr (std::is_same_v<Allocator, OtherAllocator> &&
				              SsoThresholdSize == OtherSsoThresholdSize &&
				              std::is_same_v<GrowPolicy, OtherGrowPolicy>)
				{
					if (this == &other)
					{
						return *this;
					}
				}

				Assign(gsl::make_span(other.GetStorage(), other.GetSize()));

				return *this;
			}

			// 若 Allocator 不同则直接 fallback 到复制，因为无法重用
			template <std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
			constexpr StringStorage&
			operator=(StringStorage<CharType, Allocator, OtherSsoThresholdSize, OtherGrowPolicy>&&
			              other) noexcept(SsoThresholdSize >= OtherSsoThresholdSize)
			{
				if constexpr (SsoThresholdSize == OtherSsoThresholdSize &&
				              std::is_same_v<GrowPolicy, OtherGrowPolicy>)
				{
					if (this == &other)
					{
						return *this;
					}
				}

				if constexpr (SsoThresholdSize >= OtherSsoThresholdSize)
				{
					this->~StringStorage();
					new (static_cast<void*>(this)) StringStorage(std::move(other));
				}
				else
				{
					// 构建临时对象再 move，可能会造成稍高的损耗
					StringStorage tmpStorage(std::move(other));
					(*this) = std::move(tmpStorage);
				}

				return *this;
			}

			[[nodiscard]] constexpr std::size_t GetSize() const noexcept
			// [[ensures result : result <= GetCapacity()]]
			{
				return m_Size;
			}

			[[nodiscard]] constexpr std::size_t GetCapacity() const noexcept
			// [[ensures result : result >= SsoThresholdSize]]
			{
				return m_Capacity;
			}

			[[nodiscard]] constexpr CharType* GetStorage() noexcept
			// [[ensures result : result != nullptr]]
			{
				return const_cast<CharType*>(std::as_const(*this).GetStorage());
			}

			[[nodiscard]] constexpr const CharType* GetStorage() const noexcept
			// [[ensures result : result != nullptr]]
			{
				return IsDynamicAllocated() ? m_DynamicStorage : m_SsoStorage;
			}

			[[nodiscard]] constexpr bool IsDynamicAllocated() const noexcept
			{
				return m_Capacity > SsoThresholdSize;
			}

			constexpr void Reserve(std::size_t newCapacity)
			{
				// 由于 Capacity 最小值为 SsoThresholdSize，因此大于意味着必然需要动态存储
				if (newCapacity <= m_Capacity)
				{
					return;
				}

				const auto newStorage =
				    std::allocator_traits<Allocator>::allocate(m_Allocator, newCapacity);
				std::memcpy(newStorage, GetStorage(), m_Size);
				if (m_Capacity > SsoThresholdSize)
				{
					std::allocator_traits<Allocator>::deallocate(m_Allocator, m_DynamicStorage, m_Capacity);
				}
				m_DynamicStorage = newStorage;
				m_Capacity = newCapacity;
			}

			constexpr void Resize(std::size_t newSize, CharType value = CharType{})
			{
				if (newSize <= m_Capacity)
				{
					m_Size = newSize;
					return;
				}

				Reserve(newSize);
				std::uninitialized_fill(GetStorage() + m_Size, GetStorage() + newSize, value);
				m_Size = newSize;
			}

			/// @brief  用于在已确保 Reserve 足够内存的情况下不检查边界进行追加操作
			template <std::ptrdiff_t Extent>
			constexpr void UncheckedAppend(gsl::span<const CharType, Extent> const& src)
			{
				const auto srcSize = src.size();
				const auto newSize = m_Size + srcSize;
				std::memcpy(GetStorage() + m_Size, src.data(), srcSize);
				m_Size = newSize;
			}

			template <std::ptrdiff_t Extent>
			constexpr void Append(gsl::span<const CharType, Extent> const& src)
			{
				const auto srcSize = src.size();
				const auto newSize = m_Size + srcSize;
				if (newSize > m_Capacity)
				{
					Reserve(GrowPolicy::Grow(m_Capacity, newSize));
				}

				UncheckedAppend(src);
			}

			/// @brief  用于在已确保 Reserve 足够内存的情况下不检查边界进行赋值操作
			template <std::ptrdiff_t Extent>
			constexpr void UncheckedAssign(gsl::span<const CharType, Extent> const& src)
			{
				const auto newSize = src.size();
				std::memcpy(GetStorage(), src.data(), newSize * sizeof(CharType));
				m_Size = newSize;
			}

			template <std::ptrdiff_t Extent>
			constexpr void Assign(gsl::span<const CharType, Extent> const& src)
			{
				const auto newSize = src.size();
				Reserve(newSize);
				UncheckedAssign(src);
			}

			constexpr void ShrinkToFit()
			{
				if (IsDynamicAllocated())
				{
					if (m_Size < SsoThresholdSize)
					{
						const auto oldStorage = m_DynamicStorage;
						std::memcpy(m_SsoStorage, oldStorage, m_Size);
						std::allocator_traits<Allocator>::deallocate(m_Allocator, oldStorage, m_Capacity);
					}
					else
					{
						const auto newStorage = std::allocator_traits<Allocator>::allocate(m_Allocator, m_Size);
						std::memcpy(newStorage, m_DynamicStorage, m_Size);
						std::allocator_traits<Allocator>::deallocate(m_Allocator, m_DynamicStorage, m_Capacity);
						m_DynamicStorage = newStorage;
					}
				}

				m_Capacity = m_Size;
			}

		private:
#if __has_cpp_attribute(no_unique_address)
			[[no_unique_address]]
#endif
			Allocator m_Allocator;

			std::size_t m_Size;
			std::size_t m_Capacity;
			union
			{
				CharType m_SsoStorage[SsoThresholdSize];
				CharType* m_DynamicStorage;
			};
		};
	} // namespace Detail

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent = gsl::dynamic_extent>
	class StringView
	{
	public:
		static constexpr CodePage::CodePageType UsingCodePage = CodePageValue;

	private:
		using UsingCodePageTrait = CodePage::CodePageTrait<UsingCodePage>;
		using CharType = typename UsingCodePageTrait::CharType;

	public:
		using const_pointer = const CharType*;
		using pointer = const_pointer;

		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using value_type = CharType;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type const&;
		using const_reference = reference;

		constexpr StringView() noexcept : m_Span{}
		{
		}

		constexpr StringView(gsl::span<const CharType, Extent> const& span) noexcept : m_Span{ span }
		{
		}

		template <std::size_t N>
		constexpr StringView(const CharType (&array)[N]) noexcept : StringView(gsl::make_span(array))
		{
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return &*m_Span.begin();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return &*m_Span.end();
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return cbegin();
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return cend();
		}

		[[nodiscard]] constexpr size_type GetSize() const noexcept
		{
			return m_Span.size();
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return GetSize();
		}

		[[nodiscard]] constexpr bool IsEmpty() const noexcept
		{
			return !GetSize();
		}

		[[nodiscard]] constexpr StringView SubStr(size_type begin, size_type size) const noexcept
		{
			return StringView{ m_Span.subspan(begin, size) };
		}

		[[nodiscard]] constexpr gsl::span<const CharType, Extent> GetSpan() const noexcept
		{
			return m_Span;
		}

		[[nodiscard]] constexpr const_pointer GetData() const noexcept
		{
			return m_Span.data();
		}

		[[nodiscard]] constexpr reference operator[](difference_type index) const noexcept
		{
			return m_Span[index];
		}

		template <std::ptrdiff_t OtherExtent>
		[[nodiscard]] constexpr int
		Compare(StringView<CodePageValue, OtherExtent> const& other) const noexcept
		{
			const auto size = GetSize(), otherSize = other.GetSize();
			const auto minSize = std::min(size, otherSize);

			for (std::size_t i = 0; i < minSize; ++i)
			{
				const auto currentChar = m_Span[i], currentOtherChar = other[i];
				if (currentChar > currentOtherChar)
				{
					return 1;
				}
				else if (currentChar < currentOtherChar)
				{
					return -1;
				}
			}

			if (size > otherSize)
			{
				return 1;
			}
			else if (size < otherSize)
			{
				return -1;
			}

			return 0;
		}

	private:
		gsl::span<const CharType, Extent> m_Span;
	};

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator==(StringView<CodePageValue, Extent1> const& a,
	                                        StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return a.GetSize() == b.GetSize() && a.Compare(b) == 0;
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator!=(StringView<CodePageValue, Extent1> const& a,
	                                        StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return !(a == b);
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator<(StringView<CodePageValue, Extent1> const& a,
	                                       StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return a.Compare(b) < 0;
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator>(StringView<CodePageValue, Extent1> const& a,
	                                       StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return a.Compare(b) > 0;
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator>=(StringView<CodePageValue, Extent1> const& a,
	                                        StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return a.Compare(b) >= 0;
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent1, std::ptrdiff_t Extent2>
	[[nodiscard]] constexpr bool operator<=(StringView<CodePageValue, Extent1> const& a,
	                                        StringView<CodePageValue, Extent2> const& b) noexcept
	{
		return a.Compare(b) <= 0;
	}

	template <CodePage::CodePageType CodePageValue,
	          typename Allocator =
	              std::allocator<typename CodePage::CodePageTrait<CodePageValue>::CharType>,
	          std::size_t SsoThresholdSize = Detail::DefaultSsoThresholdSize,
	          typename GrowPolicy = Detail::DefaultGrowPolicy>
	class String
	{
	public:
		static constexpr CodePage::CodePageType UsingCodePage = CodePageValue;

	private:
		using UsingCodePageTrait = CodePage::CodePageTrait<UsingCodePage>;
		using CharType = typename UsingCodePageTrait::CharType;
		using UsingStorageType =
		    Detail::StringStorage<CharType, Allocator, SsoThresholdSize, GrowPolicy>;

	public:
		using pointer = typename std::allocator_traits<Allocator>::pointer;
		using const_pointer = typename std::allocator_traits<Allocator>::pointer;

		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using value_type = CharType;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using reference = value_type&;
		using const_reference = value_type const&;

		template <std::ptrdiff_t Extent = gsl::dynamic_extent>
		using ViewType = StringView<CodePageValue, Extent>;

		constexpr String() noexcept(std::is_nothrow_default_constructible_v<UsingStorageType>)
		{
		}

		constexpr explicit String(Allocator const& allocator) noexcept : m_Storage{ allocator }
		{
		}

		template <std::ptrdiff_t Extent>
		constexpr explicit String(gsl::span<const CharType, Extent> const& src,
		                          Allocator const& allocator = Allocator{})
		    : m_Storage{ src, allocator }
		{
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr String(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    : m_Storage{ other.m_Storage }
		{
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr String(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy>&&
		        other) noexcept(std::
		                            is_nothrow_constructible_v<
		                                Detail::StringStorage<CharType, Allocator, SsoThresholdSize,
		                                                      GrowPolicy>,
		                                Detail::StringStorage<CharType, OtherAllocator,
		                                                      OtherSsoThresholdSize,
		                                                      OtherGrowPolicy>&&>)
		    : m_Storage{ std::move(other.m_Storage) }
		{
		}

		template <std::ptrdiff_t Extent>
		constexpr String(StringView<CodePageValue, Extent> const& str,
		                 Allocator const& allocator = Allocator{})
		    : String{ str.GetSpan(), allocator }
		{
		}

		template <CodePage::CodePageType OtherCodePageValue, std::ptrdiff_t Extent,
		          std::enable_if_t<CodePageValue != OtherCodePageValue, int> = 0>
		constexpr explicit String(StringView<OtherCodePageValue, Extent> const& otherCodePageStr)
		{
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr String& operator=(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		{
			m_Storage = other.m_Storage;
			return *this;
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr String&
		operator=(String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy>&&
		              other) noexcept(std::
		                                  is_nothrow_assignable_v<
		                                      Detail::StringStorage<CharType, Allocator,
		                                                            SsoThresholdSize, GrowPolicy>,
		                                      Detail::StringStorage<CharType, OtherAllocator,
		                                                            OtherSsoThresholdSize,
		                                                            OtherGrowPolicy>&&>)
		{
			m_Storage = std::move(other.m_Storage);
			return *this;
		}

		template <std::ptrdiff_t Extent>
		constexpr void Assign(StringView<CodePageValue, Extent> const& str)
		{
			m_Storage.Assign(str.GetSpan());
		}

		template <std::ptrdiff_t Extent>
		constexpr String& operator=(StringView<CodePageValue, Extent> const& str)
		{
			Assign(str);
			return *this;
		}

		template <std::ptrdiff_t Extent>
		constexpr void Assign(gsl::span<const CharType, Extent> const& src)
		{
			m_Storage.Assign(src);
		}

		template <std::ptrdiff_t Extent>
		constexpr String& operator=(gsl::span<const CharType, Extent> const& src)
		{
			Assign(src);
			return *this;
		}

		template <std::ptrdiff_t Extent>
		constexpr void Append(gsl::span<const CharType, Extent> const& src)
		{
			m_Storage.Append(src);
		}

		template <std::ptrdiff_t Extent>
		constexpr void Append(ViewType<Extent> const& src)
		{
			m_Storage.Append(src.GetSpan());
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return m_Storage.GetStorage();
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return m_Storage.GetStorage() + m_Storage.GetSize();
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_Storage.GetStorage();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return m_Storage.GetStorage() + m_Storage.GetSize();
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return cbegin();
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return cend();
		}

		[[nodiscard]] constexpr reference operator[](difference_type index) noexcept
		{
			return m_Storage.GetStorage()[index];
		}

		[[nodiscard]] constexpr const_reference operator[](difference_type index) const noexcept
		{
			return m_Storage.GetStorage()[index];
		}

		[[nodiscard]] constexpr size_type GetSize() const noexcept
		{
			return m_Storage.GetSize();
		}

		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return GetSize();
		}

		[[nodiscard]] constexpr bool IsEmpty() const noexcept
		{
			return !GetSize();
		}

		[[nodiscard]] constexpr size_type GetCapacity() const noexcept
		{
			return m_Storage.GetCapacity();
		}

		[[nodiscard]] constexpr ViewType<> GetView() const noexcept
		{
			return ViewType<>{ gsl::make_span(m_Storage.GetStorage(), m_Storage.GetSize()) };
		}

		[[nodiscard]] operator ViewType<>() const noexcept
		{
			return GetView();
		}

	private:
		UsingStorageType m_Storage;
	};
} // namespace Cafe::Encoding
