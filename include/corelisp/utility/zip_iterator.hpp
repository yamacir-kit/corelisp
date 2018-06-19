#ifndef INCLUDED_PURELISP_UTILITY_ZIP_ITERATOR_HPP
#define INCLUDED_PURELISP_UTILITY_ZIP_ITERATOR_HPP


#include <iterator>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>


namespace purelisp::utility
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
} // namespace purelisp::utility


#endif // INCLUDED_PURELISP_UTILITY_ZIP_ITERATOR_HPP

