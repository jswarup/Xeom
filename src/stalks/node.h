// node.h ----------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include <string>
#include <format>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::stalks {

//-----------------------------------------------------------------------------------------------------------------
// BinOp — binary operation types for abstract syntax and dependency graph composition.

enum class BinOp : uint64_t
{
    Sum  = 0,
    Prod = 1,
    Sub  = 2,
    Div  = 3,
    Pow  = 4,
    None = 5,
    Less = 6,
    Bor  = 7,
};

//-----------------------------------------------------------------------------------------------------------------
// BinNode — generic binary composite tree node.

template < typename L, typename R, typename Op = BinOp>
struct BinNode
{
    L  m_Left;
    R  m_Right;
    Op m_Op{BinOp::None};

    constexpr BinNode( L left, R right, Op op) noexcept
        : m_Left( std::move( left)),
          m_Right( std::move( right)),
          m_Op( op)
    {
    }

    constexpr bool operator==( const BinNode &) const = default;
};

//-----------------------------------------------------------------------------------------------------------------
// UniNode — generic unary tree node.

template < typename C, typename Op = BinOp>
struct UniNode
{
    C  m_Child;
    Op m_Op{BinOp::None};

    constexpr UniNode( C child, Op op) noexcept
        : m_Child( std::move( child)),
          m_Op( op)
    {
    }

    constexpr bool operator==( const UniNode &) const = default;
};

//-----------------------------------------------------------------------------------------------------------------
// Concept: CNode — marker concept for syntax tree elements.

template < typename T>
concept CNode = true;

} // namespace xeom::stalks
