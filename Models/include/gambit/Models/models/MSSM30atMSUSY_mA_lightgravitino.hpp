///  GAMBIT: Global and Modular BSM Inference Tool
///  *********************************************
///
///  MSSM30atMSUSY_mA_lightgravitino model definition
///
///  Specialisation of MSSM63atMSUSY_lightgravitino with all
///  off-diagonal m and A terms set to zero.
///  *********************************************
///
///  Authors
///  =======
///
///  (add name and date if you modify)
///
///  \author Ben Farmer
///          (ben.farmer@gmail.com)
///  \date 2017 Oct
///
///  \author Pat Scott
///          (p.scott@imperial.ac.uk)
///  \date 2018 Sep
///
///  *********************************************

#ifndef __MSSM30atMSUSY_mA_lightgravitino_hpp__
#define __MSSM30atMSUSY_mA_lightgravitino_hpp__

#include "gambit/Models/models/MSSM63atMSUSY_mA_lightgravitino.hpp" // Parent model must be declared first! Include it here to ensure that this happens.
#include "gambit/Models/models/MSSM30atMSUSY_lightgravitino.hpp"    // Likewise for 'friend' models

/// FlexibleSUSY compatible general (78 parameters plus sign) MSSM parameterisation
#define MODEL MSSM30atMSUSY_mA_lightgravitino
#define PARENT MSSM63atMSUSY_mA_lightgravitino
  START_MODEL

  DEFINEPARS(Qin,TanBeta,
             mA,mu,M1,M2,M3,mG)

  DEFINEPARS(mq2_1, mq2_2, mq2_3)

  DEFINEPARS(ml2_1, ml2_2, ml2_3)

  DEFINEPARS(md2_1, md2_2, md2_3)

  DEFINEPARS(mu2_1, mu2_2, mu2_3)

  DEFINEPARS(me2_1, me2_2, me2_3)

  DEFINEPARS(Ae_1, Ae_2, Ae_3)

  DEFINEPARS(Ad_1, Ad_2, Ad_3)

  DEFINEPARS(Au_1, Au_2, Au_3)

  INTERPRET_AS_PARENT_FUNCTION(MSSM30atMSUSY_mA_lightgravitino_to_MSSM63atMSUSY_mA_lightgravitino)
  INTERPRET_AS_X_FUNCTION(MSSM30atMSUSY_lightgravitino, MSSM30atMSUSY_mA_lightgravitino_to_MSSM30atMSUSY_lightgravitino)
  INTERPRET_AS_X_DEPENDENCY(MSSM30atMSUSY_lightgravitino, unimproved_MSSM_spectrum, Spectrum)

#undef PARENT
#undef MODEL

#endif
