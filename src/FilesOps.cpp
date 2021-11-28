#include "FilesOps.hpp"

void InitIfstream(ifstream &in, const string &filename)
{
  in.open(filename.c_str(), ifstream::in);
  if (!in.good())
  {
    cerr << "File " << filename << " doesn't exist" << endl;
    exit(1);
  }
}

void InitOfstream(ofstream &out, const string &filename)
{
  out.open(filename, ofstream::out);
}