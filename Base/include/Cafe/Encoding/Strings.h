#pragma once

#include "Encode.h"
#include <cassert>
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

			// 结果保证不小于 to
			[[nodiscard]] static constexpr std::size_t Grow(std::size_t from, std::size_t to)
			// [[expects: to > from]]
			// [[ensures result : result >= to]]
			{
				const auto newValue = static_cast<std::size_t>(from * GrowFactor);
				return std::max(newValue, to);
			}
		};

		// 管理字符串的存储，保证 0 结尾
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
			    : m_Allocator{ allocator }, m_Size{ static_cast<std::size_t>(src.size()) }
			{
				if (src.empty())
				{
					m_Capacity = SsoThresholdSize;
					return;
				}

				const auto isSrcNullTerminated = src[src.size() - 1] == CharType{};

				// 若 src 以 0 结尾则删去 1 位
				m_Size -= isSrcNullTerminated;
				const auto allocatingSize = m_Size + 1;

				if (allocatingSize > SsoThresholdSize)
				{
					m_Capacity = allocatingSize;
					m_DynamicStorage = std::allocator_traits<Allocator>::allocate(m_Allocator, m_Capacity);
					std::copy_n(src.data(), m_Size, m_DynamicStorage);
					m_DynamicStorage[allocatingSize - 1] = CharType{};
				}
				else
				{
					m_Capacity = SsoThresholdSize;
					std::copy_n(src.data(), m_Size, m_SsoStorage);
					m_SsoStorage[allocatingSize - 1] = CharType{};
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
			// StringStorage 的存储必定以 0 结尾，因此不需额外检查，直接复制即可
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
							m_Allocator->~Allocator();
							new (static_cast<void*>(this)) StringStorage(
							    gsl::make_span(other.GetStorage(), other.GetSize()), other.m_Allocator);
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
				std::copy_n(other.GetStorage(), m_Size + 1, m_SsoStorage);
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

			/// @brief  获得内容占据的大小，包含结尾的空字符
			[[nodiscard]] constexpr std::size_t GetSize() const noexcept
			// [[ensures result : result <= GetCapacity()]]
			{
				return m_Size + 1;
			}

			/// @brief  获得已分配的大小，包含结尾的空字符
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
				std::copy_n(GetStorage(), m_Size, newStorage);
				if (m_Capacity > SsoThresholdSize)
				{
					std::allocator_traits<Allocator>::deallocate(m_Allocator, m_DynamicStorage, m_Capacity);
				}
				m_DynamicStorage = newStorage;
				m_Capacity = newCapacity;
			}

			constexpr void Resize(std::size_t newSize, CharType value = CharType{})
			{
				assert(newSize > 0);
				--newSize; // 去除结尾空字符

				if (newSize > m_Capacity)
				{
					Reserve(newSize);
				}

				if (newSize > m_Size)
				{
					std::fill(GetStorage() + m_Size, GetStorage() + newSize, value);
				}

				GetStorage()[newSize] = CharType{};
				m_Size = newSize;
			}

			/// @remark  用于在已确保 Reserve 足够内存的情况下不检查边界进行追加操作
			template <std::ptrdiff_t Extent>
			constexpr void UncheckedAppend(gsl::span<const CharType, Extent> const& src)
			{
				const auto isSrcNullTerminated = src[src.size() - 1] == CharType{};
				const auto srcSize = src.size() - isSrcNullTerminated;
				const auto newSize = m_Size + srcSize;
				std::copy_n(src.data(), srcSize, GetStorage() + m_Size);
				GetStorage()[newSize] = CharType{};
				m_Size = newSize;
			}

			constexpr void UncheckedAppend(CharType value, std::size_t count = 1)
			{
				std::fill_n(GetStorage() + m_Size, count, value);
				m_Size += count;
				GetStorage()[m_Size] = CharType{};
			}

			template <std::ptrdiff_t Extent>
			constexpr void Append(gsl::span<const CharType, Extent> const& src)
			{
				if (src.empty())
				{
					return;
				}

				const auto srcSize = src.size();
				const auto isSrcNullTerminated = src[srcSize - 1] == CharType{};
				const auto newCapacity = m_Size + srcSize + !isSrcNullTerminated;
				if (newCapacity > m_Capacity)
				{
					Reserve(GrowPolicy::Grow(m_Capacity, newCapacity));
				}

				UncheckedAppend(src);
			}

			constexpr void Append(CharType value, std::size_t count = 1)
			{
				const auto newCapacity = m_Size + count + 1;
				if (newCapacity > m_Capacity)
				{
					Reserve(GrowPolicy::Grow(m_Capacity, newCapacity));
				}

				UncheckedAppend(value, count);
			}

			constexpr void Clear() noexcept
			{
				m_Size = 0;
				GetStorage()[0] = CharType{};
			}

			/// @brief  用于在已确保 Reserve 足够内存的情况下不检查边界进行赋值操作
			template <std::ptrdiff_t Extent>
			constexpr void UncheckedAssign(gsl::span<const CharType, Extent> const& src)
			{
				Clear();
				UncheckedAppend(src);
			}

			constexpr void UncheckedAssign(CharType value, std::size_t count = 1)
			{
				Clear();
				UncheckedAppend(value, count);
			}

			template <std::ptrdiff_t Extent>
			constexpr void Assign(gsl::span<const CharType, Extent> const& src)
			{
				Clear();
				Append(src);
			}

			constexpr void Assign(CharType value, std::size_t count = 1)
			{
				Clear();
				Append(value, count);
			}

			template <std::ptrdiff_t Extent>
			constexpr CharType* UncheckedInsert(const CharType* pos,
			                                    gsl::span<const CharType, Extent> const& src)
			{
				const auto isSrcNullTerminated = src[src.size() - 1] == CharType{};
				const auto srcSize = src.size() - isSrcNullTerminated;
				const auto newSize = m_Size + srcSize;

				const auto begin = GetStorage();
				const auto end = begin + m_Size;
				const auto moveDest = end + srcSize;

				const auto mutablePos = begin + (pos - begin);

				std::move_backward(mutablePos, end, moveDest);
				std::copy_n(src.data(), srcSize, mutablePos);
				begin[newSize] = CharType{};
				m_Size = newSize;

				return begin + newSize;
			}

			template <std::ptrdiff_t Extent>
			constexpr CharType* Insert(const CharType* pos, gsl::span<const CharType, Extent> const& src)
			{
				assert(GetStorage() <= pos && pos - GetStorage() <= m_Size);
				if (pos == GetStorage() + m_Size)
				{
					Append(src);
					return GetStorage() + m_Size;
				}

				const auto srcSize = src.size();
				const auto isSrcNullTerminated = src[srcSize - 1] == CharType{};
				const auto newCapacity = m_Size + srcSize + !isSrcNullTerminated;
				if (newCapacity > m_Capacity)
				{
					Reserve(GrowPolicy::Grow(m_Capacity, newCapacity));
				}

				return UncheckedInsert(pos, src);
			}

			constexpr CharType* Remove(const CharType* begin, std::size_t count) noexcept
			{
				const auto storage = GetStorage();

				assert(storage <= begin && begin - storage <= m_Size);
				assert(begin + count <= storage + m_Size);

				const auto mutableBegin = storage + (begin - storage);
				const auto mutableEnd = mutableBegin + count;

				std::move(mutableEnd, storage + m_Size, mutableBegin);
				m_Size -= mutableEnd - mutableBegin;
				storage[m_Size] = CharType{};

				return mutableBegin;
			}

			constexpr void RemoveBack(std::size_t count = 1) noexcept
			{
				m_Size -= std::min(count, m_Size);
				GetStorage()[m_Size] = CharType{};
			}

			constexpr void ShrinkToFit()
			{
				if (IsDynamicAllocated())
				{
					if (m_Size < SsoThresholdSize)
					{
						const auto oldStorage = m_DynamicStorage;
						std::copy_n(oldStorage, m_Size, m_SsoStorage);
						std::allocator_traits<Allocator>::deallocate(m_Allocator, oldStorage, m_Capacity);
						m_Capacity = SsoThresholdSize;
					}
					else
					{
						const auto newStorage = std::allocator_traits<Allocator>::allocate(m_Allocator, m_Size);
						std::copy_n(m_DynamicStorage, m_Size, newStorage);
						std::allocator_traits<Allocator>::deallocate(m_Allocator, m_DynamicStorage, m_Capacity);
						m_DynamicStorage = newStorage;
						m_Capacity = m_Size;
					}
				}
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

		template <std::size_t Size>
		class StringFindingCacheStorage
		{
		public:
			constexpr gsl::span<Core::Misc::UnsignedMinTypeToHold<Size>, Size> GetCacheContent() noexcept
			{
				return m_Cache;
			}

			constexpr gsl::span<const Core::Misc::UnsignedMinTypeToHold<Size>, Size>
			GetCacheContent() const noexcept
			{
				return m_Cache;
			}

		private:
			std::array<Core::Misc::UnsignedMinTypeToHold<Size>, Size> m_Cache;
		};

		template <>
		class StringFindingCacheStorage<std::numeric_limits<std::size_t>::max()>
		{
		public:
			StringFindingCacheStorage(std::size_t size)
			    : m_Cache{ std::make_unique<std::size_t[]>(size) }, m_CacheSize{ size }
			{
			}

			gsl::span<std::size_t> GetCacheContent() noexcept
			{
				return gsl::make_span(m_Cache.get(), m_CacheSize);
			}

			gsl::span<const std::size_t> GetCacheContent() const noexcept
			{
				return gsl::make_span(m_Cache.get(), m_CacheSize);
			}

		private:
			std::unique_ptr<std::size_t[]> m_Cache;
			std::size_t m_CacheSize;
		};
	} // namespace Detail

	template <CodePage::CodePageType CodePageValue,
	          typename Allocator =
	              std::allocator<typename CodePage::CodePageTrait<CodePageValue>::CharType>,
	          std::size_t SsoThresholdSize = Detail::DefaultSsoThresholdSize,
	          typename GrowPolicy = Detail::DefaultGrowPolicy>
	class String;

	/// @brief  判断类型是否为 Cafe::Encoding::String 的实例
	template <typename T>
	struct IsStringTrait : std::false_type
	{
	};

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy>
	struct IsStringTrait<String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy>>
	    : std::true_type
	{
	};

	template <typename T>
	constexpr bool IsString = IsStringTrait<T>::value;

	/// @brief  字符串视图
	/// @tparam CodePageValue   使用的编码
	/// @remark 不保证 0 结尾
	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent = gsl::dynamic_extent>
	class StringView
	{
	public:
		static constexpr CodePage::CodePageType UsingCodePage = CodePageValue;

	private:
		using UsingCodePageTrait = CodePage::CodePageTrait<UsingCodePage>;
		using CharType = typename UsingCodePageTrait::CharType;
		using SpanType = gsl::span<const CharType, Extent>;

	public:
		using const_pointer = const CharType*;
		using pointer = const_pointer;

		using iterator = pointer;
		using const_iterator = const_pointer;

		using value_type = CharType;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type const&;
		using const_reference = reference;

		constexpr StringView() noexcept : m_Span{}
		{
		}

		constexpr StringView(SpanType const& span) noexcept : m_Span{ span }
		{
		}

		template <std::size_t N>
		constexpr StringView(const CharType (&array)[N]) noexcept : StringView{ gsl::make_span(array) }
		{
		}

		constexpr StringView(StringView const&) noexcept = default;
		constexpr StringView(StringView&&) noexcept = default;
		constexpr StringView& operator=(StringView const&) noexcept = default;
		constexpr StringView& operator=(StringView&&) noexcept = default;

		template <std::ptrdiff_t OtherExtent>
		constexpr StringView(StringView<CodePageValue, OtherExtent> const& other) noexcept
		    : StringView(other.GetSpan())
		{
		}

		template <std::ptrdiff_t OtherExtent>
		constexpr StringView& operator=(StringView<CodePageValue, OtherExtent> const& other) noexcept
		{
			m_Span = other.GetSpan();
			return *this;
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_Span.data();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return m_Span.data() + m_Span.size();
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

		[[nodiscard]] constexpr StringView<CodePageValue>
		SubStr(size_type begin, size_type size = gsl::dynamic_extent) const noexcept
		{
			return StringView<CodePageValue>{ m_Span.subspan(begin, size) };
		}

		[[nodiscard]] constexpr SpanType GetSpan() const noexcept
		{
			return m_Span;
		}

		/// @brief  若以空字符结尾则返回去除结尾空字符的 span，否则等价于 GetSpan()
		[[nodiscard]] constexpr gsl::span<const CharType> GetTrimmedSpan() const noexcept
		{
			return m_Span.subspan(0, GetSize() - IsNullTerminated());
		}

		/// @brief  若以空字符结尾则返回去除结尾空字符的字符串视图，否则等价于直接复制自身
		[[nodiscard]] constexpr StringView<CodePageValue> Trim() const noexcept
		{
			return { GetTrimmedSpan() };
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
		[[nodiscard]] constexpr int Compare(StringView<CodePageValue, OtherExtent> const& other) const
		    noexcept
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

		template <std::ptrdiff_t OtherExtent>
		[[nodiscard]] constexpr bool
		BeginWith(StringView<CodePageValue, OtherExtent> const& other) const noexcept
		{
			if (other.GetSize() > GetSize())
			{
				return false;
			}

			return SubStr(0, other.GetSize()).Compare(other) == 0;
		}

		template <std::ptrdiff_t OtherExtent>
		[[nodiscard]] constexpr bool EndWith(StringView<CodePageValue, OtherExtent> const& other) const
		    noexcept
		{
			if (other.GetSize() > GetSize())
			{
				return false;
			}

			return SubStr(GetSize() - other.GetSize()).Compare(other) == 0;
		}

		[[nodiscard]] constexpr Detail::StringFindingCacheStorage<static_cast<std::size_t>(Extent)>
		CreateFindingCache() const noexcept(Extent != gsl::dynamic_extent)
		{
			auto result = [this]() constexpr
			                  ->Detail::StringFindingCacheStorage<static_cast<std::size_t>(Extent)>
			{
				if constexpr (Extent == gsl::dynamic_extent)
				{
					return { GetSize() };
				}
				else
				{
					return {};
				}
			}
			();

			const auto cacheContent = result.GetCacheContent();
			CreateFindingCacheAt(cacheContent);
			return result;
		}

		/// @brief  以 kmp 算法生成表来加速查找
		/// @remark 用于在已分配且保证长度足够的存储上创建缓存，可能用于特殊优化
		constexpr void
		CreateFindingCacheAt(gsl::span<std::conditional_t<Extent == gsl::dynamic_extent, std::size_t,
		                                                  Core::Misc::UnsignedMinTypeToHold<Extent>>,
		                               Extent> const& cacheStorage) const
		    noexcept(Extent != gsl::dynamic_extent)
		{
			if constexpr (Extent == gsl::dynamic_extent)
			{
				if (cacheStorage.size() < m_Span.size())
				{
					throw std::runtime_error("Cache storage is too small.");
				}
			}

			const auto size = m_Span.size();

			if (cacheStorage.empty())
			{
				return;
			}

			cacheStorage[0] = -1;

			if (size == 1)
			{
				return;
			}

			cacheStorage[1] = 0;

			std::size_t candidate = 0;
			for (std::size_t i = 2; i < size; ++i)
			{
				if (m_Span[i - 1] == m_Span[candidate])
				{
					cacheStorage[i] = ++candidate;
				}
				else
				{
					candidate = 0;
					cacheStorage[i] = 0;
				}
			}
		}

		static constexpr std::size_t Npos = std::size_t(-1);

		template <std::ptrdiff_t OtherExtent>
		[[nodiscard]] constexpr std::size_t
		Find(StringView<CodePageValue, OtherExtent> const& pattern,
		     gsl::span<std::conditional_t<OtherExtent == gsl::dynamic_extent, std::size_t,
		                                  Core::Misc::UnsignedMinTypeToHold<OtherExtent>>,
		               OtherExtent> const& findingCache) const noexcept
		{
			const auto size = m_Span.size();
			const auto patternSize = pattern.GetSize() - pattern.IsNullTerminated();

			std::size_t i = 0, j = 0;
			for (; i < size && j < patternSize;)
			{
				if (j == Npos || m_Span[i] == pattern.GetData()[j])
				{
					++i;
					++j;
				}
				else
				{
					j = findingCache[j];
				}
			}

			if (j >= patternSize)
			{
				return i - patternSize;
			}

			return Npos;
		}

		template <std::ptrdiff_t OtherExtent>
		[[nodiscard]] constexpr std::size_t
		Find(StringView<CodePageValue, OtherExtent> const& pattern) const
		    noexcept(OtherExtent != gsl::dynamic_extent)
		{
			return Find(pattern, pattern.CreateFindingCache().GetCacheContent());
		}

		template <typename Allocator =
		              std::allocator<typename CodePage::CodePageTrait<CodePageValue>::CharType>,
		          std::size_t SsoThresholdSize = Detail::DefaultSsoThresholdSize,
		          typename GrowPolicy = Detail::DefaultGrowPolicy>
		String<UsingCodePage, Allocator, SsoThresholdSize, GrowPolicy> ToString() const;

		template <typename Allocator>
		std::basic_string<CharType, std::char_traits<CharType>, Allocator>
		ToStdString(Allocator allocator = std::allocator<CharType>{}) const noexcept
		{
			return std::basic_string<CharType, std::char_traits<CharType>, Allocator>(
			    GetData(), GetSize(), std::move(allocator));
		}

		constexpr std::basic_string_view<CharType> ToStdStringView() const noexcept
		{
			return { GetData(), GetSize() };
		}

		/// @brief  判断字符串视图是否以 0 结尾
		/// @remark 空视图视为不以 0 结尾
		constexpr bool IsNullTerminated() const noexcept
		{
			if (m_Span.empty())
			{
				return false;
			}

			return m_Span[m_Span.size() - 1] == CharType{};
		}

	private:
		SpanType m_Span;
	};

	/// @brief  用于快速从 C 风格字符串构造字符串视图
	/// @code
	///     const auto str = AsView<CodePage::Utf8>(u8"abcd");
	/// @endcode
	template <CodePage::CodePageType CodePageValue, std::size_t N>
	constexpr StringView<CodePageValue, N>
	AsView(const typename CodePage::CodePageTrait<CodePageValue>::CharType (&arr)[N]) noexcept
	{
		return { arr };
	}

	/// @brief  用于快速从 C 风格字符串构造字符串视图
	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t N>
	constexpr StringView<CodePageValue, N>
	AsView(gsl::span<const typename CodePage::CodePageTrait<CodePageValue>::CharType, N> const&
	           span) noexcept
	{
		return { span };
	}

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

	/// @brief  判断类型是否为 Cafe::Encoding::StringView 的实例
	template <typename T>
	struct IsStringViewTrait : std::false_type
	{
	};

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent>
	struct IsStringViewTrait<StringView<CodePageValue, Extent>> : std::true_type
	{
	};

	template <typename T>
	constexpr bool IsStringView = IsStringViewTrait<T>::value;

	/// @brief  静态分配的字符串
	/// @remark 保证 0 结尾，空字符串大小为 1，若可行应使用 IsEmpty() 判断是否为空
	template <CodePage::CodePageType CodePageValue, std::size_t MaxSize>
	class StaticString
	{
	public:
		static constexpr CodePage::CodePageType UsingCodePage = CodePageValue;

	private:
		using UsingCodePageTrait = CodePage::CodePageTrait<CodePageValue>;
		using CharType = typename UsingCodePageTrait::CharType;

	public:
		using pointer = CharType*;
		using const_pointer = const CharType*;

		using iterator = pointer;
		using const_iterator = const_pointer;

		using value_type = CharType;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using reference = value_type&;
		using const_reference = value_type const&;

		constexpr StaticString() noexcept : m_Size{}, m_Storage{}
		{
		}

		constexpr StaticString(CharType ch, std::size_t count = 1) noexcept : m_Size{ count }
		{
			std::fill_n(m_Storage.data(), count, ch);
			m_Storage[count] = CharType{};
		}

		template <std::ptrdiff_t Extent,
		          std::enable_if_t<Extent == gsl::dynamic_extent || Extent <= MaxSize, int> = 0>
		constexpr StaticString(StringView<CodePageValue, Extent> const& str) noexcept
		{
			Assign(str);
		}

		[[nodiscard]] constexpr StringView<CodePageValue> GetView() const noexcept
		{
			return gsl::make_span(m_Storage.data(), GetSize());
		}

		template <std::ptrdiff_t Extent,
		          std::enable_if_t<Extent == gsl::dynamic_extent || Extent <= MaxSize, int> = 0>
		constexpr StaticString& Assign(StringView<CodePageValue, Extent> const& str) noexcept
		{
			if constexpr (Extent == gsl::dynamic_extent)
			{
				m_Size = std::min(str.GetSize() - str.IsNullTerminated(), MaxSize);
			}
			else
			{
				m_Size = Extent;
			}

			std::copy_n(str.GetData(), m_Size, m_Storage.data());
			m_Storage[m_Size] = CharType{};

			return *this;
		}

		[[nodiscard]] constexpr reference operator[](std::size_t i) noexcept
		{
			return m_Storage[i];
		}

		[[nodiscard]] constexpr const_reference operator[](std::size_t i) const noexcept
		{
			return m_Storage[i];
		}

		[[nodiscard]] constexpr pointer GetData() noexcept
		{
			return m_Storage.data();
		}

		[[nodiscard]] constexpr const_pointer GetData() const noexcept
		{
			return m_Storage.data();
		}

		[[nodiscard]] constexpr gsl::span<CharType> GetSpan() noexcept
		{
			return { m_Storage.data(), GetSize() };
		}

		[[nodiscard]] constexpr gsl::span<const CharType> GetSpan() const noexcept
		{
			return { m_Storage.data(), static_cast<std::ptrdiff_t>(GetSize()) };
		}

		[[nodiscard]] constexpr iterator begin() noexcept
		{
			return m_Storage.data();
		}

		[[nodiscard]] constexpr iterator end() noexcept
		{
			return m_Storage.data() + GetSize();
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept
		{
			return m_Storage.data();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept
		{
			return m_Storage.data() + GetSize();
		}

		[[nodiscard]] constexpr const_iterator begin() const noexcept
		{
			return cbegin();
		}

		[[nodiscard]] constexpr const_iterator end() const noexcept
		{
			return cend();
		}

		[[nodiscard]] constexpr std::size_t GetSize() const noexcept
		{
			return m_Size + 1;
		}

		[[nodiscard]] constexpr bool IsEmpty() const noexcept
		{
			return !m_Size;
		}

	private:
		std::array<CharType, MaxSize> m_Storage;
		std::size_t m_Size;
	};

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent,
	          std::enable_if_t<Extent != gsl::dynamic_extent, int> = 0>
	StaticString(StringView<CodePageValue, Extent> const& str)->StaticString<CodePageValue, Extent>;

	template <typename T>
	struct IsStaticStringTrait : std::false_type
	{
	};

	template <CodePage::CodePageType CodePageValue, std::size_t MaxSize>
	struct IsStaticStringTrait<StaticString<CodePageValue, MaxSize>> : std::true_type
	{
	};

	template <typename T>
	constexpr bool IsStaticString = IsStaticStringTrait<T>::value;

	/// @brief  字符串
	/// @remark 内部存储保证以 0 结尾，即 GetData()[GetSize()] 保证有效且为 0
	///         空字符串大小为 1，若可行应使用 IsEmpty() 判断是否为空
	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy>
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
		using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

		using iterator = pointer;
		using const_iterator = const_pointer;

		using value_type = CharType;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using reference = value_type&;
		using const_reference = value_type const&;

		using allocator_type = Allocator;

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

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr void Assign(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		{
			m_Storage = other.m_Storage;
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr void
		Assign(String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy>&&
		           other) noexcept(std::
		                               is_nothrow_assignable_v<
		                                   Detail::StringStorage<CharType, Allocator, SsoThresholdSize,
		                                                         GrowPolicy>,
		                                   Detail::StringStorage<CharType, OtherAllocator,
		                                                         OtherSsoThresholdSize,
		                                                         OtherGrowPolicy>&&>)
		{
			m_Storage = std::move(other.m_Storage);
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		constexpr String& operator=(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		{
			Assign(other);
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
			Assign(std::move(other));
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

		constexpr void Assign(CharType value, size_type count = 1)
		{
			m_Storage.Assign(value, count);
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

		constexpr void Append(CharType value, size_type count = 1)
		{
			m_Storage.Append(value, count);
		}

		template <std::ptrdiff_t Extent>
		constexpr iterator Insert(const_iterator pos, gsl::span<const CharType, Extent> const& src)
		{
			return m_Storage.Insert(pos, src);
		}

		template <std::ptrdiff_t Extent>
		constexpr iterator Insert(const_iterator pos, ViewType<Extent> const& src)
		{
			return m_Storage.Insert(pos, src.GetSpan());
		}

		constexpr iterator Remove(const_iterator pos, std::size_t count = 1) noexcept
		{
			return m_Storage.Remove(pos, count);
		}

		constexpr iterator Remove(const_iterator begin, const_iterator end) noexcept
		{
			assert(GetData() <= begin && begin <= end && end <= GetData() + GetSize() - 1);
			return m_Storage.Remove(begin, end - begin);
		}

		constexpr iterator RemoveBack(std::size_t count = 1) noexcept
		{
			return m_Storage.RemoveBack(count);
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

		[[nodiscard]] constexpr pointer GetData() noexcept
		{
			return m_Storage.GetStorage();
		}

		[[nodiscard]] constexpr const_pointer GetData() const noexcept
		{
			return m_Storage.GetStorage();
		}

		/// @brief  提供访问内部内容的操作符
		/// @remark 保证 index 处于 [0, GetSize()) 范围内时结果有效，且 index 为 GetSize() - 1
		///         时引用的字符必定为空字符，但此时不可写入空字符以外的字符，否则结果未定义
		[[nodiscard]] constexpr reference operator[](difference_type index) noexcept
		{
			return m_Storage.GetStorage()[index];
		}

		[[nodiscard]] constexpr const_reference operator[](difference_type index) const noexcept
		{
			return m_Storage.GetStorage()[index];
		}

		constexpr void Reserve(size_type newCapacity)
		{
			m_Storage.Reserve(newCapacity);
		}

		/// @brief  修改字符串长度
		/// @remark 若长度超过原长度，将使用 value 填充，否则将会截断
		///         新长度包含结尾空字符
		constexpr void Resize(size_type newSize, CharType value = CharType{})
		{
			m_Storage.Resize(newSize, value);
		}

		/// @brief  获得字符串长度，包含结尾的空字符
		/// @remark 由于结果必定包含空字符，因此最小值为 1
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
			const auto size = GetSize();
			assert(size > 0);
			return size == 1;
		}

		[[nodiscard]] constexpr size_type GetCapacity() const noexcept
		{
			return m_Storage.GetCapacity();
		}

		[[nodiscard]] constexpr ViewType<> GetView() const noexcept
		{
			return ViewType<>{ gsl::make_span(m_Storage.GetStorage(), m_Storage.GetSize()) };
		}

		[[nodiscard]] constexpr operator ViewType<>() const noexcept
		{
			return GetView();
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator==(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() == other;
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator!=(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() != other;
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator<(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() < other;
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator>(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() > other;
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator<=(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() < other;
		}

		template <std::ptrdiff_t Extent>
		[[nodiscard]] constexpr bool operator>=(StringView<CodePageValue, Extent> const& other) const
		    noexcept
		{
			return GetView() > other;
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator==(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() == other.GetView();
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator!=(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() != other.GetView();
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator<(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() < other.GetView();
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator>(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() > other.GetView();
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator<=(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() <= other.GetView();
		}

		template <typename OtherAllocator, std::size_t OtherSsoThresholdSize, typename OtherGrowPolicy>
		[[nodiscard]] constexpr bool operator>=(
		    String<CodePageValue, OtherAllocator, OtherSsoThresholdSize, OtherGrowPolicy> const& other)
		    const noexcept
		{
			return GetView() >= other.GetView();
		}

	private:
		UsingStorageType m_Storage;
	};

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator==(StringView<CodePageValue, Extent> const& a,
	           String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a == b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator!=(StringView<CodePageValue, Extent> const& a,
	           String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a != b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator>(StringView<CodePageValue, Extent> const& a,
	          String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a > b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator<(StringView<CodePageValue, Extent> const& a,
	          String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a < b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator>=(StringView<CodePageValue, Extent> const& a,
	           String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a >= b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, typename Allocator, std::size_t SsoThresholdSize,
	          typename GrowPolicy, std::ptrdiff_t Extent>
	[[nodiscard]] constexpr bool
	operator<=(StringView<CodePageValue, Extent> const& a,
	           String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy> const& b) noexcept
	{
		return a <= b.GetView();
	}

	template <CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent>
	template <typename Allocator, std::size_t SsoThresholdSize, typename GrowPolicy>
	String<StringView<CodePageValue, Extent>::UsingCodePage, Allocator, SsoThresholdSize, GrowPolicy>
	StringView<CodePageValue, Extent>::ToString() const
	{
		return String<StringView<CodePageValue, Extent>::UsingCodePage, Allocator, SsoThresholdSize,
		              GrowPolicy>{ *this };
	}

#if __cpp_nontype_template_parameter_class >= 201806L
	template <CodePage::CodePageType ToCodePage, auto Str>
	constexpr auto StaticEncode() noexcept
	{
		constexpr auto FromCodePage = decltype(Str)::UsingCodePage;
		StaticString<ToCodePage, CountEncodeSize<FromCodePage, ToCodePage>(Str.GetSpan())> result{};
		auto iter = result.GetData();
		Encoder<FromCodePage, ToCodePage>::EncodeAll(Str.GetSpan(), [&](auto const& result) {
			if constexpr (GetEncodingResultCode<decltype(result)> == EncodingResultCode::Accept)
			{
				if constexpr (CodePage::CodePageTrait<ToCodePage>::IsVariableWidth)
				{
					for (const auto item : result.Result)
					{
						*iter++ = item;
					}
				}
				else
				{
					*iter++ = result.Result;
				}
			}
		});
		return result;
	}
#endif

} // namespace Cafe::Encoding

namespace std
{
	template <Cafe::Encoding::CodePage::CodePageType CodePageValue, std::ptrdiff_t Extent>
	struct hash<Cafe::Encoding::StringView<CodePageValue, Extent>>
	{
		// 使用 BKDR hash 算法
		constexpr std::size_t
		operator()(Cafe::Encoding::StringView<CodePageValue, Extent> const& value) const noexcept
		{
			constexpr std::size_t seed = 131;
			std::size_t hash = 0;

			for (const auto item : value)
			{
				hash = hash * seed + item;
			}

			return hash;
		}
	};

	template <Cafe::Encoding::CodePage::CodePageType CodePageValue, std::size_t MaxSize>
	struct hash<Cafe::Encoding::StaticString<CodePageValue, MaxSize>>
	{
		using transparent_key_equal = std::equal_to<>;

		template <std::ptrdiff_t Extent>
		constexpr std::size_t
		operator()(Cafe::Encoding::StringView<CodePageValue, Extent> const& value) const noexcept
		{
			return hash<Cafe::Encoding::StringView<CodePageValue, Extent>>{}(value);
		}

		constexpr std::size_t
		operator()(Cafe::Encoding::StaticString<CodePageValue, MaxSize> const& value) const noexcept
		{
			return operator()(value.GetView());
		}
	};

	template <Cafe::Encoding::CodePage::CodePageType CodePageValue, typename Allocator,
	          std::size_t SsoThresholdSize, typename GrowPolicy>
	struct hash<Cafe::Encoding::String<CodePageValue, Allocator, SsoThresholdSize, GrowPolicy>>
	{
		using transparent_key_equal = std::equal_to<>;

		template <std::ptrdiff_t Extent>
		constexpr std::size_t
		operator()(Cafe::Encoding::StringView<CodePageValue, Extent> const& value) const noexcept
		{
			return hash<Cafe::Encoding::StringView<CodePageValue, Extent>>{}(value);
		}

		std::size_t operator()(Cafe::Encoding::String<CodePageValue, Allocator, SsoThresholdSize,
		                                              GrowPolicy> const& value) const noexcept
		{
			return operator()(value.GetView());
		}
	};
} // namespace std
