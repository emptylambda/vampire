/**
 * @file CommandLine.cpp
 * Implements class for processing command line.
 *
 * @since 14/11/2004 Manchester
 */

#include <stdlib.h>
#include <string.h>

#include "Debug/Assertion.hpp"
#include "Debug/Tracer.hpp"

#include "Lib/Exception.hpp"

#include "CommandLine.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

namespace Shell {

CommandLine::CommandLine (int argc, char* argv [])
  : _next(argv+1),
    _last(argv+argc)
{
  CALL ("CommandLine::CommandLine");
} // CommandLine::CommandLine

/**
 * Interpret command line.
 *
 * @since 08/08/2004 flight Manchester-Frankfurt (as Options::correct), check for KIF added
 * @since 14/11/2004 Manchester, made from Options::correct
 * @since 10/04/2005 Torrevieja, handling environment added
 * @since 14/04/2005 Manchester, handling special commands (bag) added
 * @since 06/05/2007 Manchester, simplified again
 */
void CommandLine::interpret (Options& options)
{
  CALL ("CommandLine::interpret");

  while (_next != _last) {
    ASS(_next < _last);
    const char* arg = *_next++;
    if (strcmp(arg, "--version")==0) {
      cout<<VERSION_STRING<<endl;
      exit(0);
    }
    if (arg[0] == '-') {
      if (_next == _last) {
	USER_ERROR((string)"no value specified for option " + arg);
      }
      if (arg[1] == '-') {
	options.set(arg+2,*_next);
      }
      else {
	options.setShort(arg+1,*_next);
      }
      _next++;
    }
    else {
      USER_ERROR((string)"option name expected, "+arg+" found");
    }
  }
  options.checkGlobalOptionConstraints();
} // CommandLine::interpret


} // namespace Shell
