#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

struct SimulationInfo {

  int  restartFlag      {0};
  int  algorithm        {2};
  int  system           {2};
  int  rngSeed          {-1};
  char inputFile[255]   {};

  // these are system specific:
  int  size             {3};
  char commandLine[256] {};

};


#endif
