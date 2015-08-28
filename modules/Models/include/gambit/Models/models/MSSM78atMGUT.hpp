//  GAMBIT: Global and Modular BSM Inference Tool
//  *********************************************
//
//  MSSM78 model declaration
//   
//
//  *********************************************
//
//  Authors
//  =======
//
//  (add name and date if you modify)
//
//  Ben Farmer
//  2013 May, ???, 2014 Mar, 2015 Jan
//  Pat Scott
//  2013 Sep
//
//  *********************************************

#ifndef __MSSM78atMGUT_hpp__
#define __MSSM78atMGUT_hpp__

#include "gambit/Models/models/MSSM78atQ.hpp" // Must include models which are targets of translation functions

// General GUT boundary condition parameterisation of the MSSM
// There are several of these, compatible with different spectrum generators
// To use a constrained GUT model like the CMSSM, there needs to be an 
// "interpret_as_X" function which translates the CMSSM parameters into
// the appropriate general GUT parameterisation for the spectrum generator
// being used.

/// FlexibleSUSY compatible general (78 parameters plus sign) GUT scale MSSM parameterisation
#define MODEL  MSSM78atMGUT
#define PARENT MSSM78atQ
  START_MODEL

  /// Can translate this model into MSSM78atQ (where Q will then be set to MGUT)
  INTERPRET_AS_PARENT__FUNCTION(MSSM78atMGUT_to_MSSM78atQ)
  /// Depends on an MSSM spectrum, since RGEs must run in order to determine MGUT
  INTERPRET_AS_PARENT__DEPENDENCY(unimproved_MSSM_spectrum, const Spectrum*)

  DEFINEPARS(TanBeta,SignMu,
             mHu2,mHd2,M1,M2,M3)
  
  DEFINEPARS(mq2_11, mq2_12, mq2_13,
             mq2_21, mq2_22, mq2_23,
             mq2_31, mq2_32, mq2_33)

  DEFINEPARS(ml2_11, ml2_12, ml2_13,
             ml2_21, ml2_22, ml2_23,
             ml2_31, ml2_32, ml2_33)

  DEFINEPARS(md2_11, md2_12, md2_13,
             md2_21, md2_22, md2_23,
             md2_31, md2_32, md2_33)

  DEFINEPARS(mu2_11, mu2_12, mu2_13,
             mu2_21, mu2_22, mu2_23,
             mu2_31, mu2_32, mu2_33)

  DEFINEPARS(me2_11, me2_12, me2_13,
             me2_21, me2_22, me2_23,
             me2_31, me2_32, me2_33)

  DEFINEPARS(Ae_11, Ae_12, Ae_13,
             Ae_21, Ae_22, Ae_23,
             Ae_31, Ae_32, Ae_33)
  
  DEFINEPARS(Ad_11, Ad_12, Ad_13,
             Ad_21, Ad_22, Ad_23,
             Ad_31, Ad_32, Ad_33)

  DEFINEPARS(Au_11, Au_12, Au_13,
             Au_21, Au_22, Au_23,
             Au_31, Au_32, Au_33)
#undef PARENT
#undef MODEL

#endif
