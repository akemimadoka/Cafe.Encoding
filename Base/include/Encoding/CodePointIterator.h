#include "Encode.h"
#include <optional>

namespace Cafe::Encoding
{
	namespace Detail
	{
		template <bool>
		struct MaybeCurrentWidthBase
		{
		protected:
			mutable std::optional<std::size_t> m_CurrentWidth;
		};

		template <>
		struct MaybeCurrentWidthBase<false>
		{
		};
	} // namespace Detail

	template <
	    CodePageType CodePageValue, typename WrappedIterator,
	    std::enable_if_t<std::is_same_v<std::iterator_traits<WrappedIterator>::value_type,
	                                    typename CodePageTrait<CodePageValue>::CharType> &&
	                         std::is_same_v<std::iterator_traits<WrappedIterator>::iterator_category,
	                                        std::random_access_iterator_tag>,
	                     int> = 0>
	class CodePointIterator final
	    : Detail::MaybeCurrentWidthBase<CodePageTrait<CodePageValue>::IsVariableWidth>
	{
		using WrappedIteratorTrait = std::iterator_traits<WrappedIterator>;
		using UsingCodePageTrait = CodePageTrait<CodePageValue>;

	public:
		static constexpr auto UsingCodePage = CodePageValue;

		using iterator_category = std::random_access_iterator_tag;
		using value_type =
		    std::conditional_t<UsingCodePageTrait::IsVariableWidth,
		                       gsl::span<std::iterator_traits<WrappedIterator>::value_type>,
		                       std::iterator_traits<WrappedIterator>::value_type>;
		using difference_type = typename WrappedIteratorTrait::difference_type;

		using pointer = value_type*;
		using reference = value_type;

		template <typename = CodePointIterator,
		          std::enable_if_t<std::is_default_constructible_v<WrappedIterator>, int> = 0>
		CodePointIterator() = default;

		template <
		    typename WrappedIteratorType,
		    std::enable_if_t<
		        !std::is_same_v<std::remove_cvref_t<WrappedIteratorType>, CodePointIterator>, int> = 0>
		CodePointIterator(WrappedIteratorType&& wrappedIterator)
		    : m_WrappedIterator(std::forward<WrappedIteratorType>(wrappedIterator))
		{
		}

		reference operator*() const
		{
			if constexpr (UsingCodePageTrait::IsVariableWidth)
			{
				return gsl::make_span(std::addressof(*m_WrappedIterator), GetCurrentWidth());
			}
			else
			{
				return *m_WrappedIterator;
			}
		}

		CodePointIterator& operator++()
		{
			std::advance(m_WrappedIterator, GetCurrentWidth());
			return *this;
		}

		WrappedIterator GetBase() const
		{
			return m_WrappedIterator;
		}

	private:
		std::size_t GetCurrentWidth() const
		{
			std::size_t currentWidth;
			if (m_CurrentWidth.has_value())
			{
				currentWidth = m_CurrentWidth.value();
			}
			else
			{
				const auto width = UsingCodePageTrait::GetWidth(*m_WrappedIterator);
				if (width > 0)
				{
					currentWidth = width;
					m_CurrentWidth.emplace(currentWidth);
				}
				else
				{
					// TODO
					std::terminate();
				}
			}

			return currentWidth;
		}

		WrappedIterator m_WrappedIterator;
	};
} // namespace Cafe::Encoding
