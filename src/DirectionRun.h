// Shared driver of direction.cxx (single process) and direction_mp.cxx (multiprocess):
// config reading, cluster cuts and the impact-point/direction loop over a range of entries.
#ifndef DIRECTIONRUN_H
#define DIRECTIONRUN_H

#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <TFile.h>
#include <TSystem.h>
#include <TH1.h>
#include <TNamed.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>
#include "Analyzer.h"

using Config = std::map<std::string, std::string>;

// One key=value per line; '#' starts a comment; keys without a value are ignored.
inline Config ReadConfig(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open config " + path);
  auto trim = [](std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r"));
    s.erase(s.find_last_not_of(" \t\r") + 1);
    return s;
  };
  Config c;
  for (std::string l; std::getline(in, l);) {
    l = l.substr(0, l.find('#'));
    size_t eq = l.find('=');
    if (eq == std::string::npos) continue;
    std::string k = trim(l.substr(0, eq)), v = trim(l.substr(eq + 1));
    if (!k.empty() && !v.empty()) c[k] = v;
  }
  return c;
}

inline double Get(const Config& c, const std::string& k, double def) {
  auto it = c.find(k);
  return it == c.end() ? def : std::stod(it->second);
}

inline std::string OutputPath(const Config& c) {
  std::string in = c.at("input_file");
  std::string dir = c.count("output_dir") ? c.at("output_dir") : ".";
  return dir + "/Direction_" + in.substr(in.find_last_of('/') + 1);
}

inline Long64_t Entries(const Config& c) {
  TFile f(c.at("input_file").c_str());
  auto* t = f.Get<TTree>("Events");
  if (!t) throw std::runtime_error("no Events tree in " + c.at("input_file"));
  return t->GetEntries();
}

// Optional cuts: <var>_min / <var>_max for run, scint, x_mean, y_mean;
// position_shape=ellipse adds an elliptical cut on (x_mean, y_mean).
inline bool PassCuts(const Config& c, double run, double scint, double x, double y) {
  const std::map<std::string, double> v = {{"run", run}, {"scint", scint}, {"x_mean", x}, {"y_mean", y}};
  for (const auto& kv : v) {
    if (c.count(kv.first + "_min") && kv.second < std::stod(c.at(kv.first + "_min"))) return false;
    if (c.count(kv.first + "_max") && kv.second > std::stod(c.at(kv.first + "_max"))) return false;
  }
  if (c.count("position_shape") && c.at("position_shape") == "ellipse") {
    double r = std::stod(c.at("ellipse_rotation_deg")) * M_PI / 180;   // major axis from +x
    double dx = x - std::stod(c.at("ellipse_center_x")), dy = y - std::stod(c.at("ellipse_center_y"));
    double u = (dx * std::cos(r) + dy * std::sin(r)) / std::stod(c.at("ellipse_radius_major"));
    double w = (-dx * std::sin(r) + dy * std::cos(r)) / std::stod(c.at("ellipse_radius_minor"));
    if (u * u + w * w > 1) return false;
  }
  return true;
}

// Impact point and direction of every accepted cluster in entries [first, last); tree track_info in outPath.
inline Long64_t ProcessEntries(const Config& c, Long64_t first, Long64_t last, const std::string& outPath) {
  const int npip = (int)Get(c, "NPIP", 100);
  const double wfac = Get(c, "wfactor", 1.5), thr = Get(c, "threshold", 0), noise = Get(c, "remove_noise_value", 0);
  const Long64_t pngEvery = (Long64_t)Get(c, "save_png_every", 0);
  TH1::AddDirectory(false);

  TFile fin(c.at("input_file").c_str());
  TTreeReader rd("Events", &fin);
  TTreeReaderValue<int> run(rd, "run"), event(rd, "event");
  TTreeReaderArray<float> idx(rd, "sc_redpixIdx"), integral(rd, "sc_integral"), xmean(rd, "sc_xmean"), ymean(rd, "sc_ymean");
  TTreeReaderArray<int> ix(rd, "redpix_ix"), iy(rd, "redpix_iy");
  TTreeReaderArray<float> iz(rd, "redpix_iz");

  TFile fout(outPath.c_str(), "RECREATE");
  TTree tree("track_info", "impact point and direction per cluster");
  int o_run, o_event, o_cluster;
  double o_int, o_xmean, o_ymean, o_xbar, o_ybar, o_xip, o_yip, o_dir, o_pca;
  tree.Branch("run", &o_run);
  tree.Branch("event", &o_event);
  tree.Branch("cluster", &o_cluster);              // index in the reconstruction's sc_ arrays
  tree.Branch("Integral", &o_int);
  tree.Branch("Xmean", &o_xmean);
  tree.Branch("Ymean", &o_ymean);
  tree.Branch("XBar", &o_xbar);                    // barycentre after the thresholds
  tree.Branch("YBar", &o_ybar);
  tree.Branch("X_ImpactPoint", &o_xip);
  tree.Branch("Y_ImpactPoint", &o_yip);
  tree.Branch("AngleDIR", &o_dir);                 // direction (rad), from the impact point
  tree.Branch("AnglePCA", &o_pca);                 // main axis (rad), no head-tail

  std::vector<int> X, Y, start;
  std::vector<float> Z;
  Long64_t n = 0;
  rd.SetEntriesRange(first, last);
  while (rd.Next()) {
    X.assign(ix.begin(), ix.end());
    Y.assign(iy.begin(), iy.end());
    Z.assign(iz.begin(), iz.end());
    start.assign(idx.begin(), idx.end());
    for (size_t i = 0; i < start.size(); ++i) {
      if (start[i] < 0) continue;                 // cluster without saved pixels
      int end = X.size();
      for (int s : start)
        if (s > start[i] && s < end) end = s;
      if (!PassCuts(c, *run, integral[i], xmean[i], ymean[i])) continue;

      std::string name = Form("r%d_e%d_c%zu", *run, *event, i);
      Analyzer a(name.c_str(), X.data(), Y.data(), Z.data(), start[i], end);
      if (a.GetIntegral() <= 0) continue;         // no positive pixel
      a.SetNPIP(npip);
      a.SetWScal(wfac);
      if (thr > 0) a.ApplyThr(thr);
      if (noise > 0) a.RemoveNoise(noise);
      o_pca = a.AngleLineMaxRMS();                // axis and barycentre of the thresholded track
      a.ImpactPoint((name + "_ip").c_str());
      a.ScaledTrack((name + "_sc").c_str());
      a.Direction();
      a.ImprCorrectAngle();

      o_run = *run; o_event = *event; o_cluster = i;
      o_int = integral[i]; o_xmean = xmean[i]; o_ymean = ymean[i];
      o_xbar = a.GetXbar(); o_ybar = a.GetYbar();
      o_xip = a.GetXIP(); o_yip = a.GetYIP();
      o_dir = a.GetDir();
      tree.Fill();
      if (pngEvery > 0 && n % pngEvery == 0) {
        a.BuildLineDirection();
        gSystem->mkdir("Tracks");                 // Analyzer::SavePicDir writes into ./Tracks/
        a.SavePicDir((name + ".png").c_str());
      }
      ++n;
    }
  }
  fout.cd();
  tree.Write();
  return n;
}

// Store the config text in the output file.
inline void WriteConfig(const std::string& outPath, const std::string& cfgPath) {
  std::ifstream in(cfgPath);
  std::stringstream s;
  s << in.rdbuf();
  TFile f(outPath.c_str(), "UPDATE");
  TNamed("config", s.str().c_str()).Write();
}

#endif
