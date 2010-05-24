/**
 * @file SimplifyProver.hpp
 * Defines class SimplifyProver for working with files in the Simplify format
 *
 * @since 26/08/2009 Redmond
 */

#ifndef __SimplifyProver__
#define __SimplifyProver__

#include "Lib/List.hpp"
#include "Lib/Map.hpp"
#include "Lib/Set.hpp"
#include "Lib/Stack.hpp"
#include "Kernel/Unit.hpp"
#include "LispParser.hpp"

using namespace Lib;
using namespace Kernel;

namespace Shell {

/**
 * Class SimplifyProver for working with files in the Simplify format
 * @since 26/08/2009 Redmond
 */
class SimplifyProver
{
public:
  /** Type corresponding to the Simplify prover keywords. Identifiers are selected to reflect the names of the keywords */
  enum Keyword {
    /** means no keyword */
    K_NONE,
    K_DEFPRED,
    K_DEFPREDMAP,
    K_DEFUN,
    K_DEFINJ,
    K_DEFCONSTRUCTOR,
    K_DEFTUPLE,
    K_DEFARRAY,
    K_DEFWEAKARRAY,
    K_DEFCOTUPLE,
    K_DEFVALUE,
    K_DEFOP,
    K_DEFTYPE,
    K_SETPARAMETER,
    K_BG_PUSH,
    K_BG_POP,
    K_LEMMA,
    K_PROOF,
    K_CHECK,
    K_BUILTIN,
    K_TYPE,
    K_FORALL,
    K_EXISTS,
    K_LET,
    K_TERM,
    K_FORMULA,
    K_ORDER,
    K_LBLPOS,
    K_LBLNEG,
    K_LBL,
    K_PATS,
    K_NOPATS,
    K_MPAT,
    K_PROMOTE,
    K_AND,
    K_OR,
    K_IFF,
    K_IMPLIES,
    K_ITE,
    K_EXPLIES,
    K_NOT,
    K_TRUE,
    K_FALSE,
    K_EQ,
    K_NEQ,
    K_DISTINCT,
    K_PP,
    K_DUMP_CTX,
    K_DBG_VALID,
    K_DBG_INVALID,
    K_DBG_WAS_VALID,
    K_DBG_WAS_INVALID,
    K_ECHO,
    K_PROMPT_OFF,
    K_PROMPT_ON,
    K_EVALT,
    K_EVALF,
    K_EXIT,
    K_QID,
    K_SKOLEMID,
    K_WEIGHT,
    K_ASYNC,
    K_CANCEL,
    K_MODEL,
    K_STATS,
    K_SLEEP
  };

  /** Types of Simplify */
  enum Type {
    /** Int */
    BIT_INT = 0,
    /** bool */
    BIT_BOOL = 1,
    /** bool */
    BIT_BITVEC = 2,
    /** Just the number for the first type that is not built-in */
    OTHER = 3
  };

  SimplifyProver();
  ~SimplifyProver();
  typedef LispParser::Expression Expression;
  typedef Lib::List<Expression*> List;
  UnitList* units(const Expression*);

  /** information about a function or a predicate symbol */
  struct SymbolInfo {
    int arity;
    Type returnType;
    unsigned number;
    int argTypes[1];

    void* operator new(size_t,int arity);
    SymbolInfo(int arity);
    ~SymbolInfo();
  };

private:
  /** Various kinds of parsing instructions, numbers are defined explicitly for debugging only */
  enum Command {
    /** parse a formula */
    PARSE_FORMULA = 0,
    /** parse a term */
    PARSE_TERM = 1,
    /** build a term */
    BUILD_TERM = 2,
    /** build a conjunction or a disjunction */
    BUILD_JUNCTION_FORMULA = 3,
    /** build a quantified formula */
    BUILD_QUANTIFIED_FORMULA = 4,
    /** build an implication or equivalence */
    BUILD_BINARY_FORMULA = 5,
    /** build an atom */
    BUILD_ATOM = 6,
    /** build a negated formula */
    BUILD_NEGATED_FORMULA = 7,
    /** build an equality atom */
    BUILD_EQUALITY = 8,
    /** Build a DISTINCT formula */
    BUILD_DISTINCT = 9,
    /** do bindings introduced by LET */
    DO_LET = 10,
    /** undo bindings introduced by LET */
    UNDO_LET = 11,
    /** build a term defined by ITE */
    BUILD_ITE_TERM = 12,
    /** build formula P(x1,...,xn) <=> F from a LET definition for formulas */ 
    BUILD_LET_FORMULA = 13,
    /** build formula f(x1,...,xn) = t from a LET definition for terms */ 
    BUILD_LET_TERM = 14
  };

  /** Context in which a formula is parsed */
  enum Context {
    /** top-level (argument to BG_PUSH) */
    CN_TOP_LEVEL,
    /** as a subformula */
    CN_FORMULA,
    /** as an argument */
    CN_ARGUMENT
  };

  /** The list of units collected so far */
  UnitList* _units;
  /** maps type names to types */
  Map<string,Type> _types;
  /** maps symbols to their types */
  Map<string,SymbolInfo*> _symbolInfo;
  /** symbols having one or more boolean arguments */
  Set<string> _hasBooleanArgs;
  /** used for numbering new types */
  int _nextType;
  /** used for numbering new variables */
  int _nextVar;
  /** all formulas collected during parsing */
  Stack<Formula*> _formulas;
  /** current binding to variables */
  Map<string,IntList*> _variables;
  /** Instructions to be executed during parsing */
  Stack<Command> _commands;
  /** Already parsed expressions and arguments to instructions */
  Stack<const void*> _saved;
  /** integer arguments to instructions */
  Stack<int> _isaved;
  /** Already parsed and built formulas */
  Stack<void*> _built;
  /** Already parsed terms */
  Stack<TermList> _tsaved;
  /** Stored strings */
  Stack<string> _ssaved;
  /** special stack for storing numbers: we add axioms that all of them are different */
  Stack<TermList> _numbers;
  /** function symbol for constant 0, also used as boolean value false */
  TermList _zero;
  /** function symbol for constant 1, also used as boolean value true */
  TermList _one;
  /** formulas introduced by LET */
  Map<string,Lib::List<Formula*>*> _formulaLet;
  /** formulas introduced by LET */
  Map<string,Lib::List<TermList>*> _termLet;

  void parse(const Expression*);
  void parse();
  static Keyword keyword(const string& str);
  void formula(const Expression*);
  int bindVar(const string& varName);
  int isVar(const string& varName);
  void unbindVar(const string& var);
  void formulaError(const Expression* expr);
  void formulaError(const Expression* expr,const char* explanation);
  void termError(const Expression* expr);
  void error(const string& errMsg);
  void parseFormula();
  void parseJunctionFormula(const List*,const Expression*,Connective c,Context);
  void parseBinaryFormula(const List*,const Expression*,Connective c,Context);
  void parseNegatedFormula(const List*,const Expression*,Context);
  void parseQuantifiedFormula(const List* lst,const Expression*,Connective c,Context);
  void parseAtom(const List*,const Expression*,Context);
  void parseAtom(const Expression*,Context);
  void parseEquality(const List*,const Expression*,Context,bool polarity);
  void parseDistinct(const List*,const Expression*,Context);
  void parseLet(const List*,const Expression*,Context);
  void parseTerm();
  SymbolInfo* builtInPredicate(const string& str,int arity);
  SymbolInfo* builtInFunction(const string& str,int arity);
  SymbolInfo* getFunctionSymbolInfo(const string& name,int arity);
  SymbolInfo* addNumber(const string&);
  void defType(const List*,const Expression*);
  void defOp(const List*,const Expression*);
  void bgPush(const List*);
  void buildTerm();
  void buildAtom();
  void buildEquality();
  void doLet();
  void undoLet();
  void buildBinaryFormula();
  void buildJunctionFormula();
  void buildQuantifiedFormula();
  void buildNegatedFormula();
  void buildIfThenElseTerm();
  void buildDistinct();
  void processFormula(Formula*,Context);
  void addUnit(Unit*);
  void buildLetFormula();
  void buildLetTerm();
  void parseTrueFalse(bool,Context);
}; // class SimplifyProver

}

#endif

