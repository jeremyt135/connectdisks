#pragma once

#include <type_traits>

namespace typeutil
{
	// Performs conversion from a scoped enum to its underlying type.
	// (from Scott Meyers Effective Modern C++, item 10)
	template<typename E>
	constexpr auto toUnderlyingType(E enumerator) noexcept
	{
		return static_cast<std::underlying_type_t<E>>(enumerator);
	}
	
	template<typename E, typename _ = void>
	struct toScopedEnum
	{
	};

	// Allows conversion to a scoped enum from its underlying type.
	template<typename E>
	struct toScopedEnum<
		E,
		std::enable_if_t<
			// is_convertible returns false if implicit conversion is impossible (which
			// is the case for scoped enums)
			std::is_enum<E>::value && !std::is_convertible<E, int>::value
		>>		
	{
		// wrapping in struct delays evaluation of underlying_type_t
		// until it's known that E is an enum class
		using UT = std::underlying_type_t<E>;

		// Performs conversion from an enum class's underlying type to the enum class type.
		template<typename T = UT>
		static constexpr E cast(T t) noexcept
		{
			static_assert(std::is_same<T, UT>::value, "error: argument to cast must be same as underlying type");
			return static_cast<E>(t);
		}
	};
}