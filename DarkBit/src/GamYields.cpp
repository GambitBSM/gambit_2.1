//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Routines for the calculation of gamma-ray yield 
///  from dark matter annihilation / decay.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///   
///  \author Christoph Weniger
///          (c.weniger@uva.nl)
///  \date 2013 Jul - 2015 May
///
///  *********************************************

#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/DarkBit/DarkBit_rollcall.hpp"
#include "gambit/Utils/ascii_table_reader.hpp"
#include "gambit/DarkBit/DarkBit_utils.hpp"

//#define DARKBIT_DEBUG

namespace Gambit {
  namespace DarkBit {

    //////////////////////////////////////////////////////////////////////////
    //
    //                        Gamma-ray yields
    //
    //////////////////////////////////////////////////////////////////////////


    /*! \brief Identification of final states that are not yet tabulated.
     *
     * Structure
     * ---------
     *
     * 1) Go through process catalog and find all final states that require
     * to be calculated in the cascade code.  To this end, check whether
     * two-body channels are tabulated for two-body final states, and whether
     * one-particle spectra exist for one-particle final states.
     *
     * 2) Calculate via the cascade code the missing energy spectra.
     *
     * 3) Put together the full spectrum.
     *
     */
#define DARKBIT_DEBUG
    void GA_missingFinalStates(std::vector<std::string> &result)
    {
      using namespace Pipes::GA_missingFinalStates;
      std::set<std::string> missingFinalStates;
      std::string DMid= *Dep::DarkMatter_ID;

      if ( runOptions->getValueOrDef(false, "ignore_all") ) return;

      TH_Process process = (*Dep::TH_ProcessCatalog).getProcess(DMid, DMid);

      // Add only gamma-ray spectra for two and three body final states
      for (std::vector<TH_Channel>::iterator it = process.channelList.begin();
          it != process.channelList.end(); ++it)
      {
        if ( it->nFinalStates == 2 )
        {
          if ( not runOptions->getValueOrDef(false, "ignore_two_body") )
          {
            #ifdef DARKBIT_DEBUG
              std::cout << "Checking for missing two-body final states: " 
                        << it->finalStateIDs[0] << " " << it->finalStateIDs[1]  << std::endl;
            #endif
            if ( not Dep::SimYieldTable->hasChannel(it->finalStateIDs[0], it->finalStateIDs[1], "gamma") )
            {
                if ( not Dep::SimYieldTable->hasChannel(it->finalStateIDs[0], "gamma") )
                  missingFinalStates.insert(it->finalStateIDs[0]);
                if ( not Dep::SimYieldTable->hasChannel(it->finalStateIDs[1], "gamma") )
                    missingFinalStates.insert(it->finalStateIDs[1]);
            }
          }
        }
        else if ( it->nFinalStates == 3 )
        {
          if ( not runOptions->getValueOrDef(false, "ignore_three_body") )
          {
            #ifdef DARKBIT_DEBUG
              std::cout << "Checking for missing three-body final states: " 
                        << it->finalStateIDs[0] << " " << it->finalStateIDs[1]  
                        << " " << it->finalStateIDs[2] << std::endl;
            #endif
            if (not Dep::SimYieldTable->hasChannel(it->finalStateIDs[0], "gamma") )
              missingFinalStates.insert(it->finalStateIDs[0]);
            if (not Dep::SimYieldTable->hasChannel(it->finalStateIDs[1], "gamma") )
              missingFinalStates.insert(it->finalStateIDs[1]);
            if (not Dep::SimYieldTable->hasChannel(it->finalStateIDs[2], "gamma") )
              missingFinalStates.insert(it->finalStateIDs[2]);
          }
        }
      }
      // Remove particles we don't have decays for.
      for (auto it = missingFinalStates.begin(); it != missingFinalStates.end();) 
      {
          if ((*Dep::TH_ProcessCatalog).find(*it, "") == NULL) 
          {
#ifdef DARKBIT_DEBUG
            std::cout << "Erasing (because no decays known): " << *it << std::endl;
#endif
            missingFinalStates.erase(it++);
          }
          else
          {
            ++it;
#ifdef DARKBIT_DEBUG
            std::cout << "Keeping (because decay known): " << *it << std::endl;
#endif
          }
      }

      #ifdef DARKBIT_DEBUG
        std::cout << "Number of missing final states: " << missingFinalStates.size() << std::endl;
        for (auto it = missingFinalStates.begin(); it != missingFinalStates.end(); it++)
        {
          std::cout << *it << std::endl;
        }
      #endif

      result.assign(missingFinalStates.begin(), missingFinalStates.end());
    }
#undef DARKBIT_DEBUG

    /*! \brief Boosts an energy spectrum of isotropic particles into another
     *         frame (and isotropizes again).
     *  Parameters:
     *    gamma: Lorentz boost factor
     *    dNdE: Spectrum
     *    mass: mass of particle
     */
    Funk::Funk boost_dNdE(Funk::Funk dNdE, double gamma, double mass)
    {
      if ( gamma < 1.0 + .02 )  // Ignore less than 2% boosts
      {
        if (gamma < 1.0)
          DarkBit_error().raise(LOCAL_INFO, 
            "boost_dNdE: Requested Lorentz boost with gamma < 1");
        return dNdE;
      }
      double betaGamma = sqrt(gamma*gamma-1);
      Funk::Funk E = Funk::var("E");
      Funk::Funk lnE = Funk::var("lnE");
      Funk::Funk Ep = Funk::var("Ep");
      Funk::Funk halfBox_int = betaGamma*sqrt(E*E-mass*mass);
      Funk::Funk halfBox_bound = betaGamma*sqrt(Ep*Ep-mass*mass);
      Funk::Funk integrand = dNdE/(2*halfBox_int);
      //return integrand->gsl_integration("E", Ep*gamma-halfBox_bound, Ep*gamma+halfBox_bound)
      //  ->set_epsabs(0)->set_epsrel(1e-3)->set("Ep", Funk::var("E"));
      //
      // Numerically more stable to integrate over lnE instead
      return (integrand*E)->set("E", exp(lnE))->gsl_integration("lnE", log(Ep*gamma-halfBox_bound), log(Ep*gamma+halfBox_bound))
        ->set_epsabs(0)->set_epsrel(1e-3)->set("Ep", Funk::var("E"));
    }

    /*! \brief General routine to derive annihilation yield.
     *
     * Depends on:
     * - SimYieldTable
     * - TH_ProcessCatalog
     * - cascadeMC_gammaSpectra
     *
     * This function returns 
     *
     *   dN/dE*(sv)/mDM**2 (E, v)  [cm^3/s/GeV^3]
     *
     * the energy spectrum of photons times sigma*v/m^2, as function of
     * energy (GeV) and velocity (c).  By default, only the v=0 component
     * is calculated.  
     *
     */
    void GA_AnnYield_General(Funk::Funk &result)
    {
      using namespace Pipes::GA_AnnYield_General;
      using DarkBit_utils::gamma3bdy_limits;

      std::string DMid= *Dep::DarkMatter_ID;

      double line_width = runOptions->getValueOrDef<double>(0.03,  "line_width");

      // Get annihilation process from process catalog
      TH_Process annProc = (*Dep::TH_ProcessCatalog).getProcess(DMid, DMid);

      // Get particle mass from process catalog
      const double mass = (*Dep::TH_ProcessCatalog).getParticleProperty(DMid).mass;
      const double Ecm = 2*mass;

      // Loop over all channels for that process
      Funk::Funk Yield = Funk::zero("E", "v");

      // Adding two-body channels
      for (std::vector<TH_Channel>::iterator it = annProc.channelList.begin();
          it != annProc.channelList.end(); ++it)
      {
        // Here only take care of two-body final states
        if (it->nFinalStates != 2) continue;
        
        // Get final state masses
        double m0 = (*Dep::TH_ProcessCatalog).getParticleProperty(
            it->finalStateIDs[0]).mass;
        double m1 = (*Dep::TH_ProcessCatalog).getParticleProperty(
            it->finalStateIDs[1]).mass;

        // Ignore channels that are kinematically closed for v=0
        if ( m0 + m1 > Ecm ) continue;

        // Ignore channels with 0 BR in v=0 limit
        if (it->genRate->bind("v")->eval(0.) <= 0.) continue;

        double E0 = 0.5*(Ecm*Ecm+m0*m0-m1*m1)/Ecm;
        double E1 = Ecm-E0;

        // Check whether two-body final state is in SimYield table
        if ( Dep::SimYieldTable->hasChannel(
              it->finalStateIDs[0], it->finalStateIDs[1], "gamma") )
        {
          Yield = Yield +
            it->genRate*(*Dep::SimYieldTable)(
                it->finalStateIDs[0], it->finalStateIDs[1], "gamma", Ecm);
        }
        // Deal with composite final states
        else
        {
          Funk::Funk spec0 = Funk::zero("E");
          Funk::Funk spec1 = Funk::zero("E");        

          // Final state particle one
          // Tabulated spectrum available?
          if ( Dep::SimYieldTable->hasChannel(it->finalStateIDs[0], "gamma") )
          {
            spec0 = (*Dep::SimYieldTable)(it->finalStateIDs[0], "gamma")->set("Ecm",E0);
          }
          // Gamma-ray line?
          else if ( it->finalStateIDs[0] == "gamma" )
          {
            Funk::Funk E = Funk::var("E");
            spec0 = exp(-pow((E-E0)/line_width/E0,2)/2)/E0/sqrt(2*M_PI)/line_width;
          }
          // MC spectra available?
          else if ( Dep::cascadeMC_gammaSpectra->count(it->finalStateIDs[0]) )
          {
            double gamma0 = E0/m0;
            spec0 = boost_dNdE(Dep::cascadeMC_gammaSpectra->at(it->finalStateIDs[0]), gamma0, 0.0);
          }        

          // Final state particle two
          if ( Dep::SimYieldTable->hasChannel(it->finalStateIDs[1], "gamma") )
          {
            spec1 = (*Dep::SimYieldTable)(it->finalStateIDs[1], "gamma")->set("Ecm", E1);
          }
          else if ( it->finalStateIDs[1] == "gamma" )
          {
            Funk::Funk E = Funk::var("E");
            spec1 = exp(-pow((E-E1)/line_width/E1,2)/2)/E1/sqrt(2*M_PI)/line_width;
          }
          else if ( Dep::cascadeMC_gammaSpectra->count(it->finalStateIDs[1]) )
          {
            double gamma1 = E1/m1;
            spec1 = boost_dNdE(Dep::cascadeMC_gammaSpectra->at(it->finalStateIDs[1]), gamma1, 0.0);
          }

#ifdef DARKBIT_DEBUG
          std::cout << it->finalStateIDs[0] << " " << it->finalStateIDs[1] << std::endl;
          std::cout << "gammas: " << gamma0 << ", " << gamma1 << std::endl;
          Funk::Funk chnSpec = (Funk::zero("v", "E") 
            +  spec0 
            +  spec1)-> set("v", 0.);
          std::vector<double> y = chnSpec->bind("E")->vect(x);
          os << it->finalStateIDs[0] << it->finalStateIDs[1] << ":\n";
          os << "  E: [";
          for (std::vector<double>::iterator it2 = x.begin(); it2 != x.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
          os << "  dNdE: [";
          for (std::vector<double>::iterator it2 = y.begin(); it2 != y.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
#endif

          Yield = Yield + (spec0 + spec1) * it->genRate;
        }
      } // End adding two-body final states
          
#ifdef DARKBIT_DEBUG
      std::vector<std::string> test1 = initVector<std::string> ("h0_1_test","h0_2_test","h0_2_test","h0_1_test","WH_test", "A0_test", "h0_1_test", "W+");
      std::vector<std::string> test2 = initVector<std::string> ("A0_test",  "A0_test",  "Z0_test",  "Z0_test",  "WH_test", "Z0_test", "h0_2_test", "W-");
    
      for(size_t i=0; i<test1.size();i++)
      {
          Funk::Funk chnSpec = (*Dep::SimYieldTable)(test1[i], test2[i], "gamma", Ecm);
          std::vector<double> y = chnSpec->bind("E")->vect(x);
          os << test1[i] << test2[i] << ":\n";
          os << "  E: [";
          for (std::vector<double>::iterator it2 = x.begin(); it2 != x.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
          os << "  dNdE: [";
          for (std::vector<double>::iterator it2 = y.begin(); it2 != y.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
      }
#endif
      
      // Adding three-body final states
      //
      // NOTE:  Three body processes are added even if they are closed for at v=0
      for (std::vector<TH_Channel>::iterator it = annProc.channelList.begin();
          it != annProc.channelList.end(); ++it)
      {
        // Here only take care of three-body final states
        if (it->nFinalStates != 3) continue;

        // Implement tabulated three-body final states
        /*
           if ( it->nFinalStates == 3
           and Dep::SimYieldTable->hasChannel(it->finalStateIDs[0], "gamma")
           and Dep::SimYieldTable->hasChannel(it->finalStateIDs[1], "gamma")
           and Dep::SimYieldTable->hasChannel(it->finalStateIDs[2], "gamma")
           )
           {
           Funk::Funk dNdE1dE2 = it->genRate->set("v",0.);
           Funk::Funk spec0 = 
             (*Dep::SimYieldTable)(it->finalStateIDs[0], "gamma");
           Funk::Funk spec1 = 
             (*Dep::SimYieldTable)(it->finalStateIDs[1], "gamma");
           Funk::Funk spec2 = 
             (*Dep::SimYieldTable)(it->finalStateIDs[2], "gamma");
           Yield = Yield + convspec(spec0, spec1, spec2, dNdE1dE2);
           }
        */

        if ( it->finalStateIDs[0] == "gamma" )
        {
          if ( it->finalStateIDs[1] == "gamma" or it->finalStateIDs[2] == "gamma")
          {
            // FIXME: Decide whether this is acceptable for the first release
            DarkBit_warning().raise(LOCAL_INFO, "Second and/or third primary gamma rays in three-body final states ignored.");
          }
          double m1 = (*Dep::TH_ProcessCatalog).getParticleProperty(
              it->finalStateIDs[1]).mass;
          double m2 = (*Dep::TH_ProcessCatalog).getParticleProperty(
              it->finalStateIDs[2]).mass;
          Funk::Funk E1_low =  Funk::func(gamma3bdy_limits<0>, Funk::var("E"),
              mass, m1, m2);
          Funk::Funk E1_high =  Funk::func(gamma3bdy_limits<1>, Funk::var("E"),
              mass, m1, m2);
          Funk::Funk dsigmavde = it->genRate->gsl_integration(
              "E1", E1_low, E1_high);

#ifdef DARKBIT_DEBUG
          Funk::Funk chnSpec = (Funk::zero("v", "E") + dsigmavde)-> set("v", 0.);
          std::vector<double> y = chnSpec->bind("E")->vect(x);
          os << it->finalStateIDs[0] << it->finalStateIDs[1] << it->finalStateIDs[2] << ":\n";
          os << "  E: [";
          for (std::vector<double>::iterator it2 = x.begin(); it2 != x.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
          os << "  dNdE: [";
          for (std::vector<double>::iterator it2 = y.begin(); it2 != y.end(); it2++)
            os << *it2 << ", ";
          os  << "]\n";
#endif

          Yield = Yield + dsigmavde;
        }
      }
#ifdef DARKBIT_DEBUG
      if(debug) os.close();
#endif

      result = Funk::ifelse(1e-6 - Funk::var("v"), Yield/(mass*mass), 
          Funk::throwError("Spectrum currently only defined for v=0."));
    }


    /// SimYieldTable based on DarkSUSY tabulated results.
    void SimYieldTable_DarkSUSY(SimYieldTable& result)
    {
      using namespace Pipes::SimYieldTable_DarkSUSY;

      static bool initialized = false;
      if ( not initialized )
      {
        int flag = 0;      // some flag
        int yieldk = 152;  // gamma ray yield
        Funk::Funk dNdE;

        using DarkBit_utils::str_flav_to_mass;
        
#define ADD_CHANNEL(ch, P1, P2, FINAL, EcmMin, EcmMax)                \
        dNdE = Funk::func_fromThreadsafe(                             \
            BEreq::dshayield.pointer(), Funk::var("mwimp"),           \
            Funk::var("E"), ch, yieldk, flag)->set("mwimp",           \
            Funk::var("Ecm")/2);                                      \
        result.addChannel(dNdE, str_flav_to_mass(P1), str_flav_to_mass(P2), FINAL, EcmMin, EcmMax);  

        // specifies also center of mass energy range
        // FIXME: Update energy validity ranges
        ADD_CHANNEL(12, "Z0", "Z0", "gamma", 91.2*2, 10000.)
        ADD_CHANNEL(13, "W+", "W-", "gamma", 0., 10000.)
        ADD_CHANNEL(14, "nu_e", "nubar_e", "gamma", 0., 10000.)
        ADD_CHANNEL(15, "e+", "e-", "gamma", 0., 10000.)
        ADD_CHANNEL(16, "nu_mu", "nubar_mu", "gamma", 0., 10000.)
        ADD_CHANNEL(17, "mu+", "mu-", "gamma", 0., 10000.)
        ADD_CHANNEL(18, "nu_tau", "nubar_tau", "gamma", 0., 10000.)
        ADD_CHANNEL(19, "tau+", "tau-", "gamma", 0., 10000.)
        ADD_CHANNEL(20, "u", "ubar", "gamma", 0., 10000.)
        ADD_CHANNEL(21, "d", "dbar", "gamma", 0., 10000.)
        ADD_CHANNEL(22, "c", "cbar", "gamma", 0., 10000.)
        ADD_CHANNEL(23, "s", "sbar", "gamma", 0., 10000.)
        ADD_CHANNEL(24, "t", "tbar", "gamma", 0., 10000.)
        ADD_CHANNEL(25, "b", "bbar", "gamma", 0., 10000.)
        ADD_CHANNEL(26, "g", "g", "gamma", 0., 10000.)
        // FIXME: Double-check validity of neutrino channels
#undef ADD_CHANNEL

        // Add approximations for single-particle cases.
        // FIXME: Update energy validity ranges
        // FIXME: We could actually use boosted rest-frame spectra instead -- discuss
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 12, yieldk, flag);
        result.addChannel(dNdE/2, "Z0", "gamma", 91.2, 10000.);
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 13, yieldk, flag);
        result.addChannel(dNdE/2, "W+", "gamma", 0., 10000.);
        result.addChannel(dNdE/2, "W-", "gamma", 0., 10000.);
        // FIXME: This is a bad approximation to final state radiation, I suppose
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 15, yieldk, flag);
        result.addChannel(dNdE/2, str_flav_to_mass("e+"), "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("e-"), "gamma", 0., 10000.);
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 17, yieldk, flag);
        result.addChannel(dNdE/2, str_flav_to_mass("mu+"), "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("mu-"), "gamma", 0., 10000.);        
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 19, yieldk, flag);
        result.addChannel(dNdE/2, str_flav_to_mass("tau+"), "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("tau-"), "gamma", 0., 10000.);
        // Add single particle lookup for t tbar to prevent them from being tagged as missing final states for cascades.
        dNdE = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), Funk::var("Ecm"), Funk::var("E"), 24, yieldk, flag);
        result.addChannel(dNdE/2, str_flav_to_mass("t"),    "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("tbar"), "gamma", 0., 10000.);        
        
        // Approximations for mixed quark channels
        Funk::Funk dNdE_u = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 20, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);
        Funk::Funk dNdE_d = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 21, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);
        Funk::Funk dNdE_c = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 22, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);
        Funk::Funk dNdE_s = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 23, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);   
        Funk::Funk dNdE_t = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 24, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);
        Funk::Funk dNdE_b = Funk::func_fromThreadsafe(BEreq::dshayield.pointer(), 
                              Funk::var("mwimp"), Funk::var("E"), 25, yieldk, flag)->set("mwimp", Funk::var("Ecm")/2);  
                  
        // FIXME: Update energy validty ranges
        result.addChannel(0.5*(dNdE_u+dNdE_d), str_flav_to_mass("u"), str_flav_to_mass("dbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_s), str_flav_to_mass("u"), str_flav_to_mass("sbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_b), str_flav_to_mass("u"), str_flav_to_mass("bbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_d), str_flav_to_mass("ubar"), str_flav_to_mass("d"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_s), str_flav_to_mass("ubar"), str_flav_to_mass("s"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_b), str_flav_to_mass("ubar"), str_flav_to_mass("b"), "gamma", 0., 10000.); 

        result.addChannel(0.5*(dNdE_c+dNdE_d), str_flav_to_mass("c"), str_flav_to_mass("dbar"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_c+dNdE_s), str_flav_to_mass("c"), str_flav_to_mass("sbar"), "gamma", 0., 10000.);   
        result.addChannel(0.5*(dNdE_c+dNdE_b), str_flav_to_mass("c"), str_flav_to_mass("bbar"), "gamma", 0., 10000.);        
        result.addChannel(0.5*(dNdE_c+dNdE_d), str_flav_to_mass("cbar"), str_flav_to_mass("d"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_c+dNdE_s), str_flav_to_mass("cbar"), str_flav_to_mass("s"), "gamma", 0., 10000.);   
        result.addChannel(0.5*(dNdE_c+dNdE_b), str_flav_to_mass("cbar"), str_flav_to_mass("b"), "gamma", 0., 10000.);        

        result.addChannel(0.5*(dNdE_t+dNdE_d), str_flav_to_mass("t"), str_flav_to_mass("dbar"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_t+dNdE_s), str_flav_to_mass("t"), str_flav_to_mass("sbar"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_b), str_flav_to_mass("t"), str_flav_to_mass("bbar"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_d), str_flav_to_mass("tbar"), str_flav_to_mass("d"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_t+dNdE_s), str_flav_to_mass("tbar"), str_flav_to_mass("s"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_b), str_flav_to_mass("tbar"), str_flav_to_mass("b"), "gamma", 0., 10000.);    
   
        initialized = true;
      }
    }

    /// SimYieldTable based on MicrOmegas tabulated results.
    void SimYieldTable_MicrOmegas(SimYieldTable& result)
    {
      using namespace Pipes::SimYieldTable_MicrOmegas;
      using DarkBit_utils::str_flav_to_mass;

      static bool initialized = false;
      const int outN = 0;  // gamma

      if ( not initialized )
      {
        Funk::Funk dNdE;

#define ADD_CHANNEL(inP, P1, P2, FINAL, EcmMin, EcmMax)                                                   \
        dNdE = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("Ecm"), Funk::var("E"), inP, outN)/Funk::var("E"); \
        result.addChannel(dNdE, str_flav_to_mass(P1), str_flav_to_mass(P2), FINAL, EcmMin, EcmMax);  // specifies also center of mass energy range
        // FIXME: Update energy validty ranges
        ADD_CHANNEL(0, "g", "g", "gamma", 0., 10000.)
        ADD_CHANNEL(1, "d", "dbar", "gamma", 0., 10000.)
        ADD_CHANNEL(2, "u", "ubar", "gamma", 0., 10000.)
        ADD_CHANNEL(3, "s", "sbar", "gamma", 0., 10000.)
        ADD_CHANNEL(4, "c", "cbar", "gamma", 0., 10000.)
        ADD_CHANNEL(5, "b", "bbar", "gamma", 0., 10000.)
        ADD_CHANNEL(6, "t", "tbar", "gamma", 0., 10000.)
        ADD_CHANNEL(7, "e+", "e-", "gamma", 0., 10000.)
        ADD_CHANNEL(8, "mu+", "mu-", "gamma", 0., 10000.)
        ADD_CHANNEL(9, "tau+", "tau-", "gamma", 0., 10000.)
        ADD_CHANNEL(10, "Z0", "Z0", "gamma", 0., 10000.)
        ADD_CHANNEL(13, "W+", "W-", "gamma", 0., 10000.)
#undef ADD_CHANNEL
        result.addChannel(
            Funk::zero("Ecm", "E"), "nu_e", "nubar_e", "gamma", 0., 10000.);
        result.addChannel(
            Funk::zero("Ecm", "E"), "nu_mu", "nubar_mu", "gamma", 0., 10000.);
        result.addChannel(
            Funk::zero("Ecm", "E"), "nu_tau", "nubar_tau", "gamma", 0., 10000.);
            
        // Add approximations for single-particle cases.
        // FIXME: Update energy validty ranges
        dNdE = (Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("_Ecm"), Funk::var("E"), 8, outN) 
               /Funk::var("E"))->set("_Ecm", Funk::var("Ecm")*2);
        result.addChannel(dNdE/2, str_flav_to_mass("mu+"), "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("mu-"), "gamma", 0., 10000.);        
        dNdE = (Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("_Ecm"), Funk::var("E"), 9, outN) 
               /Funk::var("E"))->set("_Ecm", Funk::var("Ecm")*2);
        result.addChannel(dNdE/2, str_flav_to_mass("tau+"), "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("tau-"), "gamma", 0., 10000.);        
        dNdE = (Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("_Ecm"), Funk::var("E"), 10, outN) 
               /Funk::var("E"))->set("_Ecm", Funk::var("Ecm")*2);
        result.addChannel(dNdE/2, "Z0", "gamma", 0., 10000.);
        dNdE = (Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("_Ecm"), Funk::var("E"), 13, outN) 
               /Funk::var("E"))->set("_Ecm", Funk::var("Ecm")*2);
        result.addChannel(dNdE/2, "W+", "gamma", 0., 10000.);
        result.addChannel(dNdE/2, "W-", "gamma", 0., 10000.);
        
        // Add single particle lookup for t tbar to prevent them from being tagged as missing final states for cascades.
        dNdE = (Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), Funk::var("_Ecm"), Funk::var("E"), 6, outN) 
               /Funk::var("E"))->set("_Ecm", Funk::var("Ecm")*2);
        result.addChannel(dNdE/2, str_flav_to_mass("t"),    "gamma", 0., 10000.);
        result.addChannel(dNdE/2, str_flav_to_mass("tbar"), "gamma", 0., 10000.);        
        
        // Approximations for mixed quark channels
        Funk::Funk dNdE_d = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 1, outN)/Funk::var("E");
        Funk::Funk dNdE_u = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 2, outN)/Funk::var("E");
        Funk::Funk dNdE_s = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 3, outN)/Funk::var("E");        
        Funk::Funk dNdE_c = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 4, outN)/Funk::var("E");
        Funk::Funk dNdE_b = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 5, outN)/Funk::var("E");          
        Funk::Funk dNdE_t = Funk::func_fromThreadsafe(BEreq::dNdE.pointer(), 
                              Funk::var("Ecm"), Funk::var("E"), 6, outN)/Funk::var("E");
                  
        // FIXME: Update energy validty ranges
        result.addChannel(0.5*(dNdE_u+dNdE_d), str_flav_to_mass("u"), str_flav_to_mass("dbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_s), str_flav_to_mass("u"), str_flav_to_mass("sbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_b), str_flav_to_mass("u"), str_flav_to_mass("bbar"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_d), str_flav_to_mass("ubar"), str_flav_to_mass("d"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_s), str_flav_to_mass("ubar"), str_flav_to_mass("s"), "gamma", 0., 10000.); 
        result.addChannel(0.5*(dNdE_u+dNdE_b), str_flav_to_mass("ubar"), str_flav_to_mass("b"), "gamma", 0., 10000.); 

        result.addChannel(0.5*(dNdE_c+dNdE_d), str_flav_to_mass("c"), str_flav_to_mass("dbar"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_c+dNdE_s), str_flav_to_mass("c"), str_flav_to_mass("sbar"), "gamma", 0., 10000.);   
        result.addChannel(0.5*(dNdE_c+dNdE_b), str_flav_to_mass("c"), str_flav_to_mass("bbar"), "gamma", 0., 10000.);        
        result.addChannel(0.5*(dNdE_c+dNdE_d), str_flav_to_mass("cbar"), str_flav_to_mass("d"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_c+dNdE_s), str_flav_to_mass("cbar"), str_flav_to_mass("s"), "gamma", 0., 10000.);   
        result.addChannel(0.5*(dNdE_c+dNdE_b), str_flav_to_mass("cbar"), str_flav_to_mass("b"), "gamma", 0., 10000.);        

        result.addChannel(0.5*(dNdE_t+dNdE_d), str_flav_to_mass("t"), str_flav_to_mass("dbar"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_t+dNdE_s), str_flav_to_mass("t"), str_flav_to_mass("sbar"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_b), str_flav_to_mass("t"), str_flav_to_mass("bbar"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_d), str_flav_to_mass("tbar"), str_flav_to_mass("d"), "gamma", 0., 10000.);    
        result.addChannel(0.5*(dNdE_t+dNdE_s), str_flav_to_mass("tbar"), str_flav_to_mass("s"), "gamma", 0., 10000.);            
        result.addChannel(0.5*(dNdE_t+dNdE_b), str_flav_to_mass("tbar"), str_flav_to_mass("b"), "gamma", 0., 10000.);             
            
        initialized = true;
      }
    }

    class PPPC_interpolation
    {
      public:
        PPPC_interpolation(std::string filename)
        {
          table = ASCIItableReader(filename);
          std::vector<std::string> colnames = initVector<std::string>(
              "mass", "log10x", "ee", "mumu", "tautau", "qq", "cc", "bb", "tt",
              "WW", "ZZ", "gg", "gammagamma", "hh");
          table.setcolnames(colnames);
          // log10x = log10(E_gamma/m);
          log10x = std::vector<double>(table["log10x"].begin(), table["log10x"].begin()+180);
        }
        PPPC_interpolation() {}  // Dummy initializer

        double operator()(std::string channel, double /*m*/, double /*e*/)
        {
          // FIXME: Write interpolation routine
          std::vector<double> y(table[channel].begin(), table[channel].end());
          return 0;
        }

      private:
        std::vector<double> log10x;
        ASCIItableReader table;
    };

    /// SimYieldTable based on PPPC4DMID Cirelli et al. 2010
    void SimYieldTable_PPPC(SimYieldTable& /*result*/)
    {
      using namespace Pipes::SimYieldTable_PPPC;
      static bool initialized = false;
      static PPPC_interpolation PPPC_gam_object;

      if ( not initialized )
      {
        // FIXME: Implemented PPPC4 tables
        std::string filename = "DarkBit/data/AtProductionNoEW_gammas.dat";
        PPPC_gam_object = PPPC_interpolation(filename);
        initialized = true;
        DarkBit_error().raise(LOCAL_INFO,
            "SimYieldTable_PPPC is not implemented yet.  Use e.g. SimYieldTable_DarkSUSY instead.");
      }
    }
  }
}
