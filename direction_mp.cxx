// Impact point and direction of reconstructed clusters, n_workers processes on consecutive entry ranges.
// Same output as direction.cxx.
// g++ -O3 -Isrc src/Analyzer.cxx direction_mp.cxx -o direction_mp `root-config --cflags --libs` -lSpectrum
// ./direction_mp config/config.txt
#include <cstdio>
#include <iostream>
#include <numeric>
#include <ROOT/TProcessExecutor.hxx>
#include <TFileMerger.h>
#include "DirectionRun.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <config>\n";
    return 1;
  }
  Config c = ReadConfig(argv[1]);
  std::string out = OutputPath(c);
  Long64_t entries = Entries(c);
  int nw = std::max<Long64_t>(1, std::min<Long64_t>((Long64_t)Get(c, "n_workers", 4), entries));
  auto part = [&](int w) { return out + ".part" + std::to_string(w); };

  std::vector<int> ids(nw);
  std::iota(ids.begin(), ids.end(), 0);
  ROOT::TProcessExecutor pool(nw);
  auto counts = pool.Map([&](int w) { return ProcessEntries(c, entries * w / nw, entries * (w + 1) / nw, part(w)); }, ids);

  TFileMerger merger(false);
  merger.OutputFile(out.c_str(), "RECREATE");
  for (int w : ids) merger.AddFile(part(w).c_str());
  bool ok = merger.Merge();
  for (int w : ids) std::remove(part(w).c_str());
  if (!ok) {
    std::cerr << "merge failed\n";
    return 1;
  }
  WriteConfig(out, argv[1]);
  std::cout << std::accumulate(counts.begin(), counts.end(), 0LL) << " clusters -> " << out << "\n";
  return 0;
}
