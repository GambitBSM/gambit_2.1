//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Implementation of DMEFT
///  DarkBit routines.
///
///  Authors (add name and date if you modify):    
///       *** Automatically created by GUM ***     
///                                                
///  \author The GAMBIT Collaboration             
///  \date 12:32PM on October 15, 2019
///
///  \author Sanjay Bloor
///         (sanjay.bloor12@imperial.ac.uk)
///  \date Oct 2019
///                                                  
///  ********************************************* 

#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/DarkBit/DarkBit_rollcall.hpp"
#include "gambit/Utils/ascii_table_reader.hpp"
#include "boost/make_shared.hpp"
#include "gambit/DarkBit/DarkBit_utils.hpp"

namespace Gambit
{
  namespace DarkBit
  {
    class DMEFT
    {
      public:
      /// Initialize DMEFT object (branching ratios etc)
      DMEFT() {};
      ~DMEFT() {};
      
      // Annihilation cross-section. sigmav is a pointer to a CalcHEP backend function.
      double sv(str channel, DecayTable& tbl, double (*sigmav)(str&, std::vector<str>&, std::vector<str>&, double&, double&, const DecayTable&), double v_rel)
      {
        /// Returns sigma*v for a given channel.
        double GeV2tocm3s1 = gev2cm2*s2cm;
        
        /// Hard-coded for now -- CalcHEP frontend needs this removing anyway, it doesn't use it.
        double QCD_coupling = 1.0;
        
        // CalcHEP args
        str model = "DMEFT"; // CalcHEP model name
        std::vector<str> in = {"chi", "chi~"}; // In states: DM+DMbar
        std::vector<str> out; // Out states
        if (channel == "dbar_1, d_1") out = {"dbar_1", "d_1"};
        if (channel == "ubar_1, u_1") out = {"ubar_1", "u_1"};
        if (channel == "dbar_2, d_2") out = {"dbar_2", "d_2"};
        if (channel == "ubar_2, u_2") out = {"ubar_2", "u_2"};
        if (channel == "dbar_3, d_3") out = {"dbar_3", "d_3"};
        if (channel == "ubar_3, u_3") out = {"ubar_3", "u_3"};
        
        // Check the channel has been filled
        if (out.size() > 1) return sigmav(model, in, out, QCD_coupling, v_rel, tbl)*GeV2tocm3s1;
        else return 0;
      }
      
      
    };
    
    void TH_ProcessCatalog_DMEFT(DarkBit::TH_ProcessCatalog &result)
    {
      using namespace Pipes::TH_ProcessCatalog_DMEFT;
      using std::vector;
      using std::string;
      
      // Initialize empty catalog, main annihilation process
      TH_ProcessCatalog catalog;
      TH_Process process_ann("chi", "chi~");
      
      // Explicitly state that Dirac DM is not self-conjugate to add extra 
      // factors of 1/2 where necessary
      process_ann.isSelfConj = false;
      
      
      // Import particle masses 
      
      // Convenience macros
      #define getSMmass(Name, spinX2) catalog.particleProperties.insert(std::pair<string, TH_ParticleProperty> (Name, TH_ParticleProperty(SM.get(Par::Pole_Mass,Name), spinX2)));
      #define addParticle(Name, Mass, spinX2) catalog.particleProperties.insert(std::pair<string, TH_ParticleProperty> (Name, TH_ParticleProperty(Mass, spinX2)));
      
      // Import Spectrum objects
      const Spectrum& spec = *Dep::DMEFT_spectrum;
      const SubSpectrum& SM = spec.get_LE();
      const SMInputs& SMI   = spec.get_SMInputs();
      
      // Get SM pole masses
      getSMmass("e-_1",     1)
      getSMmass("e+_1",     1)
      getSMmass("e-_2",     1)
      getSMmass("e+_2",     1)
      getSMmass("e-_3",     1)
      getSMmass("e+_3",     1)
      getSMmass("Z0",       2)
      getSMmass("W+",       2)
      getSMmass("W-",       2)
      getSMmass("g",        2)
      getSMmass("gamma",    2)
      getSMmass("u_3",      1)
      getSMmass("ubar_3",   1)
      getSMmass("d_3",      1)
      getSMmass("dbar_3",   1)
      
      // Pole masses not available for the light quarks.
      addParticle("u_1"   , SMI.mU,  1) // mu(2 GeV)^MS-bar
      addParticle("ubar_1", SMI.mU,  1) // mu(2 GeV)^MS-bar
      addParticle("d_1"   , SMI.mD,  1) // md(2 GeV)^MS-bar
      addParticle("dbar_1", SMI.mD,  1) // md(2 GeV)^MS-bar
      addParticle("u_2"   , SMI.mCmC,1) // mc(mc)^MS-bar
      addParticle("ubar_2", SMI.mCmC,1) // mc(mc)^MS-bar
      addParticle("d_2"   , SMI.mS,  1) // ms(2 GeV)^MS-bar
      addParticle("dbar_2", SMI.mS,  1) // ms(2 GeV)^MS-bar
      
      // Masses for neutrino flavour eigenstates. Set to zero.
      // (presently not required)
      addParticle("nu_e",     0.0, 1)
      addParticle("nubar_e",  0.0, 1)
      addParticle("nu_mu",    0.0, 1)
      addParticle("nubar_mu", 0.0, 1)
      addParticle("nu_tau",   0.0, 1)
      addParticle("nubar_tau",0.0, 1)
      
      // Meson masses
      addParticle("pi0",   meson_masses.pi0,       0)
      addParticle("pi+",   meson_masses.pi_plus,   0)
      addParticle("pi-",   meson_masses.pi_minus,  0)
      addParticle("eta",   meson_masses.eta,       0)
      addParticle("rho0",  meson_masses.rho0,      1)
      addParticle("rho+",  meson_masses.rho_plus,  1)
      addParticle("rho-",  meson_masses.rho_minus, 1)
      addParticle("omega", meson_masses.omega,     1)
      
      // DMEFT-specific masses
      double mchi = spec.get(Par::Pole_Mass, "chi");
      addParticle("chi", mchi, 1);
      addParticle("h0_1", spec.get(Par::Pole_Mass, "h0_1"), 0);
      
      // Get rid of convenience macros
      #undef getSMmass
      #undef addParticle
      
      // Import decay table from DecayBit
      DecayTable tbl = *Dep::decay_rates;
      
      // Set of imported decays
      std::set<string> importedDecays;
      
      // Minimum branching ratio to include
      double minBranching = runOptions->getValueOrDef<double>(0.0, "ProcessCatalog_MinBranching");
      
      // Import relevant decays
      using DarkBit_utils::ImportDecays;
      
      auto excludeDecays = daFunk::vec<std::string>("Z0", "W+", "W-", "u_3", "ubar_3", "e+_2", "e-_2", "e+_3", "e-_3");
      
      ImportDecays("h0_1", catalog, importedDecays, &tbl, minBranching, excludeDecays);
      
      // Instantiate new DMEFT object.
      auto pc = boost::make_shared<DMEFT>();
      
      // Populate annihilation channel list and add thresholds to threshold list.
      process_ann.resonances_thresholds.threshold_energy.push_back(2*mchi);
      auto channels = 
        daFunk::vec<string>("dbar_1, d_1", "ubar_1, u_1", "dbar_2, d_2", "ubar_2, u_2", "dbar_3, d_3", "ubar_3, u_3");
      auto p1 = 
        daFunk::vec<string>("dbar_1", "ubar_1", "dbar_2", "ubar_2", "dbar_3", "ubar_3");
      auto p2 = 
        daFunk::vec<string>("d_1", "u_1", "d_2", "u_2", "d_3", "u_3");
      
      for (unsigned int i = 0; i < channels.size(); ++i)
      {
        double mtot_final = 
        catalog.getParticleProperty(p1[i]).mass + 
        catalog.getParticleProperty(p2[i]).mass;  
        if (mchi*2 > mtot_final*0.5)
        {
          daFunk::Funk kinematicFunction = daFunk::funcM(pc, &DMEFT::sv, channels[i], tbl, 
          BEreq::CH_Sigma_V.pointer(), daFunk::var("v"));
          TH_Channel new_channel(daFunk::vec<string>(p1[i], p2[i]), kinematicFunction);
          process_ann.channelList.push_back(new_channel);
        }
        if (mchi*2 > mtot_final)
        {
          process_ann.resonances_thresholds.threshold_energy.
          push_back(mtot_final);
        }
      }
      
      catalog.processList.push_back(process_ann);
      
      // Validate
      catalog.validate();
      
      result = catalog;
    } // function TH_ProcessCatalog
    
    void DarkMatter_ID_DMEFT(std::string& result){ result = "chi"; }

    /// Relativistic Wilson Coefficients for direct detection
    /// DMEFT basis is the same as that used in DirectDM
    void DD_rel_WCs_DMEFT(map_str_dbl& result)
    {
      using namespace Pipes::DD_rel_WCs_DMEFT;

      const Spectrum& spec = *Dep::DMEFT_spectrum;

      // In our model the Wilson coefficients are dimensionless
      double Lambda = spec.get(Par::mass1, "Lambda");
      double C51  = spec.get(Par::dimensionless, "C51");
      double C52  = spec.get(Par::dimensionless, "C52");
      double C61  = spec.get(Par::dimensionless, "C61");
      double C62  = spec.get(Par::dimensionless, "C62");
      double C63  = spec.get(Par::dimensionless, "C63");
      double C64  = spec.get(Par::dimensionless, "C64");
      double C71  = spec.get(Par::dimensionless, "C71");
      double C72  = spec.get(Par::dimensionless, "C72");
      double C73  = spec.get(Par::dimensionless, "C73");
      double C74  = spec.get(Par::dimensionless, "C74");
      double C75  = spec.get(Par::dimensionless, "C75");
      double C76  = spec.get(Par::dimensionless, "C76");
      double C77  = spec.get(Par::dimensionless, "C77");
      double C78  = spec.get(Par::dimensionless, "C78");
      double C79  = spec.get(Par::dimensionless, "C79");
      double C710 = spec.get(Par::dimensionless, "C710");


      // So we need to rescale them by the appropriate scale
      result["C51"]  = C51/Lambda;
      result["C52"]  = C52/Lambda;
      // No C53 + C54 in the model -- Higgs portals terms -- we have those models already.
      result["C61"]  = C61/pow(Lambda, 2);
      result["C62"]  = C62/pow(Lambda, 2);
      result["C63"]  = C63/pow(Lambda, 2);
      result["C64"]  = C64/pow(Lambda, 2);
      result["C71"]  = C71/pow(Lambda, 3);
      result["C72"]  = C72/pow(Lambda, 3);
      result["C73"]  = C73/pow(Lambda, 3);
      result["C74"]  = C74/pow(Lambda, 3);
      result["C75"]  = C75/pow(Lambda, 3);
      result["C76"]  = C76/pow(Lambda, 3);
      result["C77"]  = C77/pow(Lambda, 3);
      result["C78"]  = C78/pow(Lambda, 3);
      result["C79"]  = C79/pow(Lambda, 3);
      result["C710"] = C710/pow(Lambda, 3);

    }
    
  } //namespace DarkBit
  
} //namespace Gambit

