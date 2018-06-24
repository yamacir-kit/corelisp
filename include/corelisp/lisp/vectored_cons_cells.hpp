#ifndef INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP
#define INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP


#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <corelisp/lisp/tokenizer.hpp>
// #include <corelisp/utility/subrange_vector.hpp>
#include <corelisp/utility/zip_iterator.hpp>


// XXX フレンド関数群はプライベートメンバにアクセスしないのであれば外に居たほうが綺麗かも


namespace lisp
{
  // ベクタードコンセル
  // 気に入ってる名前だが英文的に正しく無さそうだし意味的にはフラットコンセルの方が良いかもしれぬ
  class vectored_cons_cells
    : public std::vector<vectored_cons_cells>
  {
  public: // attirbutes
    using value_type = std::string;
    value_type value; // TODO to be constant

    using scope_type = std::unordered_map<std::string, std::shared_ptr<vectored_cons_cells>>;
    scope_type closure;

  public: // constructors
    vectored_cons_cells(const value_type& value = "")
      : value {value}
    {}

    template <typename InputIterator
    , typename = typename std::enable_if<
                            std::is_nothrow_constructible<
                              decltype(value),
                              typename std::remove_reference<InputIterator>::type::value_type
                            >::value
                          >::type>
    vectored_cons_cells(InputIterator&& begin, InputIterator&& end)
    {
      if (std::distance(begin, end) != 0)
      {
        if (*begin != "(")
        {
          (*this).value = *begin;
        }
        else while (++begin != end && *begin != ")")
        {
          emplace_back(begin, end);
        }
      }
    }

    template <template <typename...> typename SequenceContainer>
    explicit vectored_cons_cells(const SequenceContainer<value_type>& tokens)
      : vectored_cons_cells {std::begin(tokens), std::end(tokens)}
    {}

    template <template <typename...> typename SequenceContainer>
    vectored_cons_cells(SequenceContainer<value_type>&& tokens)
      : vectored_cons_cells {std::begin(tokens), std::end(tokens)}
    {}

  public: // accesses
    bool atom() const noexcept // is_atomの方が良いだろうか
    {
      return std::empty(*this);
    }

    friend auto atom(const vectored_cons_cells& e) noexcept
    {
      return e.atom();
    }

  public: // operators
    // TODO
    // 真偽値型への暗黙キャスト演算子オーバーロードがあると面白いかも
    // オペレータnull?を内部で呼ぶ感じで

    bool operator!=(const vectored_cons_cells& rhs) const noexcept
    {
      if (&(*this) == &rhs) // アドレスが等しい場合は即座に比較終了
      {
        return false;
      }

      if (std::size(*this) != std::size(rhs) or (*this).value != rhs.value)
      {
        return true;
      }

      for(auto iter {utility::zip_begin(*this, rhs)}; iter != utility::zip_end(*this, rhs); ++iter)
      {
        if (boost::get<0>(*iter) != boost::get<1>(*iter))
        {
          return true;
        }
      }

      return false;
    }

    bool operator==(const vectored_cons_cells& rhs) const noexcept
    {
      return !(*this != rhs);
    }

    friend auto operator<<(std::ostream& os, const vectored_cons_cells& e)
      -> std::ostream&
    {
      if (not e.atom())
      {
        os <<  '(';
        for (const auto& each : e)
        {
          os << each << (&each != &e.back() ? " " : "");
        }
        return os << ')';
      }
      else return os << e.value;
    }
  } static true_ {"true"}, false_;
} // namespace lisp


#endif // INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP

