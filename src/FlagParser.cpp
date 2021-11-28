#include "Headers.hpp"
#include "FlagParser.hpp"

void FlagParser::ParseFlags(int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
  {
    string flag = string(argv[i]);
    int fir_non_dash = 0;
    while (fir_non_dash < (int)flag.size() && flag[fir_non_dash] == '-')
    {
      fir_non_dash++;
    }
    if (fir_non_dash == (int)flag.size())
    {
      string message = "Just dashes is not a valid flag";
      throw message;
    }
    if (fir_non_dash == 0)
    {
      throw "All flags needs to be preceded by dashes";
    }
    size_t fir_equal = flag.find('=');
    if (fir_equal == string::npos)
    {
      flags[flag.substr(fir_non_dash)] = "";
      //throw "No = sign in flag";
    }
    else
    {
      string name = flag.substr(fir_non_dash, fir_equal - fir_non_dash);
      string val = flag.substr(fir_equal + 1);
      flags[name] = val;
    }
  }
}

string FlagParser::GetFlag(string s, bool required)
{
  if (flags.count(s))
  {
    asked.insert(s);
    return flags[s];
  }
  if (required)
  {
    throw "No such flag as " + s;
  }
  else
  {
    return "";
  }
}

bool FlagParser::Exist(string s)
{
  return flags.count(s);
}

void FlagParser::Close()
{
  for (auto p : flags)
  {
    if (asked.count(p.first) == 0)
    {
      throw "Not used " + p.first + " flag";
    }
  }
}