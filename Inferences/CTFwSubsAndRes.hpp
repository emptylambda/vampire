/**
 * @file CTFwSubsAndRes.hpp
 * Defines class CTFwSubsAndRes.
 */

#ifndef __CTFwSubsAndRes__
#define __CTFwSubsAndRes__

#include "Forwards.hpp"

#include "InferenceEngine.hpp"

namespace Inferences {

using namespace Lib;
using namespace Kernel;
using namespace Indexing;

class CTFwSubsAndRes
: public ForwardSimplificationEngine
{
public:
  CTFwSubsAndRes(bool subsumptionResolution)
  : _subsumptionResolution(subsumptionResolution) {}
  
  void attach(SaturationAlgorithm* salg);
  void detach();
  void perform(Clause* cl, ForwardSimplificationPerformer* simplPerformer);
private:
  Clause* buildSResClause(Clause* cl, unsigned resolvedIndex, Clause* premise);

  bool _subsumptionResolution;
  ClauseSubsumptionIndex* _index;
};

}

#endif // __CTFwSubsAndRes__
