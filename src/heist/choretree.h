// choretree.h ------------------------------------------------------------------------------------------------------
#pragma once

#include "../common_types.h"
#include "../silo/stash.h"
#include "../stalks/node.h"
#include "../stalks/work.h"
#include <concepts>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::heist {

class Maestro;
class Chore;

//-----------------------------------------------------------------------------------------------------------------
// Chore — concrete execution unit in a Heist chore tree.

class Chore
{
public:
    const char *m_DocStr{""};
    void      ( *m_Closure)( stalks::IWorker *){nullptr};

    constexpr Chore( void) noexcept = default;

    constexpr explicit Chore( void ( *f)( stalks::IWorker *)) noexcept
        : m_DocStr( ""),
          m_Closure( f)
    {
    }

    constexpr Chore( const char *docStr, void ( *f)( stalks::IWorker *)) noexcept
        : m_DocStr( docStr),
          m_Closure( f)
    {
    }

    static constexpr Chore New( void ( *f)( stalks::IWorker *)) noexcept
    {
        return Chore( f);
    }

    static constexpr Chore NewDoc( const char *docStr, void ( *f)( stalks::IWorker *)) noexcept
    {
        return Chore( docStr, f);
    }

    uint16_t Post( const Maestro *maestro, silo::Stash< uint16_t> &tails) const;
};

//-----------------------------------------------------------------------------------------------------------------
// PostChoreNode declaration for Chore and recursive BinNode

inline uint16_t PostChoreNode( const Chore &chore, const Maestro *maestro, silo::Stash< uint16_t> &tails)
{
    return chore.Post( maestro, tails);
}

template < typename L, typename R, typename Op>
uint16_t PostChoreNode( const stalks::BinNode< L, R, Op> &node, const Maestro *maestro, silo::Stash< uint16_t> &tails);

//-----------------------------------------------------------------------------------------------------------------
// Concept: CChoreNode — validates that a node can be posted into a Maestro/Atelier execution graph.

template < typename T>
concept CChoreNode = requires( const T &node, const Maestro *maestro, silo::Stash< uint16_t> &tails) {
    { PostChoreNode( node, maestro, tails) } -> std::convertible_to< uint16_t>;
};

//-----------------------------------------------------------------------------------------------------------------
// Node Tree Composition Operators (< for sequential dependency, | for parallel branching)

template < CChoreNode L, CChoreNode R>
constexpr auto operator<( L &&left, R &&right) noexcept
{
    return stalks::BinNode< std::decay_t< L>, std::decay_t< R>, stalks::BinOp>(
        std::forward< L>( left),
        std::forward< R>( right),
        stalks::BinOp::Less
    );
}

template < CChoreNode L, CChoreNode R>
constexpr auto operator|( L &&left, R &&right) noexcept
{
    return stalks::BinNode< std::decay_t< L>, std::decay_t< R>, stalks::BinOp>(
        std::forward< L>( left),
        std::forward< R>( right),
        stalks::BinOp::Bor
    );
}

} // namespace xeom::heist
