/**
 * @file Otter.hpp
 * Defines class Otter.
 */


#ifndef __Otter__
#define __Otter__

#include "Forwards.hpp"

#include "SaturationAlgorithm.hpp"

namespace Saturation {

using namespace Kernel;

class Otter
: public SaturationAlgorithm
{
public:
  Otter(PassiveClauseContainer* passiveContainer, LiteralSelector* selector);

  ClauseContainer* getSimplificationClauseContainer();
  ClauseContainer* getGenerationClauseContainer();

protected:

  //overrides SaturationAlgorithm::onSOSClauseAdded
  void onSOSClauseAdded(Clause* cl);

  //overrides SaturationAlgorithm::onActiveRemoved
  void onActiveRemoved(Clause* cl);

  //overrides SaturationAlgorithm::onPassiveAdded
  void onPassiveAdded(Clause* cl);
  //overrides SaturationAlgorithm::onPassiveRemoved
  void onPassiveRemoved(Clause* cl);

  //overrides SaturationAlgorithm::onClauseRetained
  void onClauseRetained(Clause* cl);



  //overrides SaturationAlgorithm::handleUnsuccessfulActivation
  void handleUnsuccessfulActivation(Clause* c);

  /**
   * Dummy container for simplification indexes to subscribe
   * to its events.
   */
  struct FakeContainer
  : public ClauseContainer
  {
    /**
     * This method is called by @b saturate() method when a clause
     * makes it from unprocessed to passive container.
     */
    void add(Clause* c)
    { addedEvent.fire(c); }

    /**
     * This method is subscribed to remove events of passive
     * and active container, so it gets called automatically
     * when a clause is removed from one of them. (Clause
     * selection in passive container doesn't count as removal.)
     */
    void remove(Clause* c)
    { removedEvent.fire(c); }
  };

  FakeContainer _simplCont;
};

};

#endif /* __Otter__ */
