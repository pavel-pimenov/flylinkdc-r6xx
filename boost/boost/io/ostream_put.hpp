/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_IO_OSTREAM_PUT_HPP
#define BOOST_IO_OSTREAM_PUT_HPP

#include <boost/config.hpp>
#include <iosfwd>
#include <cstddef>

namespace boost {
namespace io {
namespace detail {

template<class charT, class traits>
inline std::size_t
osp_put(std::basic_streambuf<charT, traits>& out, const charT* data,
    std::size_t size)
{
    return static_cast<std::size_t>(out.sputn(data, size));
}

template<class charT, class traits>
inline bool
osp_fill(std::basic_streambuf<charT, traits>& out, charT c, std::size_t size)
{
    charT fill[] = { c, c, c, c, c, c, c, c };
    enum {
        chunk = sizeof fill / sizeof(charT)
    };
    for (; size > chunk; size -= chunk) {
        if (boost::io::detail::osp_put(out, fill, chunk) != chunk) {
            return false;
        }
    }
    return boost::io::detail::osp_put(out, fill, size) == size;
}

template<class charT, class traits>
class osp_guard {
public:
    explicit osp_guard(std::basic_ostream<charT, traits>& os) BOOST_NOEXCEPT
        : os_(&os) { }

    ~osp_guard() BOOST_NOEXCEPT_IF(false) {
        if (os_) {
            os_->setstate(std::basic_ostream<charT, traits>::badbit);
        }
    }

    void release() BOOST_NOEXCEPT {
        os_ = 0;
    }

private:
    osp_guard(const osp_guard&);
    osp_guard& operator=(const osp_guard&);

    std::basic_ostream<charT, traits>* os_;
};

} /* detail */

template<class charT, class traits>
inline std::basic_ostream<charT, traits>&
ostream_put(std::basic_ostream<charT, traits>& os, const charT* data,
    std::size_t size)
{
    typedef std::basic_ostream<charT, traits> stream;
    detail::osp_guard<charT, traits> guard(os);
    typename stream::sentry entry(os);
    if (entry) {
        std::basic_streambuf<charT, traits>& out = *os.rdbuf();
        std::size_t width = static_cast<std::size_t>(os.width());
        if (width <= size) {
            if (detail::osp_put(out, data, size) != size) {
                return os;
            }
        } else if ((os.flags() & stream::adjustfield) == stream::left) {
            if (detail::osp_put(out, data, size) != size ||
                !detail::osp_fill(out, os.fill(), width - size)) {
                return os;
            }
        } else if (!detail::osp_fill(out, os.fill(), width - size) ||
            detail::osp_put(out, data, size) != size) {
            return os;
        }
        os.width(0);
    }
    guard.release();
    return os;
}

} /* io */
} /* boost */

#endif
