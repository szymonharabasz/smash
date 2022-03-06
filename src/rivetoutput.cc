/*
 *
 *    Copyright (c) 2021 Christian Holm Christensen
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "smash/rivetoutput.h"

#include <Rivet/Rivet.hh>
#include <Rivet/Tools/Logging.hh>

#include "smash/logging.h"

namespace smash {
/*!\Userguide
 * \page rivet_output_user_guide_ Rivet Output User Guide
 *
 * SMASH Rivet output interface directly to Rivet analyses on the events
 * generated by SMASH.  That is, instead of writing the events to
 * disk (or pipe) this output module generates HepMC (see also \ref
 * output_hepmc_) events in-memory and pass them on
 * directly to Rivet to avoid the costly encoding and decoding of HepMC3
 * events to and from disk (or pipe). This provides a speed up (at least
 * \f$\times 10\f$) compared to intermediate files.
 *
 * The results of the analyses are written to YODA files.
 *
 * \section rivet_output_user_guide_format_ Internal event formats
 *
 * Internally, this output module generates HepMC events.  These can
 * be made in two ways, selected by the formats \key YODA and \key
 * YODA-full
 *
 * - \key YODA In this format only initial (beam) and final state
 *   particles are stored in the event structure and there is only
 *   one interaction point. Note, the incoming nucleons are combined
 *   into single nuclei.
 *
 * - \key YODA-full In this format, the whole event is stored
 *   (including intermediate states). Note, the incoming nucleons
 *   are combined into single nuclei, only to be split at the main
 *   interaction point. This allows tracking of the individual
 *   nucleons.
 *
 * Only one format can be chosen. SMASH will use the first format recognized as
 * valid, ignoring the rest.
 * Please, note that choosing YODA or YODA-full determines the kind of
 * information available to the analysis, but the content of the final YODA
 * files depends on the analysis itself.
 * Depending on what it does, the analysis might work fine with both formats,
 * nevertheless, if it is not necessary to know the structure of the whole
 * event, it is recommended to choose the lighter YODA format, thus saving
 * computational time and resources (especially the RAM).
 *
 * \section rivet_output_user_guide_config_ Configuration
 *
 * The Rivet process can be configured in the main configuration
 * file.  This is done by adding keys to the \key Rivet key.  Here, we can
 *
 * - Set load paths for data and analyses (\key Paths)
 * - Preload data files - e.g., centrality calibrations (\key Preloads)
 * - Specify which analyses to run (\key Analyses)
 * - Choose whether to not validate beams (\key Ignore_Beams)
 * - Set log levels in Rivet (\key Logging)
 * - Specify weight handling (\key Weights)
 * - and more ... (see below)
 *
 * An example:
 * \verbatim
 Output:
    Rivet:
      Format: ["YODA"]
      Ignore_Beams: True
      Logging:
        Rivet.AnalysisHandler: Debug
        Rivet.Analysis.MC_FSPARTICLES: Debug
      Analyses:
        - MC_FSPARTICLES
 \endverbatim
 *
 *
 * The Rivet set-up can be configured through the \ref
 * output_general_ "Output" section in the configuration file
 *
 * - \key Rivet (top of Rivet configuration)
 *   - \key Format (list of strings, no default) List of formats
 *     to generate Rivet instance for. Can be one of
 *
 *     - \key YODA Only initial (beam) and final state particles
 *       are available in the events.
 *
 *     - \key YODA-full Full event structure present to analyse
 *
 *   - \key Paths (list of strings, no default)
 *     This key specifies the directories that Rivet will search for
 *     analyses and data files related to the analyses.
 *   - \key Analyses (list of strings, no default)
 *     This key specifies the analyses (including opossible options)
 *     to add to the Rivet analysis.
 *
 *   - \key Preloads (list of strings, no default)
 *     Specify data files to read into Rivet (e.g., centrality
 *     calibrations) at start-up.
 *
 *   - \key Logging (map of string to string, no default)
 *     Specifies log levels for various parts of Rivet, including
 *     analyses.  Each entry is a log name followed by a log level
 *     (one of TRACE,DEBUG,INFO,WARN,ERROR, and FATAL)
 *
 *   - \key Ignore_Beams (bool, default true) Ask Rivet to not
 *     validate beams before running analyses.  This is needed if
 *     you use the option \key Fermi_Motion (\ref
 *     input_modi_collider_) that disrupts the collision energy
 *     event-by-event
 *
 *   - \key Cross_Section (double,double, no default)
 *     Set the cross-section in pico-barns
 *
 *   - \key Weights (container, no defaults)
 *
 *     - \key No_Multi (bool, default false)
 *       Ask Rivet to not do multi-weight processing
 *
 *     - \key Nominal (string, no default)
 *       The nominal weight name
 *
 *     - \key Select (list of string, no default)
 *       Select these weights for processing
 *
 *     - \key Deselect (list of string, no default)
 *       De-select these weights for processing
 *
 *     - \key NLO_Smearing (double, default 0)
 *       Smearing histogram binning by given fraction of bin widths
 *       to avoid NLO counter events to flow into neighboring bin.
 *
 *     - \key Cap (double, no default)
 *       Cap weights to this value.
 *
 *
 */

RivetOutput::RivetOutput(const bf::path& path, std::string name,
                         const bool full_event, const bool is_an_ion_collision,
                         const OutputParameters& out_par)
    : HepMcInterface(name, full_event, is_an_ion_collision),
      handler_(),
      filename_(path / (name + ".yoda")),
      need_init_(true),
      rivet_confs_(out_par.subcon_for_rivet) {
  handler_ = std::make_shared<Rivet::AnalysisHandler>();
  setup();
}

RivetOutput::~RivetOutput() {
  logg[LOutput].debug() << "Writing Rivet results to " << filename_
                        << std::endl;
  analysis_handler_proxy()->finalize();
  analysis_handler_proxy()->writeData(filename_.string());
}

void RivetOutput::at_eventend(const Particles& particles,
                              const int32_t event_number,
                              const EventInfo& event) {
  HepMcInterface::at_eventend(particles, event_number, event);

  // Initialize Rivet on first event
  if (need_init_) {
    logg[LOutput].debug() << "Initialising Rivet" << std::endl;
    need_init_ = false;
    analysis_handler_proxy()->init(event_);
  }

  logg[LOutput].debug() << "Analysing event " << event_number << std::endl;
  // Let Rivet analyse the event
  analysis_handler_proxy()->analyze(event_);
}

void RivetOutput::add_analysis(const std::string& name) {
  analysis_handler_proxy()->addAnalysis(name);
}

void RivetOutput::add_path(const std::string& path) {
  Rivet::addAnalysisLibPath(path);
  Rivet::addAnalysisDataPath(path);
}

void RivetOutput::add_preload(const std::string& file) {
  analysis_handler_proxy()->readData(file);
}

void RivetOutput::set_ignore_beams(bool ignore) {
  logg[LOutput].info() << "Ignore beams? " << (ignore ? "yes" : "no")
                       << std::endl;
  analysis_handler_proxy()->setIgnoreBeams(ignore);
}

void RivetOutput::set_log_level(const std::string& name,
                                const std::string& level) {
  std::string fname(name);
  if (fname.rfind("Rivet", 0) != 0) {
    fname = "Rivet." + fname;
  }

  auto upcase = [](const std::string& s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](char c) { return std::toupper(c); });
    return out;
  };

  try {
    Rivet::Log::setLevel(fname, Rivet::Log::getLevelFromName(upcase(level)));
  } catch (...) {
  }
}

void RivetOutput::set_cross_section(double xs, double xserr) {
  analysis_handler_proxy()->setCrossSection(xs, xserr, true);
}

void RivetOutput::setup() {
  logg[LOutput].debug() << "Setting up from configuration:\n"
                        << rivet_confs_.to_string() << std::endl;

  // Paths to analyses libraries and data
  if (rivet_confs_.has_value({"Paths"})) {
    logg[LOutput].info() << "Processing paths" << std::endl;
    std::vector<std::string> path = rivet_confs_.take({"Paths"});
    for (auto p : path)
      add_path(p);
  }

  // Data files to pre-load e.g., for centrality configurations
  if (rivet_confs_.has_value({"Preloads"})) {
    logg[LOutput].info() << "Processing preloads" << std::endl;
    std::vector<std::string> prel = rivet_confs_.take({"Preloads"});
    for (auto p : prel)
      add_preload(p);
  }

  // Analyses (including options) to add to run
  if (rivet_confs_.has_value({"Analyses"})) {
    logg[LOutput].info() << "Processing analyses" << std::endl;
    std::vector<std::string> anas = rivet_confs_.take({"Analyses"});
    for (auto p : anas)
      add_analysis(p);
  }

  // Whether Rivet should ignore beams
  if (rivet_confs_.has_value({"Ignore_Beams"})) {
    set_ignore_beams(rivet_confs_.take({"Ignore_Beams"}));
  } else {
    // we must explicity tell Rivet, through the handler, to ignore beam checks
    set_ignore_beams(true);
  }

  // Cross sections
  if (rivet_confs_.has_value({"Cross_Section"})) {
    std::array<double, 2> xs = rivet_confs_.take({"Cross_Section"});
    set_cross_section(xs[0], xs[1]);
  }

  // Logging in Rivet
  if (rivet_confs_.has_value({"Logging"})) {
    std::map<std::string, std::string> logs = rivet_confs_.take({"Logging"});
    for (auto nl : logs)
      set_log_level(nl.first, nl.second);
  }

  // Treatment of event weights in Rivet
  if (rivet_confs_.has_value({"Weights"})) {
    auto wconf = rivet_confs_["Weights"];

    // Do not care about multi weights - bool
    if (wconf.has_value({"No_Multi"})) {
      analysis_handler_proxy()->skipMultiWeights(wconf.take({"No_Multi"}));
    }

    // Set nominal weight name
    if (wconf.has_value({"Nominal"})) {
      analysis_handler_proxy()->setNominalWeightName(wconf.take({"Nominal"}));
    }

    // Set cap (maximum) on weights
    if (wconf.has_value({"Cap"})) {
      analysis_handler_proxy()->setWeightCap(wconf.take({"Cap"}));
    }

    // Whether to smear for NLO calculations
    if (wconf.has_value({"NLO_Smearing"})) {
      analysis_handler_proxy()->setNLOSmearing(wconf.take({"NLO_Smearing"}));
    }

    // Select which weights to enable
    if (wconf.has_value({"Select"})) {
      std::vector<std::string> sel = wconf.take({"Select"});
      std::stringstream s;
      int comma = 0;
      for (auto w : sel)
        s << (comma++ ? "," : "") << w;
      analysis_handler_proxy()->selectMultiWeights(s.str());
    }

    // Select weights to disable
    if (wconf.has_value({"Deselect"})) {
      std::vector<std::string> sel = wconf.take({"Deselect"});
      std::stringstream s;
      int comma = 0;
      for (auto w : sel)
        s << (comma++ ? "," : "") << w;
      analysis_handler_proxy()->deselectMultiWeights(s.str());
    }
  }
  logg[LOutput].debug() << "After processing configuration:\n"
                        << rivet_confs_.to_string() << std::endl;
}
}  // namespace smash
