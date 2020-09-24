//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Functions of module SpecBit
///
///  Vacuum stability functions,
///  including interface to VevaciousPlusPlus.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author James McKay
///           (j.mckay14@imperial.ac.uk)
///
///  \date 2015 Nov - 2016 Mar
///
///  \author José Eliel Camargo-Molina
///          (elielcamargomolina@gmail.com)
///  \date Jun 2018++
///
///  \author Sanjay Bloor
///          (sanjay.bloor12@imperial.ac.uk)
///  \date Sep 2019
///
///  \author Janina Renk
///          (janina.renk@fysik.su.se)
///  \date 2019 July, Dec
///
///  *********************************************

#ifdef WITH_MPI
#include "mpi.h"
#endif

#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_min.h>
#include "SLHAea/slhaea.h"

#include "gambit/Elements/gambit_module_headers.hpp"
#include "gambit/Elements/spectrum.hpp"
#include "gambit/Utils/stream_overloads.hpp"
#include "gambit/Utils/util_macros.hpp"
#include "gambit/SpecBit/SpecBit_rollcall.hpp"
#include "gambit/SpecBit/SpecBit_helpers.hpp"
#include "gambit/SpecBit/SpecBit_types.hpp"
#include "gambit/SpecBit/QedQcdWrapper.hpp"

#include "gambit/SpecBit/model_files_and_boxes.hpp" // #includes lots of flexiblesusy headers and defines interface classes

// Flexible SUSY stuff (should not be needed by the rest of gambit)
#include "flexiblesusy/src/ew_input.hpp"
#include "flexiblesusy/src/lowe.h" // From softsusy; used by flexiblesusy
#include "flexiblesusy/src/numerics2.hpp"


// Switch for debug mode
//#define SPECBIT_DEBUG

namespace Gambit
{

  namespace SpecBit
  {

    using namespace LogTags;
    using namespace flexiblesusy;

    void check_EW_stability_ScalarSingletDM_Z3(double &result)
    {
      // check that the electroweak scale stability conditions are satisfied
      namespace myPipe = Pipes::check_EW_stability_ScalarSingletDM_Z3;

      const Spectrum& fullspectrum = *myPipe::Dep::ScalarSingletDM_Z3_spectrum;

      double lambda_h = fullspectrum.get(Par::dimensionless,"lambda_h");
      double lambda_s = fullspectrum.get(Par::dimensionless,"lambda_S");
      double lambda_hs = fullspectrum.get(Par::dimensionless,"lambda_hS");
      double mu3 = fullspectrum.get(Par::mass1,"mu3");
      double ms = fullspectrum.get(Par::Pole_Mass,"S");

      double check = 0;

      if ( (lambda_h*lambda_s > 0 ) )
      {
       check = 2 * pow( lambda_h * lambda_s , 0.5) + lambda_hs;
      }
      double check_2 =  2.*pow(abs(lambda_s),0.5)*ms - mu3;

      result = 0;

      // if any condition not satisfied set bad likelihood and invalidate point
      if ( lambda_hs < 0 || lambda_s < 0 || check < 0 || check_2 < 0)
      {
        invalid_point().raise("Electroweak vacuum is unstable at low scale.");
        result = -1e100;
      }
    }

    bool check_perturb_to_min_lambda(const Spectrum& spec,double scale,int pts
    ,const std::vector<SpectrumParameter> required_parameters)
    {

      std::unique_ptr<SubSpectrum> subspec = spec.clone_HE();
      double step = log10(scale) / pts;
      double runto;

      double ul=3.5449077018110318; // sqrt(4*Pi)
      for (int i=0;i<pts;i++)
      {
        runto = pow(10,step*float(i+1.0)); // scale to run spectrum to
        if (runto<100){runto=200.0;}// avoid running to low scales

          try
          {
            subspec -> RunToScale(runto);
          }
          catch (const Error& error)
          {
            return false;
          }

        for(std::vector<SpectrumParameter>::const_iterator it = required_parameters.begin();
            it != required_parameters.end(); ++it)
        {
          const Par::Tags        tag   = it->tag();
          const std::string      name  = it->name();
          const std::vector<int> shape = it->shape();
          std::ostringstream label;
          label << name <<" "<< Par::toString.at(tag);
          if(shape.size()==1 and shape[0]==1)
          {
            if (abs(subspec->get(tag,name))>ul)
            {
              return false;
            }
          }

          else if(shape.size()==1 and shape[0]>1)
          {
            for(int k = 1; k<=shape[0]; ++k) {
              if (abs(subspec->get(tag,name,k))>ul)
              {
                return false;
              }

            }
          }
          else if(shape.size()==2)
          {
            for(int k = 1; k<=shape[0]; ++k) {
              for(int j = 1; j<=shape[0]; ++j) {
                if (abs(subspec->get(tag,name,k,j))>ul)
                {
                  return false;
                }
              }
            }
          }
        }
      }

      return true;
    }

    double run_lambda(double scale ,  void *params)
    {

      std::unique_ptr<SubSpectrum>* spec=(std::unique_ptr<SubSpectrum>* )params;
      SubSpectrum& speccloned = **spec;

      // clone the original spectrum incase the running takes the spectrum
      // into a non-perturbative scale and thus the spectrum is no longer reliable
      std::unique_ptr<SubSpectrum> speccloned2 =  speccloned.clone();

      if (scale>1.0e21){scale=1.0e21;}// avoid running to high scales

      if (scale<100.0){scale=100.0;}// avoid running to very low scales

      double lambda;
      try
      {
        speccloned2->RunToScale(scale);
        lambda = speccloned2->get(Par::dimensionless,"lambda_h");
      }
      catch (const Error& error)
      {
        //   ERROR(error.what());
        //cout << "error encountered" << endl;
        lambda = 0;
        // return EXIT_FAILURE;
      }

      return lambda;
    }


     void find_min_lambda_Helper(dbl_dbl_bool& vs_tuple, const Spectrum& fullspectrum,
                                 double high_energy_limit, int check_perturb_pts,
                                 const std::vector<SpectrumParameter> required_parameters)
     {
             std::unique_ptr<SubSpectrum> speccloned = fullspectrum.clone_HE();

      // three scales at which we choose to run the quartic coupling up to, and then use a Lagrange interpolating polynomial
      // to get an estimate for the location of the minimum, this is an efficient way to narrow down over a huge energy range
      double u_1 = 1, u_2 = 5, u_3 = 12;
      double lambda_1,lambda_2,lambda_3;
      double lambda_min = 0;


      bool min_exists = 1;// check if gradient is positive at electroweak scale
      if ( run_lambda(101.0, &speccloned ) > run_lambda(100.0, &speccloned ) )
      {
        // gradient is positive, the minimum is less than electroweak scale so
        // lambda_h must be monotonally increasing
        min_exists = 0;
        lambda_min = run_lambda(100.0,&speccloned);
      }

      double mu_min = 0;
      if (min_exists)
      {

        // fit parabola (in log space) to the 3 trial points and use this to estimate the minimum

        for (int i=1;i<2;i++)
        {

          lambda_1 = run_lambda(pow(10,u_1),&speccloned);
          lambda_2 = run_lambda(pow(10,u_2),&speccloned);
          lambda_3 = run_lambda(pow(10,u_3),&speccloned);

          double min_u= (lambda_1*(pow(u_2,2)-pow(u_3,2))  - lambda_2*(pow(u_1,2)-pow(u_3,2)) + lambda_3*(pow(u_1,2)-pow(u_2,2)));
          double denominator = ( lambda_1*(u_2-u_3)+ lambda_2*(u_3-u_1)  +lambda_3*(u_1-u_2));

          min_u=0.5*(min_u/denominator);
          u_1=min_u-2/(pow(float(i),0.01));
          u_2=min_u;
          u_3=min_u+2/(pow(float(i),0.01));

        }

        // run downhill minimization routine to find exact minimum

        double mu_lower = pow(10,u_1);
        double mu_upper = pow(10,u_3);
        mu_min = pow(10,u_2);

        gsl_function F;
        F.function = &run_lambda;
        F.params = &speccloned;

        int status;
        int iteration = 0, max_iteration = 1000;

        const gsl_min_fminimizer_type *T;
        gsl_min_fminimizer *s;

        T = gsl_min_fminimizer_brent;
        s = gsl_min_fminimizer_alloc (T);
        gsl_min_fminimizer_set (s, &F, mu_min, mu_lower, mu_upper);

        do
        {
          iteration++;
          status = gsl_min_fminimizer_iterate (s);

          mu_min = gsl_min_fminimizer_x_minimum (s);
          mu_lower = gsl_min_fminimizer_x_lower (s);
          mu_upper = gsl_min_fminimizer_x_upper (s);

          status = gsl_min_test_interval (mu_lower, mu_upper, 0.0001, 0.0001);
          //  cout << "mu_lower = " << mu_lower << " mu_upper = " << mu_upper << endl;
        }
        while (status == GSL_CONTINUE && iteration < max_iteration);

        if (iteration == max_iteration)
        {
          SpecBit_error().raise(LOCAL_INFO,"The minimum of the quartic coupling could not be found");
        }

        gsl_min_fminimizer_free (s);

        lambda_min = run_lambda(mu_min,&speccloned);

      }

      #ifdef SPECBIT_DEBUG
        std::cout<< "minimum value of quartic coupling is   "<< lambda_min << " at " << mu_min <<" GeV"<<std::endl;
      #endif

      double lifetime,LB;
      if (lambda_min<0) // second minimum exists, now determine stability and lifetime
      {
        LB=mu_min;
        #ifdef SPECBIT_DEBUG
          double p=exp(4*140-26/(abs(0.5*lambda_min)))*pow(LB/(1.2e19),4); // compute tunnelling rate
          if (p>1)
          {
            cout<< "vacuum is unstable" << endl;
          }
          else
          {
            cout<< "vacuum is metastable" << endl;
          }
        #endif
        double pi2_8_over3 = 8.* pow ( pi , 2 ) / 3.;
        double conversion = (6.5821195e-25)/(31536000);
        // top factor is hbar in units of GeV.s and bottom factor is number of seconds in a year
        lifetime=conversion/(exp(3*140-pi2_8_over3/(abs(0.5*lambda_min)))*pow(1/(1.2e19),3)*pow(LB,4));

      }
      else // quartic coupling always positive, set output to default values
      {
        LB=high_energy_limit;
        lifetime=1e300;
        #ifdef SPECBIT_DEBUG
          cout << "vacuum is absolutely stable" << endl;
        #endif
        // vacuum is stable
      }
      // now do a check on the perturbativity of the couplings up to this scale
      bool perturbative=check_perturb_to_min_lambda(fullspectrum,LB,check_perturb_pts,required_parameters);
      double perturb=float(!perturbative);
      #ifdef SPECBIT_DEBUG
        cout << "perturbativity checked up to " << LB << " result = " << perturbative << endl;
        cout << "Higgs pole mass = " << fullspectrum.get(Par::Pole_Mass, "h0_1") << endl;
      #endif
      vs_tuple = dbl_dbl_bool(lifetime,LB,perturb);
    }


    void find_min_lambda_ScalarSingletDM_Z2(dbl_dbl_bool& vs_tuple)
    {
      namespace myPipe = Pipes::find_min_lambda_ScalarSingletDM_Z2;
      double high_energy_limit = myPipe::runOptions->getValueOrDef<double>(1.22e19,"set_high_scale");
      int check_perturb_pts = myPipe::runOptions->getValueOrDef<double>(10,"check_perturb_pts");
      static const SpectrumContents::ScalarSingletDM_Z2 contents;
      static const std::vector<SpectrumParameter> required_parameters = contents.all_parameters_with_tag(Par::dimensionless);
      find_min_lambda_Helper(vs_tuple, *myPipe::Dep::ScalarSingletDM_Z2_spectrum, high_energy_limit, check_perturb_pts, required_parameters);
    }

    void find_min_lambda_ScalarSingletDM_Z3(dbl_dbl_bool& vs_tuple)
    {
      namespace myPipe = Pipes::find_min_lambda_ScalarSingletDM_Z3;
      double high_energy_limit = myPipe::runOptions->getValueOrDef<double>(1.22e19,"set_high_scale");
      int check_perturb_pts = myPipe::runOptions->getValueOrDef<double>(10,"check_perturb_pts");
      static const SpectrumContents::ScalarSingletDM_Z2 contents;
      static const std::vector<SpectrumParameter> required_parameters = contents.all_parameters_with_tag(Par::dimensionless);
      find_min_lambda_Helper(vs_tuple, *myPipe::Dep::ScalarSingletDM_Z3_spectrum, high_energy_limit, check_perturb_pts, required_parameters);
    }

    void find_min_lambda_MDM(dbl_dbl_bool& vs_tuple)
    {
      namespace myPipe = Pipes::find_min_lambda_MDM;
      double high_energy_limit = myPipe::runOptions->getValueOrDef<double>(1.22e19,"set_high_scale");
      int check_perturb_pts = myPipe::runOptions->getValueOrDef<double>(10,"check_perturb_pts");
      static const SpectrumContents::MDM contents;
      static const std::vector<SpectrumParameter> required_parameters = contents.all_parameters_with_tag(Par::dimensionless);
      find_min_lambda_Helper(vs_tuple, *myPipe::Dep::MDM_spectrum, high_energy_limit, check_perturb_pts, required_parameters);
    }


    // the functions below are used to extract the desired outputs from find_min_lambda

    // gives expected lifetime in units of years, if stable give extremly large number (1e300)
    void get_expected_vacuum_lifetime(double &lifetime)
    {
      namespace myPipe = Pipes::get_expected_vacuum_lifetime;
      dbl_dbl_bool vs_tuple =  *myPipe::Dep::high_scale_vacuum_info;

      if (vs_tuple.first<1e300)
      {
        lifetime=vs_tuple.first;
      }
      else
      {
        lifetime=1e300;
      }
    }

    // log of the likelihood
    void lnL_highscale_vacuum_decay_single_field(double &result)
    {
      namespace myPipe = Pipes::lnL_highscale_vacuum_decay_single_field;
      dbl_dbl_bool vs_tuple =  *myPipe::Dep::high_scale_vacuum_info;

      const Options& runOptions=*myPipe::runOptions;
      bool demand_stable = runOptions.getValueOrDef<bool>(false,"demand_stable");
      double stability_scale = runOptions.getValueOrDef<double>(1.22e19,"set_stability_scale");

      if (demand_stable && (vs_tuple.second < stability_scale))
      {
                result = -1e100;
            }
            else
            {
                double conversion = (6.5821195e-25)/(31536000);
                result=((- ( 1 / ( vs_tuple.first/conversion ) ) * exp(140) * (1/ (1.2e19) ) )  );
            }

    }

    // Get the scale of the high-scale minimum
    void get_lambdaB(double &result)
    {
      namespace myPipe = Pipes::get_lambdaB;
      dbl_dbl_bool vs_tuple =  *myPipe::Dep::high_scale_vacuum_info;
      result=vs_tuple.second;
    }


    // Returns poor likelihood and invalidates point if couplings go non-perturbative at or below the scale of the high-scale minimum of the potential
    void check_perturb_min_lambda(double &result)
    {
      namespace myPipe = Pipes::check_perturb_min_lambda;
      dbl_dbl_bool vs_tuple =  *myPipe::Dep::high_scale_vacuum_info;

      if (vs_tuple.flag)
      {
        invalid_point().raise("Couplings are non-perturbative before scale of vacuum instability");
        result = -1e100;
      }
      else
      {
        result = 0;
      }
    }

    /******************************************/
    /* Vacuum stability likelihoods & results */
    /******************************************/

    /// Vacuum stability likelihood from a Vevacious run
    /// calculating the lifetime of & tunneling probability to the
    /// vacuua
    void get_likelihood_VS(double &result)
    {
        using namespace Pipes::get_likelihood_VS;
        
        // (JR) what is here is just copied and pasted from the global one.. once the 
        // other stuff with choosing minima works, this has to add up the results correctly
        // #todo
        VevaciousResultContainer vevacious_results = *Dep::check_vacuum_stability;
        double lifetime =  vevacious_results.get_lifetime("nearest");

        // This is based on the estimation of the past lightcone from 1806.11281
        double conversion = (6.5821195e-25)/(31536000);
        result=((- ( 1 / ( lifetime/conversion ) ) * exp(140) * (1/ (1.2e19) ) )  );
    }

    /// Vacuum stability likelihood from a Vevacious run
    /// calculating the lifetime of & tunneling probability to the global minimum
    void get_likelihood_VS_global(double &result)
    {
        using namespace Pipes::get_likelihood_VS_global;
        
        VevaciousResultContainer vevacious_results = *Dep::check_vacuum_stability_global;
        double lifetime =  vevacious_results.get_lifetime("global");

        // This is based on the estimation of the past lightcone from 1806.11281
        double conversion = (6.5821195e-25)/(31536000);
        result=((- ( 1 / ( lifetime/conversion ) ) * exp(140) * (1/ (1.2e19) ) )  );
    }

    /// Vacuum stability likelihood from a Vevacious run
    /// calculating the lifetime of & tunneling probability to the nearest minimum
    void get_likelihood_VS_nearest(double &result)
    {
        using namespace Pipes::get_likelihood_VS_nearest;
        
        VevaciousResultContainer vevacious_results = *Dep::check_vacuum_stability_nearest;
        double lifetime =  vevacious_results.get_lifetime("nearest");

        // This is based on the estimation of the past lightcone from 1806.11281
        double conversion = (6.5821195e-25)/(31536000);
        result=((- ( 1 / ( lifetime/conversion ) ) * exp(140) * (1/ (1.2e19) ) )  );
    }

    /// get all results from VS as str to dbl map to easily print them
    /// (global vacuum is panic)
    void get_VS_results_global(map_str_dbl &result)
    {
        using namespace Pipes::get_VS_results_global;
        
        VevaciousResultContainer vevacious_results = *Dep::check_vacuum_stability_global;
        result =  vevacious_results.get_global_results();
    }

    /// get all results from VS as str to dbl map to easily print them
    /// (nearest vacuum is panic)
    void get_VS_results_nearest(map_str_dbl &result)
    {
        using namespace Pipes::get_VS_results_nearest;
        
        VevaciousResultContainer vevacious_results = *Dep::check_vacuum_stability_nearest;
        result =  vevacious_results.get_nearest_results();
    }
    

    /**********************/
    /* VEVACIOUS ROUTINES */
    /**********************/
    
    /// Declaration of helper function for making vevaciousPlusPlus
    /// inputs.
    void make_vpp_inputs(map_str_str&);

    /// Create a string set containing a list with all likelihoods that vevacious
    /// should calculate. The options are tunneling to
    /// - the global minimum -> "global" (quantum tunneling)
    /// - the global minimum -> "global_thermal" (thermal tunneling)
    /// - the nearest minimum -> "nearest" (quantum tunneling)
    /// - the global minimum -> "nearest_thermal" (thermal tunneling)
    /// Default behaviour (if no sub-capabilities are set): calculate all of them
    void set_panic_vacua(std::set<std::string> & result)
    {
      using namespace Pipes::set_panic_vacua;

      static bool firstrun = true;

      if(firstrun)
      {
        // Get the list of likelihoods given in the YAML file as sub-capabilities.
        std::vector<str> subcaps = Downstream::subcaps->getNames();
        
        // if no sub-capabilities are set, fix default behaviour: calculate tunneling to global and 
        // nearest minimum for zero and non-zero temperature
        std::set<std::string> default_choice = {"global","nearest","global_thermal","nearest_thermal"};
        
        // read in yaml options or ..
        if(not subcaps.empty()) for (const auto& x : subcaps) {result.insert(x);}
        // .. use default behaviour
        else {result = default_choice;}

        // add info which contributions will be added up to logger
        std::ostringstream ss;
        ss << "The LogLike calculation 'VS_likelihood' will add up the contributions" << endl;
        for (const auto& x : result) ss << "- "<< x << endl;
        ss << "from the tunneling calculations. " << endl
           << "You can change this in the ObsLikes section of your YAML file," << endl
           << "by setting sub_capabilities 'VS_likelihood', e.g." << endl
           << "    sub_capabilities:" << endl
           << "      - nearest" << endl
           << "      - nearest_thermal" << endl
           << "to only calculate the tunneling probabilities to the nearest minimum." << endl;
        logger() << ss.str() << EOM;
      
        // just need to set this once
        firstrun = false;   
      }
    }

    /// Set tunnelling strategy for the different minima, either
    /// - JustQuantum -> only quantum
    /// - JustThermal -> only thermal or
    /// - QuantumThenThermal -> both
    /// depending on what needs to be calculated 
    map_str_str helper_set_tunnelingStragey(std::set<std::string> panic_vacua)
    {

      map_str_str result;

      // quantum and thermal tunnelling for global
      if(panic_vacua.find("global") != panic_vacua.end() and panic_vacua.find("global_thermal") != panic_vacua.end())
      {
        result["global"] = "QuantumThenThermal";
      }
      // only thermal requested 
      else if(panic_vacua.find("global_thermal") != panic_vacua.end())
      {
        result["global"] = "JustThermal";
      }
      // only quantum requested 
      else if(panic_vacua.find("global") != panic_vacua.end())
      {
        result["global"] = "JustQuantum";
      }
      
      // quantum and thermal tunnelling for nearest
      if(panic_vacua.find("nearest") != panic_vacua.end() and panic_vacua.find("nearest_thermal") != panic_vacua.end())
      {
        result["nearest"] = "QuantumThenThermal";
      }
      // only thermal requested 
      else if(panic_vacua.find("nearest_thermal") != panic_vacua.end())
      {
        result["nearest"] = "JustThermal";
      }
      // only quantum requested 
      else if(panic_vacua.find("nearest") != panic_vacua.end())
      {
        result["nearest"] = "JustQuantum";
      }

      return result;
    }

    
    /// Parses the YAML file for any settings, then passes to make_vpp_inputs to create
    /// .xml files for vevacious to run with.
    void initialize_vevacious(std::string &inputspath)
    {
        namespace myPipe = Pipes::initialize_vevacious;
        const Options& runOptions = *myPipe::runOptions;

        static bool firstrun = true;

        if (firstrun)
        {
            // Create a map of opts to pass to the helper function
            map_str_str opts;

            // Generating a Random seed from Gambit random generator
            std::string randomseed_gen = std::to_string(int(Random::draw() * 2 * 1987.));
            
            opts["phc_random_seed"] =               runOptions.getValueOrDef<std::string>(randomseed_gen, "phc_random_seed");
            opts["MinuitStrategy"] =                runOptions.getValueOrDef<std::string>("0", "minuit_strategy");
            opts["PotentialFunctionClassType"] =    runOptions.getValueOrDef<std::string>("FixedScaleOneLoopPotential", "potential_type");
            opts["homotopybackend"] =               runOptions.getValueOrDef<std::string>("hom4ps", "homotopy_backend");
            opts["globalIsPanic"] =                 runOptions.getValueOrDef<std::string>("false", "global_minimum_is_panic");
            // (JR) this is fixed here for all vevacious runs 
            // if we want to be able to set this differently for global and nearest, we have 
            // to think about how to pass it to vev
            // -> probably best like "global" and "nearest" when calling 
            // vevaciousPlusPlus.RunPoint(panic_vacuum, tunnelingStrategy)
            // # todo
            opts["TunnelingStrategy"] =             runOptions.getValueOrDef<std::string>("JustQuantum", "tunneling_strategy");
            opts["pathFindingTimeout"] =            runOptions.getValueOrDef<std::string>("3600", "path_finding_timeout");
            opts["SurvivalProbabilityThreshold"] =  runOptions.getValueOrDef<std::string>("0.01", "survival_probability_threshold");
            opts["radialResolution"] =              runOptions.getValueOrDef<std::string>("0.1", "radial_resolution_undershoot_overshoot");
            opts["PathResolution"] =                runOptions.getValueOrDef<std::string>("1000", "PathResolution");
            
            // Insert the file location info to the options map 
            map_str_str file_locations = *myPipe::Dep::vevacious_file_location;
            opts.insert(file_locations.begin(), file_locations.end());

            // Pass to the helper function and make some vevacious input.
            make_vpp_inputs(opts);

            inputspath = opts["inputspath"];

            firstrun = false;
        }
        // Done.
    }

    /// Execute the passing of the spectrum object (as SLHAea) to vevacious. It is a helper function and not a 
    /// capability since this has to be executed before every single vevacious run. vevacious can run multiple times for 
    /// a single point in parameter space depending on settings: 
    ///   -> global and/or nearest minimum for tunneling requested? 
    ///   -> multiple attempts for one vevacious run allowed?
    vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus exec_pass_spectrum_to_vevacious(SpectrumEntriesForVevacious &pass_spectrum )
    {
        
        // get inputFilename and initialise vevaciousPlusPlus object with it
        static std::string inputFilename = pass_spectrum.get_inputFilename();
        vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus vevaciousPlusPlus(inputFilename);

        // get scale and the map containing all spectrum entries that need to be passed to vevacious
        double scale = pass_spectrum.get_scale();
        map_str_SpectrumEntry spec_entry_map = pass_spectrum.get_spec_entry_map();

        // iterate through map and call vevacious' 'ReadLhaBlock' to read spectrum entries
        for (auto it=spec_entry_map.begin(); it!=spec_entry_map.end(); ++it) 
        {
          SpectrumEntry entry = it->second;
          logger() << LogTags::debug << "Passing ReadLhaBlock option  "<< entry.name  << " scale " << scale << " parameters" << entry.parameters <<" and dimension " << entry.dimension << " to vevacious" << EOM;
          vevaciousPlusPlus.ReadLhaBlock(entry.name, scale , entry.parameters, entry.dimension );
        }  

        return vevaciousPlusPlus;
      }

    /// Call vevacious, the result is either "Stable", "Metastable" or "Inconclusive" in case
    /// a vevacious run failed for some reason
    void helper_run_vevacious(vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus &vevaciousPlusPlus,VevaciousResultContainer& result, std::string panic_vacuum, std::string inputPath)
    {
       
        double lifetime, thermalProbability;

        //Checking if file exists, fastest method.
        struct stat buffer; 
        std::string HomotopyLockfile = inputPath + "/Homotopy/busy.lock";
        // Check if homotopy binary is being used
        //Here I check it the busy.lock file exists and if it does I go into a
        // while loop that either breaks when the file is deleted or after
        // 30 seconds have passed.
        // This deals with the problem of MARCONI not liking a binary accessed by too many
        // processes at the same time
        std::chrono::system_clock::time_point tStart = Utils::get_clock_now();
              
        while(stat(HomotopyLockfile.c_str(), &buffer)==0)
        {
          std::chrono::system_clock::time_point tNow = Utils::get_clock_now();
          std::chrono::seconds tSofar = std::chrono::duration_cast<std::chrono::seconds>(tNow - tStart);
          if(tSofar >= std::chrono::seconds(30) )
          {
            remove( HomotopyLockfile.c_str());
            break; 
          }
        }

        // call vevacious and pass the setting for which vacuum is supposed
        // to be used for tunneling calculation as string
        vevaciousPlusPlus.RunPoint(panic_vacuum);

        // get vevacious results
        lifetime = vevaciousPlusPlus.GetLifetimeInSeconds();
        thermalProbability = vevaciousPlusPlus.GetThermalProbability();

        // decide how to deal with different vevacious outcomes
        // here -1 from Vevacious means that the point is stable. 
        if(lifetime == -1 && thermalProbability == -1 )
        { 
            lifetime = 3.0E+100;
            thermalProbability = 1;
        }
        else if(lifetime == -1 && thermalProbability != -1)
        {
            lifetime = 3.0E+100;
        }
        else if(lifetime != -1 && thermalProbability == -1)
        {
            thermalProbability = 1;
        }

        cout << "VEVACIOUS LIFETIME:  "<< lifetime << endl;
        cout << "VEVACIOUS Prob. non zero temp:  "<< thermalProbability << endl;
        std::string vevacious_result = vevaciousPlusPlus.GetResultsAsString();
        cout << "VEVACIOUS RESULT:  "<< vevacious_result << endl;
    
        // return a vector containing the results from vevacious, these are filled
        // in any successfully run case with the entries
        //   BounceActionsThermal = ["Bounce Action Threshold", "straight path bounce action", 
        //              "best result from path_finder 1", "best result from path_finder 2",...]
        //   Note that the entries "best result from path_finder x" are only filled if the
        //               "straight path bounce action" (entry 1) is higher than the "bounce Action Threshold"
        //                (entry 0). The vector length depends on how many different path finders are implemented in
        //                vevacious
        std::vector<double> BounceActionsThermal_vec = vevaciousPlusPlus.GetThermalThresholdAndActions();
        std::vector<double> BounceActions_vec = vevaciousPlusPlus.GetThresholdAndActions();
            
        logger() << LogTags::debug  << "VEVACIOUS result size "<< BounceActions_vec.size();
        logger() << LogTags::debug  << "\nVEVACIOUS thermal result size "<< BounceActionsThermal_vec.size();
        std::cout  << "VEVACIOUS thermal result size "<< BounceActionsThermal_vec.size() << std::endl;

        // set bounce actions values & the threshold if they were calculated
        for(std::size_t ii=0; ii<BounceActions_vec.size(); ++ii) 
        {
          logger() << LogTags::debug << "\nSetting bounceActionThreshold_[" << ii << "]"<< BounceActions_vec.at(ii); 
          result.set_results(panic_vacuum, "bounceActionThreshold_[" + std::to_string(ii) + "]", BounceActions_vec.at(ii));
        }

        // set thermal bounce actions values & the threshold if they were calculated
        for(std::size_t ii=0; ii<BounceActionsThermal_vec.size(); ++ii) 
        {
          logger() << LogTags::debug << "\nSetting bounceActionThresholdThermal_[" << ii << "]"<< BounceActionsThermal_vec.at(ii); 
          std::cout << "Setting bounceActionThresholdThermal_[" << ii << "]"<< BounceActionsThermal_vec.at(ii) << std::endl; 
          result.set_results(panic_vacuum, "bounceActionThresholdThermal_[" + std::to_string(ii) + "]", BounceActionsThermal_vec.at(ii));
        }

        logger() << LogTags::debug << EOM;

        // add entry 'straightPathGoodEnough' to result map
        // -> -1 if no bounce actions calculated
        // ->  1 if threshold > straightPath
        // ->  0 else
        // Note that this has to be done after the bounceActionThreshold results are set
        result.add_straightPathGoodEnough(panic_vacuum);

        // set lifetime & thermal probability
        result.set_results(panic_vacuum,"lifetime", lifetime);
        result.set_results(panic_vacuum,"thermalProbability", thermalProbability);
    }


    /// Decide how to deal with a failed vevacious run --> set lifetime and thermalProbability
    /// conservatively to a value easy to identify in analysis
    void helper_catch_vevacious(VevaciousResultContainer& result, std::string panic_vacuum)
    {

      //Vevacious has crashed -- set results to fixed values
      double lifetime = 2.0E+100; 
      double thermalProbability= 1;

      cout << "VEVACIOUS LIFETIME:  "<< lifetime << endl;
      std::string vevacious_result = "Inconclusive";
      cout << "VEVACIOUS RESULT:  "<< vevacious_result << endl;
      logger() << "Vevacious could not calculate lifetime. Conservatively setting it to large value."<<endl;
      
      result.set_results(panic_vacuum,"lifetime", lifetime);
      result.set_results(panic_vacuum,"thermalProbability", thermalProbability);

    }

    /// If tunnelling to global and nearest vacuum are requested, this
    /// capability compares if the two vacua are the same. 
    /// Return true if they coincide, false if not.
    void compare_panic_vacua(map_str_str &result)
    {
      using namespace Pipes::compare_panic_vacua;

      // get the object that holds all inputs that needs to be passed to vevacious
      SpectrumEntriesForVevacious pass_spectrum = (*Dep::pass_spectrum_to_vevacious);
      // create vev object to check if global and near panic vacuum are the same
      vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus vevaciousPlusPlus = exec_pass_spectrum_to_vevacious(pass_spectrum);
      
      // get set with contributions to likelihood that should be calculated
      std::set<std::string> panic_vacua = *Dep::panic_vacua;

      // Check if global and nearest are requested, if so test if the happen to be 
      // the same minimum for this point in parameters space.
      // If that is the case, remove the global points from the 
      if( (panic_vacua.find("global") != panic_vacua.end() or panic_vacua.find("global_thermal") != panic_vacua.end())
        and (panic_vacua.find("nearest") != panic_vacua.end() or panic_vacua.find("nearest_thermal") != panic_vacua.end()) )
      {
        // get vector pair with VEVs of global and nearest vacua
        std::pair<std::vector<double>,std::vector<double>> vevs = vevaciousPlusPlus.RunVacua("Internal");
        std::vector<double> global = vevs.first;
        std::vector<double> nearest = vevs.second;

        // check if vectors are equal
        bool compare = std::equal(global.begin(), global.end(), nearest.begin());

        // if the minima are the same, remove global entries if the corresponding nearest
        // entry is contained
        if(compare)
        {
          if (panic_vacua.find("global") != panic_vacua.end() and panic_vacua.find("nearest") != panic_vacua.end())
          {
            panic_vacua.erase("global");
          }
          if (panic_vacua.find("global_thermal") != panic_vacua.end() and panic_vacua.find("nearest_thermal") != panic_vacua.end())
          {
            panic_vacua.erase("global_thermal");
          }
          // add info to logger
          std::ostringstream ss;
          ss << "Global and nearest minimum are the same. Will only calculate tunnelling" << endl;
          ss << "probability once." << endl;
          logger() << ss.str() << EOM;
        }
        else
        {
          // add info to debug logger
          std::ostringstream ss;
          ss << "Global and nearest minimum are not the same. Will calculate tunnelling" << endl;
          ss << "probability to both." << endl;
          logger() << LogTags::debug << ss.str() << EOM;
        }

      }
      // set tunnelling strategy for the minima: either 
      // - JustQuantum -> only quantum
      // - JustThermal -> only thermal
      // - QuantumThenThermal -> both
      result = helper_set_tunnelingStragey(panic_vacua);            

    }


    /// Check stability of global vacuum of the potential with vevacious
    void check_vacuum_stability_vevacious(VevaciousResultContainer &result)
    {
        using namespace Pipes::check_vacuum_stability_vevacious;

        // get a str-str map with valculations vevacious has to execute for this parameter point
        // note: this has to be executed for each point as it can happen that the nearest and the 
        // global panic vaccum are the same. 
        map_str_str panic_vacua = *Dep::compare_panic_vacua;

        // This is the number of different pathFinders implemented in vevacious. It should be 
        // returned by a BE convenience function, I tried to do it but didn't manage to 
        // with the BOSS stuff -- that should probably be fixed  # todo 
        static int pathFinder_number = 2;
              
        // get the object that holds all inputs that needs to be passed to vevacious
        SpectrumEntriesForVevacious pass_spectrum = (*Dep::pass_spectrum_to_vevacious);
        
        // run vevacious for different panic vacua, respecting the choices for thermal 
        // or only quantum tunnelling calculations
        for(auto x : panic_vacua)
        { 
          // set panic vacuum and tunnelling strategy for vevacious run  
          std::string panic_vacuum = x.first;
          std::string tunnellingStrategy = x.second;

          // reset all entries of the of VevaciousResultContainer map holding the results
          // to avoid that any value could be carried over from a previous calculated point
          result.clear_results(panic_vacuum, pathFinder_number);
          try 
          {    
              // create vevaciousPlusPlus object new for every try since spectrum 
              // vevacious deletes spectrum after each run => to be able to do this 
              // we need the non-rollcalled helper function that gets executed every time 
              // we get to this line (unlike a dependency)
              vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus vevaciousPlusPlus = exec_pass_spectrum_to_vevacious(pass_spectrum);
              
              // helper function to set lifetime & thermal probability accordingly 
              // (this is a separate function s.t. we don't have to reproduce code 
              // for different panic vacua)
              helper_run_vevacious(vevaciousPlusPlus, result, panic_vacuum, pass_spectrum.get_inputPath());
          }
          catch(const std::exception& e)
          {
              // call function that sets the lifetime & thermal probability if vevacious
              // has crashed for some reason
              helper_catch_vevacious(result, panic_vacuum);
              logger() << "Error occurred: " << e.what() << EOM;
          }
        }
    }

    /// Check stability of global vacuum of the potential with vevacious
    void check_vacuum_stability_vevacious_global(VevaciousResultContainer &result)
    {
        namespace myPipe = Pipes::check_vacuum_stability_vevacious_global;

        // global vacuum is panic in this capability
        static const std::string panic_vacuum = "global";
        // This is the number of different pathFinders implemented in vevacious. It should be 
        // returned by a BE convenience function, I tried to do it but didn't manage to 
        // with the BOSS stuff -- that should probably be fixed  # todo 
        static int pathFinder_number = 2;
        // reset all entries of the of VevaciousResultContainer map holding the results
        // to avoid that any value could be carried over from a previous calculated point
        result.clear_results(panic_vacuum, pathFinder_number);
        
        // read in option how often to re-run vevacious in case of an INCONCLUSIVE RESULT
        static int maxTrials = myPipe::runOptions->getValueOrDef<int>(2,"max_run_trials");
        static bool firstrun = true;
        if(firstrun){std::cout << " ... running vevacious at max. " << maxTrials << " times if results are inconclusive." << std::endl;firstrun=false;}

        int trial = 0;
        bool successful_run = false;
        
        // get the object that holds all inputs that needs to be passed to vevacious
        SpectrumEntriesForVevacious pass_spectrum = (*myPipe::Dep::pass_spectrum_to_vevacious);
        
        // run vevacious until it runs successfully or the maximum number of trials is reached
        while(trial < maxTrials and not successful_run)
        { 
          try 
          {    
              // create vevaciousPlusPlus object new for every try since spectrum 
              // vevacious deletes spectrum after each run => to be able to do this 
              // we need the non-rollcalled helper function that gets executed every time 
              // we get to this line (unlike a dependency)
              vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus vevaciousPlusPlus = exec_pass_spectrum_to_vevacious(pass_spectrum);
              
              // helper function to set lifetime & thermal probability accordingly 
              // (this is a separate function s.t. we don't have to reproduce code 
              // for different panic vacua)
              helper_run_vevacious(vevaciousPlusPlus, result, panic_vacuum, pass_spectrum.get_inputPath());

              // congrats -- if you got up to this point vevacious did not crash!
              successful_run = true;
          }

          catch(const std::exception& e)
          {
              // call function that sets the lifetime & thermal probability if vevacious
              // has crashed for some reason
              helper_catch_vevacious(result, panic_vacuum);
              logger() << "Error occurred: " << e.what() << EOM;
          }
          // and here we go for the next try -- fingers crossed!
          trial += 1;
        }
    }

    /// Check stability of global vacuum of the potential with vevacious
    void check_vacuum_stability_vevacious_nearest(VevaciousResultContainer &result)
    {
        namespace myPipe = Pipes::check_vacuum_stability_vevacious_nearest;

        // nearest vacuum is panic in this capability
        static const std::string panic_vacuum = "nearest";
        // This is the number of different pathFinders implemented in vevacious. It should be 
        // returned by a BE convenience function, I tried to do it but didn't manage to 
        // with the BOSS stuff -- that should probably be fixed  # todo 
        static int pathFinder_number = 2;
        // reset all entries of the of VevaciousResultContainer map holding the results
        // to avoid that any value could be carried over from a previous calculated point
        result.clear_results(panic_vacuum, pathFinder_number);

        // read in option how often to re-run vevacious in case of an INCONCLUSIVE RESULT
        static int maxTrials = myPipe::runOptions->getValueOrDef<int>(2,"max_run_trials");
        static bool firstrun = true;
        if(firstrun){std::cout << " ... running vevacious at max. " << maxTrials << " times if results are inconclusive." << std::endl;firstrun=false;}

        int trial = 0;
        bool successful_run = false;
        
        // get the object that holds all inputs that needs to be passed to vevacious
        SpectrumEntriesForVevacious pass_spectrum = (*myPipe::Dep::pass_spectrum_to_vevacious);
        
        // run vevacious until it runs successfully or the maximum number of trials is reached
        while(trial < maxTrials and not successful_run)
        { 
          try 
          {    
              // create vevaciousPlusPlus object new for every try since spectrum 
              // vevacious deletes spectrum after each run => to be able to do this 
              // we need the non-rollcalled helper function that gets executed every time 
              // we get to this line (unlike a dependency)
              vevacious_1_0::VevaciousPlusPlus::VevaciousPlusPlus vevaciousPlusPlus = exec_pass_spectrum_to_vevacious(pass_spectrum);
              
              // helper function to set lifetime & thermal probability accordingly 
              // (this is a separate function s.t. we don't have to reproduce code 
              // for different panic vacua)
              helper_run_vevacious(vevaciousPlusPlus, result, panic_vacuum, pass_spectrum.get_inputPath());

              // congrats -- if you got up to this point vevacious did not crash!
              successful_run = true;
          }

          catch(const std::exception& e)
          {
              // call function that sets the lifetime & thermal probability if vevacious
              // has crashed for some reason
              helper_catch_vevacious(result, panic_vacuum);
              logger() << "Error occurred: " << e.what() << EOM;
          }
          // and here we go for the next try -- fingers crossed!
          trial += 1;
        }
    }
   
    
    /********/
    /* MSSM */
    /********/

    /// Tell GAMBIT which files to work with for the MSSM.
    void vevacious_file_location_MSSM(map_str_str &result)
    {
        namespace myPipe = Pipes::vevacious_file_location_MSSM;
        const Options& runOptions = *myPipe::runOptions;

        int rank;

        // Get mpi rank
        #ifdef WITH_MPI
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        #else
            rank = 0;
        #endif

        //Creating string with rank number
        std::string rankstring = std::to_string(rank);

        // Getting the run folder for saving initialization files
        std::string inputspath = runOptions.getValue<std::string>("where_to_save_input");
        result["inputspath"] = inputspath;
        std::string modelfilesPath = inputspath + "/ModelFiles/mpirank_"+ rankstring + "/";

        // Get the path to the library
        std::string vevaciouslibpath = Backends::backendInfo().path_dir("vevacious", "1.0");
        std::string vevaciouspath = vevaciouslibpath + "/../";

        result["ScaleAndBlockFileSource"] = vevaciouspath + "ModelFiles/LagrangianParameters/MssmCompatibleWithSlhaOneAndSlhaTwo.xml";
        result["ModelFileSource"] = vevaciouspath + "ModelFiles/PotentialFunctions/RealMssmWithStauAndStopVevs.vin";
        result["ScaleAndBlockFile"] = modelfilesPath + "ScaleAndBlockFile.xml";
        result["ModelFile"] = modelfilesPath + "ModelFile.vin";
    }

    
    /// Helper function that takes any YAML options and makes the vevacious input,
    /// in the form of .xml files.
    void make_vpp_inputs(map_str_str &opts)
    {
        static bool firstrun = true;
        int rank;

        // Here we make sure files are only written the first time this is run
        if(firstrun) 
        {
            std::string vevaciouslibpath = Backends::backendInfo().path_dir("vevacious", "1.0");

            std::string vevaciouspath = vevaciouslibpath + "/../";

            // Get MPI rank
            #ifdef WITH_MPI
                MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            #else
                rank = 0;
            #endif

            //Creating string with rank number
            std::string rankstring = std::to_string(rank);

            // Getting the run folder for saving initialization files
            std::string inputspath = opts["inputspath"];

            // Declare some filenames before we make 'em.'
            std::string initfilesPath = inputspath + "/InitializationFiles/mpirank_"+ rankstring + "/";
            std::string modelfilesPath = inputspath + "/ModelFiles/mpirank_"+ rankstring + "/";
            std::string potentialfunctioninitpath = initfilesPath + "/PotentialFunctionInitialization.xml";
            std::string potentialminimizerinitpath = initfilesPath +"/PotentialMinimizerInitialization.xml";
            std::string tunnelingcalculatorinitpath = initfilesPath +"/TunnelingCalculatorInitialization.xml";
            

            // Creating folders for initialization files
            try
            {
              Utils::ensure_path_exists(initfilesPath);
              Utils::ensure_path_exists(modelfilesPath);
            }
            catch(const std::exception& e)
            {
                std::ostringstream errmsg;
                errmsg << "Error creating vevacious initialization and model files folders for MPI process " << rankstring;
                SpecBit_error().forced_throw(LOCAL_INFO,errmsg.str());
            }

            // Copy model files.
            try
            {

                std::ifstream  ScaleAndBlocksrc(opts.at("ScaleAndBlockFileSource") , std::ios::binary);
                std::ofstream  ScaleAndBlockdst(opts.at("ScaleAndBlockFile")       , std::ios::binary);

                ScaleAndBlockdst << ScaleAndBlocksrc.rdbuf();

                std::ifstream  ModelFilesrc(opts.at("ModelFileSource") , std::ios::binary);
                std::ofstream  ModelFiledst(opts.at("ModelFile")       , std::ios::binary);

                ModelFiledst << ModelFilesrc.rdbuf();
            }
            catch(const std::exception& e)
            {
                std::ostringstream errmsg;
                errmsg << "Error copying model and scale/block vevacious files. Check they exist." << rankstring;
                SpecBit_error().forced_throw(LOCAL_INFO,errmsg.str());
            }

            // Writing potential function initialization file
            // File contents
            std::string potentialfunctioninit =
                "<VevaciousPlusPlusPotentialFunctionInitialization>\n"
                " <LagrangianParameterManagerClass>\n"
                "    <ClassType>\n"
                "      SlhaCompatibleWithSarahManager\n"
                "    </ClassType>\n"
                "    <ConstructorArguments>\n"
                "      <ScaleAndBlockFile>\n"
                "      " + opts.at("ScaleAndBlockFile") + "\n"
                "      </ScaleAndBlockFile>\n"
                "    </ConstructorArguments>\n"
                "  </LagrangianParameterManagerClass>\n"
                "  <PotentialFunctionClass>\n"
                "    <ClassType>\n"
                "      " + opts.at("PotentialFunctionClassType") + "\n"
                "    </ClassType>\n"
                "    <ConstructorArguments>\n"
                "      <ModelFile>\n"
                "         " + opts.at("ModelFile") +
                "\n"
                "      </ModelFile>\n"
                "      <AssumedPositiveOrNegativeTolerance>\n"
                "        0.5\n"
                "      </AssumedPositiveOrNegativeTolerance>\n"
                "    </ConstructorArguments>\n"
                "  </PotentialFunctionClass>\n"
                "</VevaciousPlusPlusPotentialFunctionInitialization>";
            std::ofstream potentialfunctioninitwrite(potentialfunctioninitpath);
            potentialfunctioninitwrite << potentialfunctioninit;
            potentialfunctioninitwrite.close();
    
            std::string potentialminimizerinit;

            // Writing potential minimizer initialization file
            // Check whether user wants hom4ps2 or PHC as homotopy continuation backend

            if(opts.at("homotopybackend") == "hom4ps") 
            {

                // Getting Path to hom4ps2

                std::string PathToHom4ps2 = Backends::backendInfo().path_dir("hom4ps", "2.0");

                // File contents
                potentialminimizerinit =
                        "<VevaciousPlusPlusPotentialMinimizerInitialization>\n"
                        "  <PotentialMinimizerClass>\n"
                        "    <ClassType>\n"
                        "      GradientFromStartingPoints\n"
                        "    </ClassType>\n"
                        "    <ConstructorArguments>\n"
                        "      <StartingPointFinderClass>\n"
                        "        <ClassType>\n"
                        "          PolynomialAtFixedScalesSolver\n"
                        "        </ClassType>\n"
                        "        <ConstructorArguments>\n"
                        "          <NumberOfScales>\n"
                        "            1\n"
                        "          </NumberOfScales>\n"
                        "          <ReturnOnlyPolynomialMinima>\n"
                        "            No\n"
                        "          </ReturnOnlyPolynomialMinima>\n"
                        "          <PolynomialSystemSolver>\n"
                        "            <ClassType>\n"
                        "              Hom4ps2Runner\n"
                        "            </ClassType>\n"
                        "            <ConstructorArguments>\n"
                        "              <PathToHom4ps2>\n"
                        "        " + PathToHom4ps2 + "\n"
                        "              </PathToHom4ps2>\n"
                        "              <Hom4ps2Argument>\n"
                        "                1\n"
                        "              </Hom4ps2Argument>\n"
                        "              <ResolutionSize>\n"
                        "                1.0\n"
                        "              </ResolutionSize>\n"
                        "            </ConstructorArguments>\n"
                        "          </PolynomialSystemSolver>\n"
                        "        </ConstructorArguments>\n"
                        "      </StartingPointFinderClass>\n"
                        "      <GradientMinimizerClass>\n"
                        "        <ClassType>\n"
                        "          MinuitPotentialMinimizer\n"
                        "        </ClassType>\n"
                        "        <ConstructorArguments>\n"
                        "          <InitialStepSizeFraction>\n"
                        "            0.1\n"
                        "          </InitialStepSizeFraction>\n"
                        "          <MinimumInitialStepSize>\n"
                        "            1.0\n"
                        "          </MinimumInitialStepSize>\n"
                        "          <MinuitStrategy>\n"
                        "             "+ opts.at("MinuitStrategy") +"\n"
                        "          </MinuitStrategy>\n"
                        "        </ConstructorArguments>\n"
                        "      </GradientMinimizerClass>\n"
                        "      <ExtremumSeparationThresholdFraction>\n"
                        "        0.05\n"
                        "      </ExtremumSeparationThresholdFraction>\n"
                        "      <NonDsbRollingToDsbScalingFactor>\n"
                        "        4.0\n"
                        "      </NonDsbRollingToDsbScalingFactor>\n"
                        "      <GlobalIsPanic>\n"
                        "        " + opts.at("globalIsPanic") + "\n"
                        "      </GlobalIsPanic>\n"
                        "    </ConstructorArguments>\n"
                        "  </PotentialMinimizerClass>\n"
                        "</VevaciousPlusPlusObjectInitialization>\n";
            } 
            else if(opts.at("homotopybackend") == "phc") 
            {
                // Getting path to PHC
                std::string PathToPHC = Backends::backendInfo().path_dir("phc", "2.4.58");
                // Creating symlink to PHC in run folder
                std::string PHCSymlink = inputspath + "Homotopy/mpirank_"+ rankstring + "/";

                // random seed for phc (can be fixed as input option)
                std::string randomseed = opts["phc_random_seed"];
                
                try
                {
                    Utils::ensure_path_exists(PHCSymlink);
                }
                catch(const std::exception& e)
                {
                    std::ostringstream errmsg;
                    errmsg << "Error creating PHC folder for MPI process " << rankstring;
                    SpecBit_error().forced_throw(LOCAL_INFO,errmsg.str());
                }

                // Here we check whether the symlink to phc already exists. 
                std::string filename(PHCSymlink + "/phc");
                std::ifstream in(filename.c_str(), std::ios::binary);
                if(in.good())
                {
                    std::cout << "Symlink for phc already exists, skipping creating it." << std::endl;
                }
                else
                {

                    std::cout << "Creating PHC symlink" << std::endl;
                    std::string systemCommand( "ln -s " + PathToPHC + "/phc" + " " + PHCSymlink );

                    int systemReturn = system( systemCommand.c_str() ) ;
                    if( systemReturn == -1 )
                    {
                        std::ostringstream errmsg;
                        errmsg << "Error making symlink for PHC in process " << rankstring;
                        SpecBit_error().forced_throw(LOCAL_INFO,errmsg.str());
                    }
                }

                // File contents
                potentialminimizerinit =
                        "<VevaciousPlusPlusPotentialMinimizerInitialization>\n"
                        "  <PotentialMinimizerClass>\n"
                        "    <ClassType>\n"
                        "      GradientFromStartingPoints\n"
                        "    </ClassType>\n"
                        "    <ConstructorArguments>\n"
                        "      <StartingPointFinderClass>\n"
                        "        <ClassType>\n"
                        "          PolynomialAtFixedScalesSolver\n"
                        "        </ClassType>\n"
                        "        <ConstructorArguments>\n"
                        "          <NumberOfScales>\n"
                        "            1\n"
                        "          </NumberOfScales>\n"
                        "          <ReturnOnlyPolynomialMinima>\n"
                        "            No\n"
                        "          </ReturnOnlyPolynomialMinima>\n"
                        "          <PolynomialSystemSolver>\n"
                        "            <ClassType>\n"
                        "              PHCRunner\n"
                        "            </ClassType>\n"
                        "            <ConstructorArguments>\n"
                        "              <PathToPHC>\n"
                        "        " + PHCSymlink + "\n"
                        "              </PathToPHC>\n"
                        "              <ResolutionSize>\n"
                        "                1.0\n"
                        "              </ResolutionSize>\n"
                        "              <RandomSeed>\n"
                        "               " + randomseed + "\n"
                        "              </Randomseed>\n"
                        "            <Tasks>\n "
                        "             1                     "
                        "            </Tasks>\n            "
                        "            </ConstructorArguments>\n"
                        "          </PolynomialSystemSolver>\n"
                        "        </ConstructorArguments>\n"
                        "      </StartingPointFinderClass>\n"
                        "      <GradientMinimizerClass>\n"
                        "        <ClassType>\n"
                        "          MinuitPotentialMinimizer\n"
                        "        </ClassType>\n"
                        "        <ConstructorArguments>\n"
                        "          <InitialStepSizeFraction>\n"
                        "            0\n"
                        "          </InitialStepSizeFraction>\n"
                        "          <MinimumInitialStepSize>\n"
                        "            0.5\n"
                        "          </MinimumInitialStepSize>\n"
                        "          <MinuitStrategy>\n"
                        "            "+ opts.at("MinuitStrategy") +"\n"
                        "          </MinuitStrategy>\n"
                        "        </ConstructorArguments>\n"
                        "      </GradientMinimizerClass>\n"
                        "      <ExtremumSeparationThresholdFraction>\n"
                        "        0.1\n"
                        "      </ExtremumSeparationThresholdFraction>\n"
                        "      <NonDsbRollingToDsbScalingFactor>\n"
                        "        4.0\n"
                        "      </NonDsbRollingToDsbScalingFactor>\n"
                        "      <GlobalIsPanic>\n"
                        "        " + opts.at("globalIsPanic") + "\n"
                        "      </GlobalIsPanic>\n"
                        "    </ConstructorArguments>\n"
                        "  </PotentialMinimizerClass>\n"
                        "</VevaciousPlusPlusObjectInitialization>\n";
            } else
            {
                std::ostringstream errmsg;
                errmsg << "The homotopy_backend option in the YAML file has not been set up properly. Check the input YAML file." << std::endl;
                SpecBit_error().raise(LOCAL_INFO,errmsg.str());
            }

            std::ofstream potentialminimizerinitwrite(potentialminimizerinitpath);
            potentialminimizerinitwrite << potentialminimizerinit;
            potentialminimizerinitwrite.close();

            // Writing tunneling calculator initialization file
            // File contents
            std::string tunnelingcalculatorinit =
                "<VevaciousPlusPlusObjectInitialization>\n"
                "  <TunnelingClass>\n"
                "    <ClassType>\n"
                "      BounceAlongPathWithThreshold\n"
                "    </ClassType>\n"
                "    <ConstructorArguments>\n"
                "      <TunnelingStrategy>\n"
                "    " + opts.at("TunnelingStrategy") + "\n"
                "      </TunnelingStrategy>\n"
                "      <SurvivalProbabilityThreshold>\n"
                "        " + opts.at("SurvivalProbabilityThreshold") + "\n"
                "      </SurvivalProbabilityThreshold>\n"
                "      <ThermalActionResolution>\n"
                "        5\n"
                "      </ThermalActionResolution>\n"
                "      <CriticalTemperatureAccuracy>\n"
                "        4\n"
                "      </CriticalTemperatureAccuracy>\n"
                "      <PathResolution>\n"
                "        " + opts.at("PathResolution") + "\n"
                "      </PathResolution>\n"
                "      <Timeout>\n"
                "        "+ opts.at("pathFindingTimeout") +"\n"
                "      </Timeout>\n"
                "      <MinimumVacuumSeparationFraction>\n"
                "        0.2\n"
                "      </MinimumVacuumSeparationFraction>\n"
                "      <BouncePotentialFit>\n"
                "        <ClassType>\n"
                "          BubbleShootingOnPathInFieldSpace\n"
                "        </ClassType>\n"
                "        <ConstructorArguments>\n"
                "          <NumberShootAttemptsAllowed>\n"
                "            32\n"
                "          </NumberShootAttemptsAllowed>\n"
                "          <RadialResolution>\n"
                "            "+ opts.at("radialResolution") +"\n"
                "          </RadialResolution>\n"
                "        </ConstructorArguments>\n"
                "      </BouncePotentialFit>\n"
                "      <TunnelPathFinders>\n"
                "        <PathFinder>\n"
                "          <ClassType>\n"
                "            MinuitOnPotentialOnParallelPlanes\n"
                "          </ClassType>\n"
                "          <ConstructorArguments>\n"
                "            <NumberOfPathSegments>\n"
                "              50 \n"
                "            </NumberOfPathSegments>\n"
                "            <MinuitStrategy>\n"
                "             "+ opts.at("MinuitStrategy") +"\n"
                "            </MinuitStrategy>\n"
                "            <MinuitTolerance>\n"
                "              1\n"
                "            </MinuitTolerance>\n"
                "          </ConstructorArguments>\n"
                "        </PathFinder>\n"
                "        <PathFinder>\n"
                "          <ClassType>\n"
                "            MinuitOnPotentialPerpendicularToPath\n"
                "          </ClassType>\n"
                "          <ConstructorArguments>\n"
                "            <NumberOfPathSegments>\n"
                "              50\n"
                "            </NumberOfPathSegments>\n"
                "            <NumberOfAllowedWorsenings>\n"
                "              3\n"
                "            </NumberOfAllowedWorsenings>\n"
                "            <ConvergenceThresholdFraction>\n"
                "              0.5\n"
                "            </ConvergenceThresholdFraction>\n"
                "            <MinuitDampingFraction>\n"
                "              0.75\n"
                "            </MinuitDampingFraction>\n"
                "            <NeighborDisplacementWeights>\n"
                "              0.5\n"
                "              0.25\n"
                "            </NeighborDisplacementWeights>\n"
                "            <MinuitStrategy>\n"
                "               "+ opts.at("MinuitStrategy") +"\n"
                "            </MinuitStrategy>\n"
                "            <MinuitTolerance>\n"
                "              1\n"
                "            </MinuitTolerance>\n"
                "          </ConstructorArguments>\n"
                "        </PathFinder>\n"
                "      </TunnelPathFinders>\n"
                "    </ConstructorArguments>\n"
                "  </TunnelingClass>\n"
                "</VevaciousPlusPlusObjectInitialization>";

            std::ofstream tunnelingcalculatorinitwrite(tunnelingcalculatorinitpath);
            tunnelingcalculatorinitwrite << tunnelingcalculatorinit;
            tunnelingcalculatorinitwrite.close();

            //Finally write the main input file for VevaciousPlusPlus

            std::string inputFilename =
                    inputspath + "/InitializationFiles/VevaciousPlusPlusObjectInitialization_mpirank_"+ rankstring +".xml";

            // File contents
            std::string inputfile =
                "<VevaciousPlusPlusObjectInitialization>\n"
                "  <PotentialFunctionInitializationFile>\n"
                "   " + potentialfunctioninitpath + "\n"
                "  </PotentialFunctionInitializationFile>\n"
                "  <PotentialMinimizerInitializationFile>\n"
                "   " + potentialminimizerinitpath + "\n"
                "  </PotentialMinimizerInitializationFile>\n"
                "  <TunnelingCalculatorInitializationFile>\n"
                "   " +
                tunnelingcalculatorinitpath + "\n"
                "  </TunnelingCalculatorInitializationFile>\n"
                "</VevaciousPlusPlusObjectInitialization>";
            std::ofstream inputwrite(inputFilename);
            inputwrite << inputfile;
            inputwrite.close();
            firstrun = false;
        }

    }

    /// This function adds all entries of the spectrum object (as SLHAea) that need to be passed to vevacious
    /// to an instance of type SpectrumEntriesForVevacious. The actual passing happens in the helper function 
    /// exec_pass_spectrum_to_vevacious which gets executed every time before a vevacious call. 
    /// Model dependent. 
    void prepare_pass_MSSM_spectrum_to_vevacious(SpectrumEntriesForVevacious &result)
    {
        namespace myPipe = Pipes::prepare_pass_MSSM_spectrum_to_vevacious;

        static std::string inputspath =  *myPipe::Dep::init_vevacious;

        // print input parameters for vevaciouswith full precision to be able to reproduce 
        // vevacious run with the exact same parameters
        std::ostringstream InputsForLog;
        InputsForLog << std::fixed << std::setprecision(12) << "Passing parameters to Vevacious with values: ";
        
        for (auto it=myPipe::Param.begin(); it != myPipe::Param.end(); it++)
        {
            std::string name = it->first;
            double value = *myPipe::Param[name];
            InputsForLog << "\n  " << name << ": " << value;
            //std::cout << " " << name << " = " << value << std::endl;
        }
        std::string InputsForLogString = InputsForLog.str();
        logger() << InputsForLogString << EOM;

        // Getting mpi rank
        int rank;
        #ifdef WITH_MPI
                    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        #else
                    rank = 0;
        #endif

        std::string rankstring = std::to_string(rank);

        std::string inputFilename = inputspath + "/InitializationFiles/VevaciousPlusPlusObjectInitialization_mpirank_"+ rankstring +".xml";
        
        result.set_inputFilename(inputFilename);
        result.set_inputPath(inputspath);

        // Get the spectrum object for the MSSM
        const Spectrum& fullspectrum = *myPipe::Dep::unimproved_MSSM_spectrum;
        const SubSpectrum& spectrumHE = fullspectrum.get_HE();

        // Get the SLHAea::Coll object & scale from the spectrum
        SLHAea::Coll slhaea = spectrumHE.getSLHAea(2);
        double scale = spectrumHE.GetScale();

        cout << "VEVACIOUS SCALE:  "<< scale << endl;
        result.set_scale(scale);

        // safe parameters form the SLHAea::Coll object in SpectrumEntriesForVevacious 
        // by calling the method .add_entry => pass name, vector with <int,double> pairs
        // and the third setting (some int, don't know what it does @Sanjay? ) 
        std::vector<std::pair<int,double>> gaugecouplings = { { 1 , SLHAea::to<double>(slhaea.at("GAUGE").at(1).at(1))  }, 
                      { 2, SLHAea::to<double>(slhaea.at("GAUGE").at(2).at(1)) }, 
                      { 3, SLHAea::to<double>(slhaea.at("GAUGE").at(3).at(1)) }};
        result.add_entry("GAUGE", gaugecouplings, 1);

        std::vector<std::pair<int,double>> Hmix = { { 1 , SLHAea::to<double>(slhaea.at("HMIX").at(1).at(1))},
                      { 101, SLHAea::to<double>(slhaea.at("HMIX").at(101).at(1))},
                      { 102, SLHAea::to<double>(slhaea.at("HMIX").at(102).at(1))},
                      { 103, SLHAea::to<double>(slhaea.at("HMIX").at(103).at(1))},
                      { 3, SLHAea::to<double>(slhaea.at("HMIX").at(3).at(1))}};
        result.add_entry("HMIX", Hmix, 1);
      
       
        std::vector<std::pair<int,double>> minpar = {{ 3, SLHAea::to<double>(slhaea.at("MINPAR").at(3).at(1))}};
        result.add_entry("MINPAR", minpar, 1);
      
        std::vector<std::pair<int,double>> msoft = { { 21 , SLHAea::to<double>(slhaea.at("MSOFT").at(21).at(1))},
                              { 22  , SLHAea::to<double>(slhaea.at("MSOFT").at(22).at(1))},
                              { 1  ,  SLHAea::to<double>(slhaea.at("MSOFT").at(1).at(1))},
                              { 2  ,  SLHAea::to<double>(slhaea.at("MSOFT").at(2).at(1))},
                              { 3   , SLHAea::to<double>(slhaea.at("MSOFT").at(3).at(1))}};       
        result.add_entry("MSOFT", msoft, 1);
        
        // Check if the block "TREEMSOFT" is present
        try 
        {
            std::vector<std::pair<int, double>> treemsoft = {{21, SLHAea::to<double>(slhaea.at("TREEMSOFT").at(21).at(1))},
                                                             {22, SLHAea::to<double>(slhaea.at("TREEMSOFT").at(22).at(1))} };
            result.add_entry("TREEMSOFT", treemsoft, 1);
        }
        catch (const std::exception& e){ cout << "No TREEMSOFT, skipping" << endl;}
        
        // Check if the block "LOOPMSOFT" is present
        try 
        {
            std::vector<std::pair<int, double>> loopmsoft = {{21, SLHAea::to<double>(slhaea.at("LOOPMSOFT").at(21).at(1))},
                                                             {22, SLHAea::to<double>(slhaea.at("LOOPMSOFT").at(22).at(1))}};
            result.add_entry("LOOPMSOFT", loopmsoft, 1);
        }
        catch (const std::exception& e) {cout << "No LOOPMSOFT, skipping" << endl;}

        bool diagonalYukawas = false;
        // Here we check if the Yukawas are diagonal or not, i.e if the "YX" blocks have off-diagonal entries.
        try 
        {
          SLHAea::to<double>(slhaea.at("YU").at(1,2));
        }
        catch (const std::exception& e) {diagonalYukawas = true; cout << "Diagonal Yukawas detected"<< endl;}

        // If diagonal pass the diagonal values to Vevacious
        if (diagonalYukawas) 
        {
            std::vector<std::pair<int,double>> Yu = { { 11 , SLHAea::to<double>(slhaea.at("YU").at(1,1).at(2))},
                                                      { 12, 0},
                                                      { 13, 0},
                                                      { 21, 0},
                                                      { 22, SLHAea::to<double>(slhaea.at("YU").at(2,2).at(2))},
                                                      { 23, 0},
                                                      { 31, 0},
                                                      { 32, 0},
                                                      { 33, SLHAea::to<double>(slhaea.at("YU").at(3,3).at(2))}};
            result.add_entry("YU", Yu, 2);

            std::vector<std::pair<int,double>> Yd = { { 11 , SLHAea::to<double>(slhaea.at("YD").at(1,1).at(2))},
                                                      { 12, 0},
                                                      { 13, 0},
                                                      { 21, 0},
                                                      { 22, SLHAea::to<double>(slhaea.at("YD").at(2,2).at(2))},
                                                      { 23, 0},
                                                      { 31, 0},
                                                      { 32, 0},
                                                      { 33, SLHAea::to<double>(slhaea.at("YD").at(3,3).at(2))}};
            result.add_entry("YD", Yd, 2);

            std::vector<std::pair<int,double>> Ye = { { 11 , SLHAea::to<double>(slhaea.at("YE").at(1,1).at(2))},
                                                      { 12, 0},
                                                      { 13, 0},
                                                      { 21, 0},
                                                      { 22, SLHAea::to<double>(slhaea.at("YE").at(2,2).at(2))},
                                                      { 23, 0},
                                                      { 31, 0},
                                                      { 32, 0},
                                                      { 33, SLHAea::to<double>(slhaea.at("YE").at(3,3).at(2))}};
            result.add_entry("YE", Ye, 2);
        } 
        // If NOT diagonal pass values to Vevacious
        else 
        { 
            std::vector<std::pair<int, double>> Yu = {{11, SLHAea::to<double>(slhaea.at("YU").at(1, 1).at(2))},
                                                      {12, SLHAea::to<double>(slhaea.at("YU").at(1, 2).at(2))},
                                                      {13, SLHAea::to<double>(slhaea.at("YU").at(1, 3).at(2))},
                                                      {21, SLHAea::to<double>(slhaea.at("YU").at(2, 1).at(2))},
                                                      {22, SLHAea::to<double>(slhaea.at("YU").at(2, 2).at(2))},
                                                      {23, SLHAea::to<double>(slhaea.at("YU").at(2, 3).at(2))},
                                                      {31, SLHAea::to<double>(slhaea.at("YU").at(3, 1).at(2))},
                                                      {32, SLHAea::to<double>(slhaea.at("YU").at(3, 2).at(2))},
                                                      {33, SLHAea::to<double>(slhaea.at("YU").at(3, 3).at(2))}};
            result.add_entry("YU", Yu, 2);

            std::vector<std::pair<int, double>> Yd = {{11, SLHAea::to<double>(slhaea.at("YD").at(1, 1).at(2))},
                                                      {12, SLHAea::to<double>(slhaea.at("YD").at(1, 2).at(2))},
                                                      {13, SLHAea::to<double>(slhaea.at("YD").at(1, 3).at(2))},
                                                      {21, SLHAea::to<double>(slhaea.at("YD").at(2, 1).at(2))},
                                                      {22, SLHAea::to<double>(slhaea.at("YD").at(2, 2).at(2))},
                                                      {23, SLHAea::to<double>(slhaea.at("YD").at(2, 3).at(2))},
                                                      {31, SLHAea::to<double>(slhaea.at("YD").at(3, 1).at(2))},
                                                      {32, SLHAea::to<double>(slhaea.at("YD").at(3, 2).at(2))},
                                                      {33, SLHAea::to<double>(slhaea.at("YD").at(3, 3).at(2))}};
            result.add_entry("YD", Yd, 2);

            std::vector<std::pair<int, double>> Ye = {{11, SLHAea::to<double>(slhaea.at("YE").at(1, 1).at(2))},
                                                      {12, SLHAea::to<double>(slhaea.at("YE").at(1, 2).at(2))},
                                                      {13, SLHAea::to<double>(slhaea.at("YE").at(1, 3).at(2))},
                                                      {21, SLHAea::to<double>(slhaea.at("YE").at(2, 1).at(2))},
                                                      {22, SLHAea::to<double>(slhaea.at("YE").at(2, 2).at(2))},
                                                      {23, SLHAea::to<double>(slhaea.at("YE").at(2, 3).at(2))},
                                                      {31, SLHAea::to<double>(slhaea.at("YE").at(3, 1).at(2))},
                                                      {32, SLHAea::to<double>(slhaea.at("YE").at(3, 2).at(2))},
                                                      {33, SLHAea::to<double>(slhaea.at("YE").at(3, 3).at(2))}};
            result.add_entry("YE", Ye, 2);
        }
        std::vector<std::pair<int, double>> Tu = {{11, SLHAea::to<double>(slhaea.at("TU").at(1, 1).at(2))},
                                                  {12, SLHAea::to<double>(slhaea.at("TU").at(1, 2).at(2))},
                                                  {13, SLHAea::to<double>(slhaea.at("TU").at(1, 3).at(2))},
                                                  {21, SLHAea::to<double>(slhaea.at("TU").at(2, 1).at(2))},
                                                  {22, SLHAea::to<double>(slhaea.at("TU").at(2, 2).at(2))},
                                                  {23, SLHAea::to<double>(slhaea.at("TU").at(2, 3).at(2))},
                                                  {31, SLHAea::to<double>(slhaea.at("TU").at(3, 1).at(2))},
                                                  {32, SLHAea::to<double>(slhaea.at("TU").at(3, 2).at(2))},
                                                  {33, SLHAea::to<double>(slhaea.at("TU").at(3, 3).at(2))}};
        result.add_entry("TU", Tu, 2);

        std::vector<std::pair<int,double>> Td = { { 11 , SLHAea::to<double>(slhaea.at("TD").at(1,1).at(2))},
                                                  { 12, SLHAea::to<double>(slhaea.at("TD").at(1,2).at(2))},
                                                  { 13, SLHAea::to<double>(slhaea.at("TD").at(1,3).at(2))},
                                                  { 21, SLHAea::to<double>(slhaea.at("TD").at(2,1).at(2))},
                                                  { 22, SLHAea::to<double>(slhaea.at("TD").at(2,2).at(2))},
                                                  { 23, SLHAea::to<double>(slhaea.at("TD").at(2,3).at(2))},
                                                  { 31, SLHAea::to<double>(slhaea.at("TD").at(3,1).at(2))},
                                                  { 32, SLHAea::to<double>(slhaea.at("TD").at(3,2).at(2))},
                                                  { 33, SLHAea::to<double>(slhaea.at("TD").at(3,3).at(2))}};                      
        result.add_entry("TD", Td, 2);

        std::vector<std::pair<int,double>> Te = { { 11 , SLHAea::to<double>(slhaea.at("TE").at(1,1).at(2))},
                                                  { 12, SLHAea::to<double>(slhaea.at("TE").at(1,2).at(2))},
                                                  { 13, SLHAea::to<double>(slhaea.at("TE").at(1,3).at(2))},
                                                  { 21, SLHAea::to<double>(slhaea.at("TE").at(2,1).at(2))},
                                                  { 22, SLHAea::to<double>(slhaea.at("TE").at(2,2).at(2))},
                                                  { 23, SLHAea::to<double>(slhaea.at("TE").at(2,3).at(2))},
                                                  { 31, SLHAea::to<double>(slhaea.at("TE").at(3,1).at(2))},
                                                  { 32, SLHAea::to<double>(slhaea.at("TE").at(3,2).at(2))},
                                                  { 33, SLHAea::to<double>(slhaea.at("TE").at(3,3).at(2))}};              
        result.add_entry("TE", Te, 2);


        std::vector<std::pair<int,double>> msq2 = { { 11 , SLHAea::to<double>(slhaea.at("MSQ2").at(1,1).at(2))},
                                                  { 12, SLHAea::to<double>(slhaea.at("MSQ2").at(1,2).at(2))},
                                                  { 13, SLHAea::to<double>(slhaea.at("MSQ2").at(1,3).at(2))},
                                                  { 21, SLHAea::to<double>(slhaea.at("MSQ2").at(2,1).at(2))},
                                                  { 22, SLHAea::to<double>(slhaea.at("MSQ2").at(2,2).at(2))},
                                                  { 23, SLHAea::to<double>(slhaea.at("MSQ2").at(2,3).at(2))},
                                                  { 31, SLHAea::to<double>(slhaea.at("MSQ2").at(3,1).at(2))},
                                                  { 32, SLHAea::to<double>(slhaea.at("MSQ2").at(3,2).at(2))},
                                                  { 33, SLHAea::to<double>(slhaea.at("MSQ2").at(3,3).at(2))}};               
        result.add_entry("MSQ2", msq2, 2);

        std::vector<std::pair<int,double>> msl2 = { { 11 , SLHAea::to<double>(slhaea.at("MSL2").at(1,1).at(2))},
                                                    { 12, SLHAea::to<double>(slhaea.at("MSL2").at(1,2).at(2))},
                                                    { 13, SLHAea::to<double>(slhaea.at("MSL2").at(1,3).at(2))},
                                                    { 21, SLHAea::to<double>(slhaea.at("MSL2").at(2,1).at(2))},
                                                    { 22, SLHAea::to<double>(slhaea.at("MSL2").at(2,2).at(2))},
                                                    { 23, SLHAea::to<double>(slhaea.at("MSL2").at(2,3).at(2))},
                                                    { 31, SLHAea::to<double>(slhaea.at("MSL2").at(3,1).at(2))},
                                                    { 32, SLHAea::to<double>(slhaea.at("MSL2").at(3,2).at(2))},
                                                    { 33, SLHAea::to<double>(slhaea.at("MSL2").at(3,3).at(2))}};               
        result.add_entry("MSL2", msl2, 2);

        std::vector<std::pair<int,double>> msd2 = { { 11 , SLHAea::to<double>(slhaea.at("MSD2").at(1,1).at(2))},
                                                    { 12, SLHAea::to<double>(slhaea.at("MSD2").at(1,2).at(2))},
                                                    { 13, SLHAea::to<double>(slhaea.at("MSD2").at(1,3).at(2))},
                                                    { 21, SLHAea::to<double>(slhaea.at("MSD2").at(2,1).at(2))},
                                                    { 22, SLHAea::to<double>(slhaea.at("MSD2").at(2,2).at(2))},
                                                    { 23, SLHAea::to<double>(slhaea.at("MSD2").at(2,3).at(2))},
                                                    { 31, SLHAea::to<double>(slhaea.at("MSD2").at(3,1).at(2))},
                                                    { 32, SLHAea::to<double>(slhaea.at("MSD2").at(3,2).at(2))},
                                                    { 33, SLHAea::to<double>(slhaea.at("MSD2").at(3,3).at(2))}};               
        result.add_entry("MSD2", msd2, 2);

        std::vector<std::pair<int,double>> mse2 = { { 11 , SLHAea::to<double>(slhaea.at("MSE2").at(1,1).at(2))},
                                                    { 12, SLHAea::to<double>(slhaea.at("MSE2").at(1,2).at(2))},
                                                    { 13, SLHAea::to<double>(slhaea.at("MSE2").at(1,3).at(2))},
                                                    { 21, SLHAea::to<double>(slhaea.at("MSE2").at(2,1).at(2))},
                                                    { 22, SLHAea::to<double>(slhaea.at("MSE2").at(2,2).at(2))},
                                                    { 23, SLHAea::to<double>(slhaea.at("MSE2").at(2,3).at(2))},
                                                    { 31, SLHAea::to<double>(slhaea.at("MSE2").at(3,1).at(2))},
                                                    { 32, SLHAea::to<double>(slhaea.at("MSE2").at(3,2).at(2))},
                                                    { 33, SLHAea::to<double>(slhaea.at("MSE2").at(3,3).at(2))}};                
        result.add_entry("MSE2", mse2, 2);

        std::vector<std::pair<int,double>> msu2 = { { 11 , SLHAea::to<double>(slhaea.at("MSU2").at(1,1).at(2))},
                                                    { 12, SLHAea::to<double>(slhaea.at("MSU2").at(1,2).at(2))},
                                                    { 13, SLHAea::to<double>(slhaea.at("MSU2").at(1,3).at(2))},
                                                    { 21, SLHAea::to<double>(slhaea.at("MSU2").at(2,1).at(2))},
                                                    { 22, SLHAea::to<double>(slhaea.at("MSU2").at(2,2).at(2))},
                                                    { 23, SLHAea::to<double>(slhaea.at("MSU2").at(2,3).at(2))},
                                                    { 31, SLHAea::to<double>(slhaea.at("MSU2").at(3,1).at(2))},
                                                    { 32, SLHAea::to<double>(slhaea.at("MSU2").at(3,2).at(2))},
                                                    { 33, SLHAea::to<double>(slhaea.at("MSU2").at(3,3).at(2))}};            
        result.add_entry("MSU2", msu2, 2);
    }
  } // end namespace SpecBit
} // end namespace Gambit
