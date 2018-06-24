#ifndef INCLUDED_CORELISP_UTILITY_ZIP_ITERATOR_HPP
#define INCLUDED_CORELISP_UTILITY_ZIP_ITERATOR_HPP


#include <iterator>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>


namespace utility
{
  template <typename... Ts>
  inline constexpr auto zip_begin(Ts&&... args)
  {
    using namespace boost;
    return make_zip_iterator(boost::make_tuple(std::begin(args)...));
  }

  template <typename... Ts>
  inline constexpr auto zip_end(Ts&&... args)
  {
    using namespace boost;
    return make_zip_iterator(boost::make_tuple(std::end(args)...));
  }
} // namespace utility


#endif // INCLUDED_CORELISP_UTILITY_ZIP_ITERATOR_HPP

