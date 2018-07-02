#ifndef INCLUDED_CORELISP_LISP_READER_HPP
#define INCLUDED_CORELISP_LISP_READER_HPP


namespace lisp
{
  // XXX なんか違う、作りたかったのはコレジャナイ
  template <typename CharType>
  class reader
    : public std::basic_string<CharType>
  {
  public:
    using char_type = CharType;

    enum class as // read as...
    {
      getchar, getline
    };

  public:
    // やりたかったのはコンパイルタイムタグディスパッチだがどうしてこうなった
    template <auto Dispatcher, typename... Ts>
    decltype(auto) operator()(Ts&&... args)
    {
      return read_<Dispatcher> {}(std::forward<Ts>(args)...);
    }

  protected:
    template <auto Dispatcher, typename = void>
    class read_;

    template <auto Dispatcher>
    class read_<Dispatcher, typename std::enable_if<Dispatcher == as::getchar>::type>
    {
    public:
      auto operator()(std::istream& is) const noexcept(noexcept(is.get()))
      {
        return static_cast<char_type>(is.get());
      }
    };

    template <typename Dispatcher>
    class read_<Dispatcher, typename std::enable_if<Dispatcher == >::type>
  } static read;
} // namespace lisp


#endif // INCLUDED_CORELISP_LISP_READER_HPP

