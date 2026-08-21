// seg.h -------------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::silo {

template < typename TSzType = uint32_t>
struct Seg
{
    TSzType m_begin{0};
    TSzType m_size{0};

    static constexpr Seg New( TSzType begin, TSzType size) noexcept
    {
        return { begin, size };
    }

    constexpr TSzType Begin( void) const noexcept
    {
        return m_begin;
    }

    constexpr TSzType End( void) const noexcept
    {
        return m_begin + m_size;
    }

    constexpr TSzType Size( void) const noexcept
    {
        return m_size;
    }

    constexpr bool IsEmpty( void) const noexcept
    {
        return m_size == 0;
    }

    constexpr Seg RSnip( TSzType count) const noexcept
    {
        return { m_begin, ( m_size > count) ? ( m_size - count) : 0 };
    }

    constexpr Seg LSnip( TSzType count) const noexcept
    {
        TSzType snip = ( m_size > count) ? count : m_size;
        return { m_begin + snip, m_size - snip };
    }

template < typename F>
    constexpr bool Span( F &&f) const
    {
        const TSzType end = End();
        for ( TSzType i = m_begin; i < end; ++i) {
            if ( !f( i)) {
                return false;
            }
        }
        return true;
    }

template < typename F>
    constexpr void Traverse( F &&f) const
    {
        const TSzType end = End();
        for ( TSzType i = m_begin; i < end; ++i) {
            f( i);
        }
    }
};

using USeg = Seg< uint32_t>;

} // namespace xeom::silo

namespace xeom::common {

using silo::Seg;
using silo::USeg;

} // namespace xeom::common
