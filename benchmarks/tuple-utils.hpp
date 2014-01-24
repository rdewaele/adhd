#pragma once

#include <tuple>
#include <iostream>

template< size_t ...i > struct IndexList {};

template< size_t ... > struct EnumBuilder;

// Increment cur until cur == end.
template< size_t end, size_t cur, size_t ...i >
struct EnumBuilder< end, cur, i... > 
    // Recurse, adding cur to i...
    : EnumBuilder< end, cur+1, i..., cur >
{
};

// cur == end; the list has been built.
template< size_t end, size_t ...i >
struct EnumBuilder< end, end, i... > {
    using type = IndexList< i... >;
};

template< size_t b, size_t e >
struct Enumerate {
    using type = typename EnumBuilder< e, b >::type;
};

template< class > struct IListFrom;

template< class ...X > 
struct IListFrom< std::tuple<X...> > {
    static constexpr size_t N = sizeof ...(X);
    using type = typename Enumerate< 0, N >::type;
};

// std::tuple_element<i,T> does not perfect forward.
template< size_t i, class T >
using Elem = decltype( std::get<i>(std::declval<T>()) );

template< size_t i > struct Get {
    template< class T >
    constexpr auto operator () ( T&& t ) 
        -> Elem< i, T >
    {
        return std::get<i>( std::forward<T>(t) );
    }
};

template< size_t i, class T, 
          class _T = typename std::decay<T>::type,
          size_t N = std::tuple_size<_T>::value - 1, // Highest index
          size_t j = N - i >
constexpr auto rget( T&& t ) 
    -> Elem< j, T >
{
    return std::get<j>( std::forward<T>(t) );
}

template< size_t i > struct RGet {
    template< class T >
    constexpr auto operator () ( T&& t ) 
        -> decltype( rget<i>( std::forward<T>(t) ) )
    {
        return rget<i>( std::forward<T>(t) );
    }
};

template< size_t i, class T, 
          class _T = typename std::decay<T>::type,
          size_t N = std::tuple_size<_T>::value,
          size_t j = i % N >
constexpr auto mod_get( T&& t ) 
    -> Elem< j, T >
{
    return std::get<j>( std::forward<T>(t) );
}

template< size_t ...i, class F, class T >
constexpr auto applyIndexList( IndexList<i...>, F f, const T& t )
    -> typename std::result_of< F( Elem<i,T>... ) >::type
{
    return f( std::get<i>(t)... );
}

// Safe to overload this way.
template< template<size_t> class Fi, size_t ...i, class F, class T >
constexpr auto applyIndexList( IndexList<i...>, F f, const T& t )
    -> typename std::result_of< F( 
        typename std::result_of< Fi<i>(const T&) >::type...
    ) >::type
{
    return f( Fi<i>()(t)... );
}

template< template<size_t> class Fi, class F, class T, class IL = typename IListFrom<T>::type >
constexpr auto applyTuple( F f, const T& t )
    -> decltype( applyIndexList<Fi>( IL(), f, t ) )
{
    return applyIndexList<Fi>( IL(), f, t );
}

template< class F, class T,
          class IL = typename IListFrom<T>::type >
constexpr auto applyTuple( F f, const T& t )
    -> decltype( applyIndexList( IL(), f, t ) )
{
    return applyIndexList( IL(), f, t );
}

// Because std::make_tuple can't be passed 
// to higher order functions.
constexpr struct MakeTuple {
    template< class ...X >
    constexpr std::tuple<X...> operator () ( X ...x ) {
        return std::tuple<X...>( std::move(x)... );
    }
} tuple{};

// Returns the initial elements. (All but the last.)
// init( {1,2,3} ) = {1,2}
template< class T,
          size_t N = std::tuple_size<T>::value, 
          class IL = typename Enumerate< 0, N-1 >::type >
constexpr auto init( const T& t )
    -> decltype( applyIndexList(IL(),tuple,t) )
{
    return applyIndexList( IL(), tuple, t );
}

// Returns a new tuple with every value from t except the first.
// tail( {1,2,3} ) = {2,3}
template< class T,
          size_t N = std::tuple_size<T>::value, 
          class IL = typename Enumerate< 1, N >::type >
constexpr auto tail( const T& t )
    -> decltype( applyIndexList(IL(),tuple,t) )
{
    return applyIndexList( IL(), tuple, t );
}

// Reconstruct t in reverse.
template< class T >
constexpr auto reverse( const T& t ) 
    -> decltype( applyTuple<RGet>(tuple,t) )
{
    return applyTuple< RGet >( tuple, t );
}

template< size_t i, size_t ...j, class F, class T >
void forEachIndex( IndexList<i,j...>, const F& f, const T& t ) {
    f( std::get<i>(t) );
    forEachIndex( IndexList<j...>(), f, t );
}

template< class F, class T >
void forEachIndex( IndexList<>, const F&, const T& ) {
}

template< class F, class T >
void forEach( const F& f, const T& t ) {
    constexpr size_t N = std::tuple_size<T>::value;
    using IL = typename Enumerate<0,N>::type;
    forEachIndex( IL(), f, t );
}

template< size_t i, size_t ...j, class F, class T >
bool whileTrueIndex( IndexList<i,j...>, const F& f, T& t ) {
	if ( f( std::get<i>(t) ) )
		return whileTrueIndex( IndexList<j...>(), f, t );
	else
		return false;
}

template< class F, class T >
bool whileTrueIndex( IndexList<>, const F&, T& ) {
	return true;
}

template< class F, class T >
bool whileTrue( const F& f, T& t ) {
	constexpr size_t N = std::tuple_size<T>::value;
	using IL = typename Enumerate<0,N>::type;
	return whileTrueIndex( IL(), f, t );
}

constexpr struct PrintItem {
    template< class X >
    void operator () ( const X& x ) const {
        std::cout << x << ' ';
    }
} printItem{};

constexpr struct PushBack {
    template< class ...X, class Y >
    constexpr auto operator () ( std::tuple<X...> t, Y y )
        -> std::tuple< X..., Y >
    {
        return std::tuple_cat( std::move(t), tuple(std::move(y)) );
    }
} pushBack{};

constexpr struct PushFront {
    template< class ...X, class Y >
    constexpr auto operator () ( std::tuple<X...> t, Y y )
        -> std::tuple< Y, X... >
    {
        return std::tuple_cat( tuple(std::move(y)), std::move(t) );
    }
} pushFront{};

constexpr auto head = Get<0>();
constexpr auto last = RGet<0>();

// Chain Left.
constexpr struct ChainL {
    template< class F, class X >
    constexpr X operator () ( const F&, X x ) {
        return x;
    }

    template< class F, class X, class Y, class ...Z >
    constexpr auto operator () ( const F& b, const X& x, const Y& y, const Z& ...z) 
        -> decltype( (*this)(b, b(x,y), z... ) )
    {
        return (*this)(b, b(x,y), z... );
    }
} chainl{};

// Fold Left.
constexpr struct FoldL {
    // Given f and {x,y,z}, returns f( f(x,y), z ).
    template< class F, class T >
    constexpr auto operator () ( const F& f, const T& t ) 
        -> decltype( applyTuple(chainl,pushFront(t,f)) )
    {
        return applyTuple( chainl, pushFront(t,f) );
    }
} foldl{};

// Fold Right.
constexpr struct FoldR {
    // Given f and {x,y,z}, returns f( f(z,y), x ).
    template< class F, class T >
    constexpr auto operator () ( const F& f, const T& t ) 
        -> decltype( foldl(f,reverse(t)) )
    {
        return foldl( f, reverse(t) );
    }
} foldr{};

//auto ten = foldl( std::plus<int>(), std::make_tuple(1,2,3,4) );

template< class ...X >
constexpr auto third_arg( X&& ...x )
    -> Elem< 2, std::tuple<X...> >
{
    return std::get<2>( std::forward_as_tuple(std::forward<X>(x)...) );
}


template< class F, class ...X >
struct TFunction {
    F f;
    std::tuple<X...> applied; // applied arguments.

    template< class ...Y >
    constexpr TFunction( F f, Y&& ...y )
        : f( std::move(f) ), applied( std::forward<Y>(y)... )
    {
    }

    template< class ...Y, class T = std::tuple<X...,Y...> >
    constexpr T add( Y&& ...y ) {
        return std::tuple_cat (
            applied,
            std::forward_as_tuple( std::forward<Y>(y)... )
        );
    }

    template< class ...Y >
    constexpr auto operator () ( Y&& ...y )
        -> typename std::result_of< F( X..., Y... ) >::type
    {
        return applyTuple( f, add(std::forward<Y>(y)...) );
    }
};

template< class F, class ...X, class R = TFunction<F,X...> >
R tfun( F f, X ...x ) {
    return R( std::move(f), std::move(x)... );
}

#include <cmath>

// Quadratic root.
constexpr struct QRoot {
    using result = std::pair<float,float>;

    result operator () ( float a, float b, float c ) {
        float root = std::sqrt( b*b - 4*a*c );
        float den  = 2 * a;
        return std::make_pair( (-b+root)/den, (-b-root)/den );
    }
} qroot{};

#if 0
std::ostream& operator << ( std::ostream& os, const QRoot::result r ) {
    return os << std::get<0>(r) << " or " << std::get<1>(r);
}

int main() {
    auto ab = std::make_tuple( 1, 3 );
    auto qroot_ab = [&] ( float c ) {
        return applyTuple( qroot, pushBack(ab,c) );
    };
    std::cout << "qroot(1,3,-4) = " << qroot_ab(-4) << std::endl;
    std::cout << "qroot(1,3,-5) = " << qroot_ab(-5) << std::endl;

    auto bc = std::make_tuple( 3, -4 );
    auto qroot_bc = [&] ( float a ) {
        return applyTuple( qroot, pushFront(bc,a) );
    };
    std::cout << "qroot(1,3,-4) = " << qroot_bc(1) << std::endl;
    std::cout << "qroot(1,3,-5) = " << qroot_bc(2) << std::endl;

    std::cout << "ten = " << ten << std::endl;
    std::cout << "third_arg(1,2,3,4) = " << third_arg(1,2,3,4) << std::endl;

    std::cout << "2 + 4 = " 
              << applyTuple( std::plus<int>(), std::make_tuple(2,4) ) 
              << std::endl;

    constexpr auto t = std::make_tuple( 1, 'a', "str" );
    std::cout << "t = ";
    forEach( printItem, t );
    std::cout << std::endl;

    std::cout << "head = " << head(t) << std::endl;
    std::cout << "last = " << last(t) << std::endl;

    std::cout << "reverse = ";
    forEach( printItem, reverse(t) );
    std::cout << std::endl;
}
#endif
