#ifndef KET_UTILITY_IS_NOTHROW_SWAPPABLE_HPP
# define KET_UTILITY_IS_NOTHROW_SWAPPABLE_HPP

# include <boost/config.hpp>

# include <utility>
# ifndef BOOST_NO_CXX11_HDR_TYPE_TRAITS
#   include <type_traits>
# else // BOOST_NO_CXX11_HDR_TYPE_TRAITS
#   include <boost/type_traits/declval.hpp>
#   include <boost/type_traits/integral_constant.hpp>
# endif // BOOST_NO_CXX11_HDR_TYPE_TRAITS

# ifndef BOOST_NO_CXX11_HDR_TYPE_TRAITS
#   define KET_declval std::declval
#   define KET_integral_constant std::integral_constant
# else // BOOST_NO_CXX11_HDR_TYPE_TRAITS
#   define KET_declval boost::declval
#   define KET_integral_constant boost::integral_constant
# endif // BOOST_NO_CXX11_HDR_TYPE_TRAITS


namespace ket
{
  namespace utility
  {
    namespace is_nothrow_swappable_impl
    {
      using std::swap;
      template <typename T>
      struct test
      {
        static bool const value
          = BOOST_NOEXCEPT_EXPR(swap(KET_declval<T&>(), KET_declval<T&>()));
      };
    } // namespace is_nothrow_swappable_impl

    template <typename T>
    struct is_nothrow_swappable
      : KET_integral_constant<bool, ::ket::utility::is_nothrow_swappable_impl::test<T>::value>
    { };
  } // namespace utility
} // namespace ket


# undef KET_integral_constant
# undef KET_declval

#endif
