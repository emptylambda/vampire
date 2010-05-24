/**
 * @file SATClauseSharing.hpp
 * Defines class SATClauseSharing.
 */


#ifndef __SATClauseSharing__
#define __SATClauseSharing__

#include "Debug/Assertion.hpp"

#include "Lib/Set.hpp"
#include "Lib/VirtualIterator.hpp"

#include "SATClause.hpp"


namespace SAT {

using namespace Lib;

class SATClauseSharing
{
public:
  SATClause* insert(SATClause* c);
  void wipe();

  static SATClauseSharing* getInstance();

  SATClauseIterator content() { return pvi( ClauseSet::Iterator(_storage) ); }

private:
  struct Hasher {
    static unsigned hash(SATClause* t);
    static bool equals(SATClause* t1,SATClause* t2);
  };
  typedef Set<SATClause*, Hasher> ClauseSet;
  ClauseSet _storage;
};

};

#endif /* __SATClauseSharing__ */
