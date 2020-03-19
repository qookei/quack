#ifndef LIB_FUNCTION_TRAITS_H
#define LIB_FUNCTION_TRAITS_H

template <typename F>
struct function_traits;

template <typename R, typename ...Args>
struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

template <typename R, typename ...Args>
struct function_traits<R(Args...)> {
	using return_type = R;
};

template <typename R, typename C, typename ...Args>
struct function_traits<R (C::*)(Args...)> {
	using return_type = R;
};

template <typename F>
using return_type_t = typename function_traits<F>::return_type;

#endif //LIB_FUNCTION_TRAITS_H
