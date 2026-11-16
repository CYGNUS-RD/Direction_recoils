// Impact point and direction of reconstructed clusters, single process.
// g++ -O3 -Isrc src/Analyzer.cxx direction.cxx -o direction `root-config --cflags --libs` -lSpectrum
// ./direction config/config.txt
#include <iostream>
#include "DirectionRun.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <config>\n";
    return 1;
  }
  Config c = ReadConfig(argv[1]);
  std::string out = OutputPath(c);
  Long64_t n = ProcessEntries(c, 0, Entries(c), out);
  WriteConfig(out, argv[1]);
  std::cout << n << " clusters -> " << out << "\n";
  return 0;
}
