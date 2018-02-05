//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Standard Model Neutrino parameters.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///
///  \author Tomas Gonzalo
///          (t.e.gonzalo@fys.uio.no)
///  \date 2017 July
///
///  *********************************************


#ifndef __StandardModel_Neutrinos_hpp__
#define __StandardModel_Neutrinos_hpp__

#define MODEL StandardModel_Neutrinos

  START_MODEL

  // Mass parameters (correspoding to the SLHA2 entries 12, 14 and 8 respectively)
  //DEFINEPARS(min_mass, ordering, md21, md31, md23)
  DEFINEPARS(mnu1, mnu2, mnu3)

  // PMNS parameters
  DEFINEPARS(theta12, theta23, theta13)
  DEFINEPARS(delta13, alpha1, alpha2)

#undef MODEL

#endif /* __StandardModel_Neutrinos_hpp__ */
