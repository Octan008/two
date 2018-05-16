//  Copyright (c) 2018 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#pragma once

/* std */
#include <type_traits>

namespace mud
{
	class Type;

	template<typename T>
	using unqual_type = typename std::remove_cv<typename std::remove_reference<T>::type>;

	template<typename T>
	using type_class = typename std::remove_pointer<typename unqual_type<T>::type>;

	inline void sink_type(Type&) {}

	template <typename T, typename = int>
	struct is_typed : std::false_type { };

	template <typename T>
	struct is_typed <T, decltype(sink_type(std::declval<T>().m_type), 0)> : std::true_type { };

	template <class T>
	inline typename std::enable_if<!is_typed<T>::value,
		Type&>::type typeof(const T& value) { UNUSED(value); return type<T>(); }

	template <class T>
	inline typename std::enable_if<is_typed<T>::value,
		Type&>::type typeof(const T& value) { return value.m_type; }

	template <class T>
	inline typename std::enable_if<!is_typed<T>::value,
		Type&>::type typeof(const T* value) { UNUSED(value); return type<T>(); }

	template <class T>
	inline typename std::enable_if<is_typed<T>::value,
		Type&>::type typeof(const T* value) { if(value) return value->m_type; return type<T>(); }


}
