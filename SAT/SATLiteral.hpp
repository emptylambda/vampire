/**
 * @file SATLiteral.hpp
 * Defines class SATLiteral.
 */


#ifndef __SATLiteral__
#define __SATLiteral__

#include <string>
#include <ostream>

#include "Debug/Assertion.hpp"

namespace SAT {

using namespace std;

class SATLiteral
{
public:
  inline SATLiteral() {}
  inline SATLiteral(unsigned content) :_content(content) {}
  inline SATLiteral(unsigned var, unsigned polarity) :_polarity(polarity), _var(var)
  { ASS_NEQ(var,0x7FFFFFFF); }


  inline void set(unsigned var, bool positive) {
    _var=var;
    _polarity=positive?1:0;
  }
  inline void setContent(unsigned content) {
    _content=content;
  }

  inline bool isPositive() const { return _polarity; }
  inline bool isNegative() const { return !_polarity; }
  inline unsigned var() const { return _var; }
  inline unsigned polarity() const { return _polarity; }
  inline unsigned content() const { return _content; }

  inline bool operator==(const SATLiteral& l) const
  { return _content==l._content; }
  inline bool operator!=(const SATLiteral& l) const
  { return _content!=l._content; }


  string toString() const;

  /**
   * Return a dummy literal that is not equal to any
   * literal present in any clause. Moreover, its var()
   * returns variable number bigger than any variable
   * present in regular literals.
   */
  inline static SATLiteral dummy() { return SATLiteral(0xFFFFFFFF); }

private:
  union {
    unsigned _content;
    struct {
      unsigned _polarity : 1;
      unsigned _var : 31;
    };
  };
};

};

std::ostream& operator<< (std::ostream& out, const SAT::SATLiteral& lit );

#endif /* __SATLiteral__ */
