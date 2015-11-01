#ifndef LODTALK_MPL_HPP
#define LODTALK_MPL_HPP

#include <stddef.h>

namespace Lodtalk
{
namespace MPL
{

struct error_base;

/**
 * Sequence arguments
 */
template<typename C>
struct front;

template<typename C>
struct back;

template<typename C>
struct pop_front;

template<typename C>
struct pop_back;

template<typename C, typename V>
struct push_front;

template<typename C, typename V>
struct push_back;

template<typename C>
struct size;

template<typename T>
struct TagArg {};

/**
 * Vector collection
 */
template<typename... Types>
struct vector {};

template<typename First, typename...Rest>
struct front<vector<First, Rest...> >
{
	typedef First type; 	
};

template<typename First, typename...Rest>
struct pop_front<vector<First, Rest...> >
{
	typedef vector<Rest...> type; 	
};

template<typename... Head, typename Last>
struct back<vector<Head..., Last> >
{
	typedef Last type; 	
};

template<typename... Head, typename Last>
struct pop_back<vector<Head..., Last> >
{
	typedef vector<Head...> type; 	
};

template<typename...Types, typename NewType>
struct push_back<vector<Types...>, NewType>
{
	typedef vector<Types..., NewType> type;
};

template<typename...Types, typename NewType>
struct push_front<vector<Types...>, NewType>
{
	typedef vector<NewType, Types...> type;
};

template<typename...Types>
struct size<vector<Types...>>
{
	static constexpr size_t value = sizeof...(Types);
};


// If
template<bool cond, typename A, typename B>
struct if_;

template<typename A, typename B>
struct if_<true, A, B>
{
	typedef A type;
};

template<typename A, typename B>
struct if_<false, A, B>
{
	typedef B type;
};

}

} // End of namespace Lodtalk
#endif //LODTALK_MPL_HPP