/**
 * @file EPRInlining.cpp
 * Implements class EPRInlining.
 */

#include "Debug/RuntimeStatistics.hpp"

#include "Lib/DHMap.hpp"
#include "Lib/Environment.hpp"
#include "Lib/MultiCounter.hpp"
#include "Lib/Timer.hpp"

#include "Kernel/Clause.hpp"
#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/Inference.hpp"
#include "Kernel/Signature.hpp"
#include "Kernel/SubformulaIterator.hpp"
#include "Kernel/SubstHelper.hpp"
#include "Kernel/TermIterators.hpp"
#include "Kernel/Unit.hpp"

#include "Flattening.hpp"
#include "Rectify.hpp"
#include "SimplifyFalseTrue.hpp"
#include "Statistics.hpp"
#include "VarManager.hpp"

#include "EPRInlining.hpp"

namespace Shell
{
/**
 * @warning The units must not contain formulas with predicate equivalences
 * (these are formulas which can be interpreted as predicate
 * definitions in two ways).
 */
void EPRInlining::scan(UnitList* units)
{
  CALL("EPRInlining::scan");

  UnitList::Iterator it(units);
  while(it.hasNext()) {
    Unit* u = it.next();
    if(u->isClause()) {
      continue;
    }
    FormulaUnit* fu = static_cast<FormulaUnit*>(u);
    if(hasDefinitionShape(fu)) {
      if(scanDefinition(fu)) {
	continue;
      }
    }
  }

  performClosure();
}

void EPRInlining::performClosure()
{
  CALL("EPRInlining::performClosure");

  while(_newNEPreds.isNonEmpty()) {
    unsigned p = _newNEPreds.pop_front();
    int polarity = _nonEprDefPolarities[p];
    DepMap::ValList::Iterator deps = _dependent.keyIterator(p);
    while(deps.hasNext()) {
      pair<FormulaUnit*,int> depRecord = deps.next(); //unit and the polarity with which p appears in it
      FormulaUnit* u = depRecord.first;
      int inheritedPolarity = polarity*depRecord.second*(_nonEprReversedPolarity[p] ? -1 : 1);
      addNEDef(u, _defPreds.get(u), inheritedPolarity);
    }
  }

  ZIArray<unsigned> dependencyCnt; // for each NE predicate contains number of NE predicates on which it depends
  MapToLIFO<unsigned,unsigned> dependencies;
  static Stack<unsigned> zeroPreds;
  static Stack<unsigned> deps;
  zeroPreds.reset();

  Stack<unsigned>::Iterator neIt(_nonEprPreds);
  while(neIt.hasNext()) {
    unsigned p = neIt.next();
    FormulaUnit* u = _nonEprDefs[p];
    ASS(u);
    deps.reset();
    u->collectPredicates(deps);
    makeUnique(deps);

    unsigned neDepCnt = 0;
    while(deps.isNonEmpty()) {
      unsigned dep = deps.pop();
      if(dep!=p && _nonEprDefs[dep]) {
	neDepCnt++;
	dependencies.pushToKey(dep, p);
      }
    }
    if(neDepCnt==0) {
      zeroPreds.push(p);
    }
    else {
      dependencyCnt[p] = neDepCnt;
    }
  }

  Stack<unsigned> activePreds;
  PDInliner defInliner(false, _trace);

  while(zeroPreds.isNonEmpty()) {
    unsigned p = zeroPreds.pop();
    FormulaUnit* u0 = _nonEprDefs[p];
    FormulaUnit* u = static_cast<FormulaUnit*>(defInliner.apply(u0));
    ASS(!u->isClause());
    ALWAYS(defInliner.tryGetDef(u));
    _nonEprDefs[p] = u;

    if(_trace) {
      cerr<<"Unit "<<u0->toString()<<" activated"<<endl;
    }
    _activeUnits.insert(u0);
    activePreds.push(p);

    MapToLIFO<unsigned,unsigned>::ValList::Iterator depIt=dependencies.keyIterator(p);
    while(depIt.hasNext()) {
      unsigned dep = depIt.next();
      ASS_G(dependencyCnt[dep],0);
      dependencyCnt[dep]--;
      if(dependencyCnt[dep]==0) {
	zeroPreds.push(dep);
      }
    }
  }
  //TODO: If there is a cycle, we stop inlining entirely. We might rather
  //want to remove one definition from the cycle and carry on.

  while(activePreds.isNonEmpty()) {
    unsigned p = activePreds.pop();
    FormulaUnit* u = _nonEprDefs[p];
    int polarity = _nonEprDefPolarities[p];
    Literal* lhs;
    Formula* rhs;
    splitDefinition(u, lhs, rhs);
    switch(polarity) {
    case 1:
      _inliner.addAsymetricDefinition(lhs, rhs, 0, rhs, u);
      break;
    case -1:
      _inliner.addAsymetricDefinition(lhs, 0, rhs, rhs, u);
      break;
    case 0:
      _inliner.addAsymetricDefinition(lhs, rhs, rhs, rhs, u);
      break;
    default:
      ASSERTION_VIOLATION;
    }
  }
}

bool EPRInlining::addNEDef(FormulaUnit* unit, unsigned pred, int polarity)
{
  CALL("EPRInlining::addNEDef");

  if(_nonEprDefs[pred]) {
    if(_nonEprDefs[pred]!=unit) {
      if(_trace) {
        cerr<<"Unit "<<unit->toString()<<" identified as EPR violating definition and ignored "
            "because there is already such definition for the predicate"<<endl;
      }
      //we already have a different non-epr definition, so we'll ignore this one
      return false;
    }
    int newPolarity = combinePolarities(_nonEprDefPolarities[pred],polarity);
    if(_nonEprDefPolarities[pred]==newPolarity) {
      return true;
    }
    _nonEprDefPolarities[pred] = newPolarity;
  }
  else {
    if(_trace) {
      cerr<<"Unit "<<unit->toString()<<" identified as EPR violating definition"<<endl;
    }
    _nonEprDefs[pred] = unit;
    _nonEprDefPolarities[pred] = polarity;
    _nonEprPreds.push(pred);
  }
  _newNEPreds.push_back(pred);
  return true;
}

bool EPRInlining::scanDefinition(FormulaUnit* unit)
{
  CALL("EPRInlining::scanDefinition");
  ASS(!PDInliner::isPredicateEquivalence(unit)); //predicate equivalences must be removed before applying this rule

  Literal* lhs;
  Formula* rhs;
  splitDefinition(unit, lhs, rhs);
  unsigned pred = lhs->functor();

  _defPreds.insert(unit, pred);

  int polarity;
  if(isNonEPRDef(lhs, rhs, polarity)) {
    if(!addNEDef(unit, pred, polarity)) {
      //we already have a non-epr definition, so we'll ignore this one
      return false;
    }
    _nonEprReversedPolarity[pred] = lhs->isNegative();
  }

  static Stack<pair<unsigned,int> > dependencies;
  rhs->collectPredicatesWithPolarity(dependencies);
  makeUnique(dependencies);
  while(dependencies.isNonEmpty()) {
    pair<unsigned,int> d = dependencies.pop();
    int polarity = d.second;
    _dependent.pushToKey(d.first, make_pair(unit, polarity));
  }

  return true;
}

Unit* EPRInlining::apply(Unit* unit)
{
  CALL("EPRInlining::apply");

  if(_activeUnits.find(unit)) {
    unsigned pred = _defPreds.get(unit);
    if(_nonEprDefPolarities[pred]==0) {
      //we are inlining all occurences, so we may delete the definition
      return 0;
    }
    else {
      //we are inlining just one polarity, the UPDR will take care of simplifying
      //this definition
      return unit;
    }
  }
  return _inliner.apply(unit);
}

void EPRInlining::splitDefinition(FormulaUnit* unit, Literal*& lhs, Formula*& rhs)
{
  CALL("EPRInlining::splitDefinition");

  Formula* f = unit->formula();
  if(f->connective()==FORALL) {
    f = f->qarg();
  }
  ASS_EQ(f->connective(),IFF);

  if(f->left()->connective()==LITERAL) {
    if(hasDefinitionShape(unit, f->left()->literal(), f->right())) {
      //we don't allow predicate equivalences here
      ASS(f->right()->connective()!=LITERAL || !hasDefinitionShape(unit, f->right()->literal(), f->left()));
      lhs = f->left()->literal();
      rhs = f->right();
      return;
    }
  }
  ASS_EQ(f->right()->connective(),LITERAL);
  ASS(hasDefinitionShape(unit, f->right()->literal(), f->left()));
  lhs = f->right()->literal();
  rhs = f->left();
}

/**
 * Perform local checks whether givan formula can be a definition.
 */
bool EPRInlining::hasDefinitionShape(FormulaUnit* unit)
{
  CALL("EPRInlining::hasDefinitionShape/1");

  Formula* f = unit->formula();
  if(f->connective()==FORALL) {
    f = f->qarg();
  }
  if(f->connective()!=IFF) {
    return false;
  }
  if(f->left()->connective()==LITERAL) {
    if(hasDefinitionShape(unit, f->left()->literal(), f->right())) {
      return true;
    }
  }
  if(f->right()->connective()==LITERAL) {
    return hasDefinitionShape(unit, f->right()->literal(), f->left());
  }
  return false;
}

/**
 * Perform local checks whether givan formula can be a definition.
 *
 * Check whether lhs is not an equality and its arguments are distinct
 * variables. Also check that there are no unbound variables in the body
 * that wouldn't occur in the lhs, and that the lhs predicate doesn't occur
 * in the body.
 */
bool EPRInlining::hasDefinitionShape(FormulaUnit* unit, Literal* lhs, Formula* rhs)
{
  CALL("EPRInlining::hasDefinitionShape/3");

  if(lhs->isEquality()) {
    return false;
  }

  unsigned defPred = lhs->functor();

  MultiCounter counter;
  for (const TermList* ts = lhs->args(); ts->isNonEmpty(); ts=ts->next()) {
    if (! ts->isVar()) {
      return false;
    }
    int w = ts->var();
    if (counter.get(w) != 0) { // more than one occurrence
      return false;
    }
    counter.inc(w);
  }

  static Stack<unsigned> bodyPredicates;
  bodyPredicates.reset();

  rhs->collectPredicates(bodyPredicates);
  if(bodyPredicates.find(defPred)) {
    return false;
  }

  {
    Formula::VarList* freeVars = rhs->freeVariables();
    bool extraFreeVars = false;
    while(freeVars) {
      unsigned v = Formula::VarList::pop(freeVars);
      if(!counter.get(v)) {
	extraFreeVars = true;
      }
    }
    if(extraFreeVars) {
      return false;
    }
  }

  return true;
}


/**
 * Return true if clausification of definition @c lhs<=>rhs will lead
 * to introduction of non-constant skolem functions
 */
bool EPRInlining::isNonEPRDef(Literal* lhs, Formula* rhs, int& polarity)
{
  CALL("EPRInlining::isNonEPRDef/3");

  if(lhs->arity()==0) {
    return false;
  }
  bool haveUniversal = false;
  bool haveExistential = false;
  SubformulaIterator sfit(rhs);
  while(sfit.hasNext()) {
    Formula* sf = sfit.next();
    if(sf->connective()==FORALL) {
      haveUniversal = true;
    }
    if(sf->connective()==EXISTS) {
      haveExistential = true;
    }
  }
  if(!haveUniversal && !haveExistential) {
    return false;
  }
  if(haveExistential) {
    USER_ERROR("Existential quantifiers not supported in EPRInlining");
  }
//  polarity = (!haveExistential) ? -1 : (!haveUniversal) ? 1 : 0;
  polarity = -1; //TODO: correctly determine which quentifiers become existential and which universal in ENNF
  return true;
}

int EPRInlining::combinePolarities(int p1, int p2)
{
  CALL("EPRInlining::combinePolarities");

  return (p1==p2) ? p1 : 0;
}

void EPRInlining::apply(UnitList*& units)
{
  CALL("EPRInlining::apply");

  {
    //remove predicate equivalences
    PDInliner pdi(false);
    pdi.apply(units, true);
  }

  scan(units);

  UnitList::DelIterator uit(units);
  while(uit.hasNext()) {
    Unit* u = uit.next();
    Unit* newUnit = apply(u);
    if(u==newUnit) {
      continue;
    }
    if(newUnit) {
      uit.replace(newUnit);
    }
    else {
      uit.del();
    }
  }
}

}
