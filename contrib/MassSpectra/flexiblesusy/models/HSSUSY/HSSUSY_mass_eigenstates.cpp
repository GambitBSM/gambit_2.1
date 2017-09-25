// ====================================================================
// This file is part of FlexibleSUSY.
//
// FlexibleSUSY is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// FlexibleSUSY is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FlexibleSUSY.  If not, see
// <http://www.gnu.org/licenses/>.
// ====================================================================

// File generated at Sun 24 Sep 2017 15:57:18

/**
 * @file HSSUSY_mass_eigenstates.cpp
 * @brief implementation of the HSSUSY model class
 *
 * Contains the definition of the HSSUSY model class methods
 * which solve EWSB and calculate pole masses and mixings from MSbar
 * parameters.
 *
 * This file was generated at Sun 24 Sep 2017 15:57:18 with FlexibleSUSY
 * 2.0.0-dev (git commit: 4d4c39a2702e9a6604f84813ccb0b85d40987f3b) and SARAH 4.11.0 .
 */

#include "HSSUSY_mass_eigenstates.hpp"
#include "HSSUSY_ewsb_solver_interface.hpp"
#include "eigen_utils.hpp"
#include "ewsb_solver.hpp"
#include "wrappers.hpp"
#include "linalg2.hpp"
#include "numerics2.hpp"
#include "logger.hpp"
#include "error.hpp"
#include "pv.hpp"
#include "raii.hpp"
#include "thread_pool.hpp"
#include "functors.hpp"

#include "config.h"

#ifdef ENABLE_TWO_SCALE_SOLVER
#include "HSSUSY_two_scale_ewsb_solver.hpp"
#endif

#include "sm_twoloophiggs.hpp"

#include "sm_threeloophiggs.hpp"

#include "sm_threeloop_as.hpp"


#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace flexiblesusy {

#define CLASSNAME HSSUSY_mass_eigenstates

#define PHYSICAL(parameter) physical.parameter
#define INPUT(parameter) model.get_input().parameter
#define LOCALINPUT(parameter) input.parameter
#define MODELPARAMETER(parameter) model.get_##parameter()
#define EXTRAPARAMETER(parameter) model.get_##parameter()

#define HIGGS_2LOOP_CORRECTION_AT_AS     loop_corrections.higgs_at_as
#define HIGGS_2LOOP_CORRECTION_AB_AS     loop_corrections.higgs_ab_as
#define HIGGS_2LOOP_CORRECTION_AT_AT     loop_corrections.higgs_at_at
#define HIGGS_2LOOP_CORRECTION_ATAU_ATAU loop_corrections.higgs_atau_atau
#define TOP_POLE_QCD_CORRECTION          loop_corrections.top_qcd
#define HIGGS_3LOOP_CORRECTION_AT_AS_AS  loop_corrections.higgs_at_as_as
#define HIGGS_3LOOP_CORRECTION_AB_AS_AS  loop_corrections.higgs_ab_as_as
#define HIGGS_3LOOP_MDR_SCHEME           loop_corrections.higgs_3L_mdr_scheme
#define HIGGS_3LOOP_CORRECTION_AT_AT_AS  loop_corrections.higgs_at_at_as
#define HIGGS_3LOOP_CORRECTION_AT_AT_AT  loop_corrections.higgs_at_at_at

CLASSNAME::HSSUSY_mass_eigenstates(const HSSUSY_input_parameters& input_)
   : HSSUSY_soft_parameters(input_)
#if defined(ENABLE_TWO_SCALE_SOLVER)
   , ewsb_solver(new HSSUSY_ewsb_solver<Two_scale>())
#endif
{
}

void CLASSNAME::do_calculate_sm_pole_masses(bool flag)
{
   calculate_sm_pole_masses = flag;
}

bool CLASSNAME::do_calculate_sm_pole_masses() const
{
   return calculate_sm_pole_masses;
}

void CLASSNAME::do_calculate_bsm_pole_masses(bool flag)
{
   calculate_bsm_pole_masses = flag;
}

bool CLASSNAME::do_calculate_bsm_pole_masses() const
{
   return calculate_bsm_pole_masses;
}

void CLASSNAME::do_force_output(bool flag)
{
   force_output = flag;
}

bool CLASSNAME::do_force_output() const
{
   return force_output;
}

void CLASSNAME::set_ewsb_loop_order(int loop_order)
{
   ewsb_loop_order = loop_order;
   if (ewsb_solver) {
      ewsb_solver->set_loop_order(ewsb_loop_order);
   }
}

void CLASSNAME::set_loop_corrections(const Loop_corrections& loop_corrections_)
{
   loop_corrections = loop_corrections_;
}

const Loop_corrections& CLASSNAME::get_loop_corrections() const
{
   return loop_corrections;
}

void CLASSNAME::set_threshold_corrections(const Threshold_corrections& tc)
{
   threshold_corrections = tc;
}

const Threshold_corrections& CLASSNAME::get_threshold_corrections() const
{
   return threshold_corrections;
}

int CLASSNAME::get_number_of_ewsb_iterations() const
{
   return static_cast<int>(std::abs(-log10(ewsb_iteration_precision) * 10));
}

int CLASSNAME::get_number_of_mass_iterations() const
{
   return static_cast<int>(std::abs(-log10(precision) * 10));
}

void CLASSNAME::set_precision(double precision_)
{
   precision = precision_;
   ewsb_iteration_precision = precision_;
   if (ewsb_solver) {
      ewsb_solver->set_precision(precision_);
   }
}

void CLASSNAME::set_pole_mass_loop_order(int loop_order)
{
   pole_mass_loop_order = loop_order;
}

int CLASSNAME::get_pole_mass_loop_order() const
{
   return pole_mass_loop_order;
}

void CLASSNAME::set_ewsb_iteration_precision(double precision)
{
   ewsb_iteration_precision = precision;
   if (ewsb_solver) {
      ewsb_solver->set_precision(precision);
   }
}

double CLASSNAME::get_ewsb_iteration_precision() const
{
   return ewsb_iteration_precision;
}

double CLASSNAME::get_precision() const
{
   return precision;
}

double CLASSNAME::get_ewsb_loop_order() const
{
   return ewsb_loop_order;
}

const HSSUSY_physical& CLASSNAME::get_physical() const
{
   return physical;
}

HSSUSY_physical& CLASSNAME::get_physical()
{
   return physical;
}

void CLASSNAME::set_physical(const HSSUSY_physical& physical_)
{
   physical = physical_;
}

const Problems& CLASSNAME::get_problems() const
{
   return problems;
}

Problems& CLASSNAME::get_problems()
{
   return problems;
}

void CLASSNAME::set_ewsb_solver(const std::shared_ptr<HSSUSY_ewsb_solver_interface>& solver)
{
   ewsb_solver = solver;
}

/**
 * Method which calculates the tadpoles at the current loop order.
 *
 * @param tadpole array of tadpole
 */
void CLASSNAME::tadpole_equations(double tadpole[number_of_ewsb_equations]) const
{
   const auto tadpole_(tadpole_equations());
   std::copy(tadpole_.data(), tadpole_.data() + number_of_ewsb_equations, tadpole);
}

/**
 * Method which calculates the tadpoles at the current loop order.
 *
 * @return array of tadpoles
 */
Eigen::Matrix<double,CLASSNAME::number_of_ewsb_equations,1> CLASSNAME::tadpole_equations() const
{
   Eigen::Matrix<double,number_of_ewsb_equations,1> tadpole(
      Eigen::Matrix<double,number_of_ewsb_equations,1>::Zero());

   tadpole[0] = get_ewsb_eq_hh_1();

   if (ewsb_loop_order > 0) {
      tadpole[0] -= Re(tadpole_hh_1loop());

      if (ewsb_loop_order > 1) {

      }
   }

   return tadpole;
}

/**
 * This function returns the vector of tadpoles, each divided by the
 * corresponding VEV.  Thus, the returned tadpoles have the dimension
 * GeV^2 each.
 *
 * @return vector of tadpoles
 */
Eigen::Matrix<double,CLASSNAME::number_of_ewsb_equations,1> CLASSNAME::tadpole_equations_over_vevs() const
{
   auto tadpole = tadpole_equations();

   tadpole[0] /= v;


   return tadpole;
}

int CLASSNAME::solve_ewsb_tree_level_custom()
{
   int error = EWSB_solver::SUCCESS;



   return error;
}

int CLASSNAME::solve_ewsb_tree_level()
{
   if (!ewsb_solver) {
      throw SetupError("HSSUSY_mass_eigenstates::solve_ewsb_tree_level: "
                       "no EWSB solver set");
   }

   const int old_loop_order = ewsb_solver->get_loop_order();
   const auto save_loop_order = make_raii_guard(
      [this, old_loop_order] () {
         this->ewsb_solver->set_loop_order(old_loop_order);
      });

   const int old_iterations = ewsb_solver->get_number_of_iterations();
   const auto save_iterations = make_raii_guard(
      [this, old_iterations] () {
         this->ewsb_solver->set_number_of_iterations(old_iterations);
      });

   const double old_precision = ewsb_solver->get_precision();
   const auto save_precision = make_raii_guard(
      [this, old_precision] () {
         this->ewsb_solver->set_precision(old_precision);
      });

   ewsb_solver->set_loop_order(0);
   ewsb_solver->set_number_of_iterations(get_number_of_ewsb_iterations());
   ewsb_solver->set_precision(ewsb_iteration_precision);

   return ewsb_solver->solve(*this);
}

int CLASSNAME::solve_ewsb_one_loop()
{
   if (!ewsb_solver) {
      throw SetupError("HSSUSY_mass_eigenstates::solve_ewsb_one_loop: "
                       "no EWSB solver set");
   }

   const int old_loop_order = ewsb_solver->get_loop_order();
   const auto save_loop_order = make_raii_guard(
      [this, old_loop_order] () {
         this->ewsb_solver->set_loop_order(old_loop_order);
      });

   const int old_iterations = ewsb_solver->get_number_of_iterations();
   const auto save_iterations = make_raii_guard(
      [this, old_iterations] () {
         this->ewsb_solver->set_number_of_iterations(old_iterations);
      });

   const double old_precision = ewsb_solver->get_precision();
   const auto save_precision = make_raii_guard(
      [this, old_precision] () {
         this->ewsb_solver->set_precision(old_precision);
      });

   ewsb_solver->set_loop_order(1);
   ewsb_solver->set_number_of_iterations(get_number_of_ewsb_iterations());
   ewsb_solver->set_precision(ewsb_iteration_precision);

   return ewsb_solver->solve(*this);
}

int CLASSNAME::solve_ewsb()
{
   if (!ewsb_solver) {
      throw SetupError("HSSUSY_mass_eigenstates::solve_ewsb: "
                       "no EWSB solver set");
   }

   VERBOSE_MSG("\t\tSolving HSSUSY EWSB at " << ewsb_loop_order << "-loop order");

   const int old_loop_order = ewsb_solver->get_loop_order();
   const auto save_loop_order = make_raii_guard(
      [this, old_loop_order] () {
         this->ewsb_solver->set_loop_order(old_loop_order);
      });

   const int old_iterations = ewsb_solver->get_number_of_iterations();
   const auto save_iterations = make_raii_guard(
      [this, old_iterations] () {
         this->ewsb_solver->set_number_of_iterations(old_iterations);
      });

   const double old_precision = ewsb_solver->get_precision();
   const auto save_precision = make_raii_guard(
      [this, old_precision] () {
         this->ewsb_solver->set_precision(old_precision);
      });

   ewsb_solver->set_loop_order(ewsb_loop_order);
   ewsb_solver->set_number_of_iterations(get_number_of_ewsb_iterations());
   ewsb_solver->set_precision(ewsb_iteration_precision);

   return ewsb_solver->solve(*this);
}

void CLASSNAME::print(std::ostream& ostr) const
{
   ostr << "========================================\n"
           "HSSUSY\n"
           "========================================\n";
   HSSUSY_soft_parameters::print(ostr);
   ostr << "----------------------------------------\n"
           "tree-level MSbar masses:\n"
           "----------------------------------------\n";
   ostr << "MVG = " << MVG << '\n';
   ostr << "MHp = " << MHp << '\n';
   ostr << "MFv = " << MFv.transpose() << '\n';
   ostr << "MAh = " << MAh << '\n';
   ostr << "Mhh = " << Mhh << '\n';
   ostr << "MFd = " << MFd.transpose() << '\n';
   ostr << "MFu = " << MFu.transpose() << '\n';
   ostr << "MFe = " << MFe.transpose() << '\n';
   ostr << "MVWp = " << MVWp << '\n';
   ostr << "MVP = " << MVP << '\n';
   ostr << "MVZ = " << MVZ << '\n';

   ostr << "----------------------------------------\n"
           "tree-level MSbar mixing matrices:\n"
           "----------------------------------------\n";
   ostr << "Vd = " << Vd << '\n';
   ostr << "Ud = " << Ud << '\n';
   ostr << "Vu = " << Vu << '\n';
   ostr << "Uu = " << Uu << '\n';
   ostr << "Ve = " << Ve << '\n';
   ostr << "Ue = " << Ue << '\n';
   ostr << "ZZ = " << ZZ << '\n';

   physical.print(ostr);
}

/**
 * wrapper routines for passarino Veltman functions
 * @note: They take squared arguments!
 */

double CLASSNAME::A0(double m) const noexcept
{
   return passarino_veltman::ReA0(m, Sqr(get_scale()));
}

double CLASSNAME::B0(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReB0(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::B1(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReB1(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::B00(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReB00(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::B22(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReB22(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::H0(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReH0(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::F0(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReF0(p, m1, m2, Sqr(get_scale()));
}

double CLASSNAME::G0(double p, double m1, double m2) const noexcept
{
   return passarino_veltman::ReG0(p, m1, m2, Sqr(get_scale()));
}

/**
 * routine which finds the MSbar mass eigenstates and mixings.
 */
void CLASSNAME::calculate_DRbar_masses()
{
   const auto save_mu2_raii = make_raii_save(mu2);

   const bool has_no_ewsb_flag = problems.no_ewsb();
   const auto save_ewsb_flag = make_raii_guard(
      [this, has_no_ewsb_flag] () {
         if (has_no_ewsb_flag) {
            this->problems.flag_no_ewsb();
         } else {
            this->problems.unflag_no_ewsb();
         }
      }
   );
   problems.unflag_no_ewsb();
   solve_ewsb_tree_level();
#ifdef ENABLE_VERBOSE
   if (problems.no_ewsb()) {
      WARNING("solving EWSB at 0-loop order failed");
   }
#endif

   calculate_MVPVZ();
   calculate_MVWp();
   calculate_MFe();
   calculate_MFu();
   calculate_MFd();
   calculate_Mhh();
   calculate_MAh();
   calculate_MFv();
   calculate_MHp();
   calculate_MVG();

}

/**
 * routine which finds the pole mass eigenstates and mixings.
 */
void CLASSNAME::calculate_pole_masses()
{
#ifdef ENABLE_THREADS
   Thread_pool tp(std::min(std::thread::hardware_concurrency(), 9u));

   if (calculate_bsm_pole_masses) {
   }

   if (calculate_sm_pole_masses) {
      tp.run_task([this] () { calculate_MVG_pole(); });
      tp.run_task([this] () { calculate_MFv_pole(); });
      tp.run_task([this] () { calculate_Mhh_pole(); });
      tp.run_task([this] () { calculate_MVP_pole(); });
      tp.run_task([this] () { calculate_MVZ_pole(); });
      tp.run_task([this] () { calculate_MFd_pole(); });
      tp.run_task([this] () { calculate_MFu_pole(); });
      tp.run_task([this] () { calculate_MFe_pole(); });
      tp.run_task([this] () { calculate_MVWp_pole(); });
   }

#else
   if (calculate_bsm_pole_masses) {
   }

   if (calculate_sm_pole_masses) {
      calculate_MVG_pole();
      calculate_MFv_pole();
      calculate_Mhh_pole();
      calculate_MVP_pole();
      calculate_MVZ_pole();
      calculate_MFd_pole();
      calculate_MFu_pole();
      calculate_MFe_pole();
      calculate_MVWp_pole();
   }

#endif
}

void CLASSNAME::copy_DRbar_masses_to_pole_masses()
{
   PHYSICAL(MVG) = MVG;
   PHYSICAL(MHp) = MHp;
   PHYSICAL(MFv) = MFv;
   PHYSICAL(MAh) = MAh;
   PHYSICAL(Mhh) = Mhh;
   PHYSICAL(MFd) = MFd;
   PHYSICAL(Vd) = Vd;
   PHYSICAL(Ud) = Ud;
   PHYSICAL(MFu) = MFu;
   PHYSICAL(Vu) = Vu;
   PHYSICAL(Uu) = Uu;
   PHYSICAL(MFe) = MFe;
   PHYSICAL(Ve) = Ve;
   PHYSICAL(Ue) = Ue;
   PHYSICAL(MVWp) = MVWp;
   PHYSICAL(MVP) = MVP;
   PHYSICAL(MVZ) = MVZ;

}

/**
 * reorders MSbar masses so that golstones are placed at the index
 * specified in the model files definition of the associated
 * gauge boson (see Z-boson definition in default particles.m file
 * in the Models directory of your SARAH distribution for example)
 */
void CLASSNAME::reorder_DRbar_masses()
{

}

/**
 * reorders pole masses so that golstones are placed at the index
 * specified in the model files definition of the associated
 * gauge boson (see Z-boson definition in default particles.m file
 * in the Models directory of your SARAH distribution for example)
 */
void CLASSNAME::reorder_pole_masses()
{

}

/**
 * Checks the pole masses for tachyons
 */
void CLASSNAME::check_pole_masses_for_tachyons()
{
   if (PHYSICAL(Mhh) < 0.) problems.flag_pole_tachyon(HSSUSY_info::hh);

}

/**
 * calculates spectrum for model once the MSbar parameters at
 * at low energies are known
 */
void CLASSNAME::calculate_spectrum()
{
   calculate_DRbar_masses();
   if (pole_mass_loop_order > 0)
      calculate_pole_masses();

   // move goldstone bosons to the front
   reorder_DRbar_masses();
   if (pole_mass_loop_order == 0)
      copy_DRbar_masses_to_pole_masses();
   else
      reorder_pole_masses();

   check_pole_masses_for_tachyons();

   if (problems.have_problem() && !force_output) {
      clear_DRbar_parameters();
      physical.clear();
   }
}

void CLASSNAME::clear_DRbar_parameters()
{
   MVG = 0.;
   MHp = 0.;
   MFv = Eigen::Matrix<double,3,1>::Zero();
   MAh = 0.;
   Mhh = 0.;
   MFd = Eigen::Matrix<double,3,1>::Zero();
   Vd = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   Ud = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   MFu = Eigen::Matrix<double,3,1>::Zero();
   Vu = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   Uu = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   MFe = Eigen::Matrix<double,3,1>::Zero();
   Ve = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   Ue = Eigen::Matrix<std::complex<double>,3,3>::Zero();
   MVWp = 0.;
   MVP = 0.;
   MVZ = 0.;



}

void CLASSNAME::clear_problems()
{
   problems.unflag_all_tachyons();
}

void CLASSNAME::clear()
{
   HSSUSY_soft_parameters::clear();
   clear_DRbar_parameters();
   physical.clear();
   problems.clear();
}

void CLASSNAME::set_DRbar_masses(const Eigen::ArrayXd& pars)
{
   MVG = pars(0);
   MHp = pars(1);
   MFv(0) = pars(2);
   MFv(1) = pars(3);
   MFv(2) = pars(4);
   MAh = pars(5);
   Mhh = pars(6);
   MFd(0) = pars(7);
   MFd(1) = pars(8);
   MFd(2) = pars(9);
   MFu(0) = pars(10);
   MFu(1) = pars(11);
   MFu(2) = pars(12);
   MFe(0) = pars(13);
   MFe(1) = pars(14);
   MFe(2) = pars(15);
   MVWp = pars(16);
   MVP = pars(17);
   MVZ = pars(18);

}

Eigen::ArrayXd CLASSNAME::get_DRbar_masses() const
{
   Eigen::ArrayXd pars(19);

   pars(0) = MVG;
   pars(1) = MHp;
   pars(2) = MFv(0);
   pars(3) = MFv(1);
   pars(4) = MFv(2);
   pars(5) = MAh;
   pars(6) = Mhh;
   pars(7) = MFd(0);
   pars(8) = MFd(1);
   pars(9) = MFd(2);
   pars(10) = MFu(0);
   pars(11) = MFu(1);
   pars(12) = MFu(2);
   pars(13) = MFe(0);
   pars(14) = MFe(1);
   pars(15) = MFe(2);
   pars(16) = MVWp;
   pars(17) = MVP;
   pars(18) = MVZ;

   return pars;
}

void CLASSNAME::set_DRbar_masses_and_mixings(const Eigen::ArrayXd& pars)
{
   set_DRbar_masses(pars);

   Vd(0,0) = std::complex<double>(pars(19), pars(20));
   Vd(0,1) = std::complex<double>(pars(21), pars(22));
   Vd(0,2) = std::complex<double>(pars(23), pars(24));
   Vd(1,0) = std::complex<double>(pars(25), pars(26));
   Vd(1,1) = std::complex<double>(pars(27), pars(28));
   Vd(1,2) = std::complex<double>(pars(29), pars(30));
   Vd(2,0) = std::complex<double>(pars(31), pars(32));
   Vd(2,1) = std::complex<double>(pars(33), pars(34));
   Vd(2,2) = std::complex<double>(pars(35), pars(36));
   Ud(0,0) = std::complex<double>(pars(37), pars(38));
   Ud(0,1) = std::complex<double>(pars(39), pars(40));
   Ud(0,2) = std::complex<double>(pars(41), pars(42));
   Ud(1,0) = std::complex<double>(pars(43), pars(44));
   Ud(1,1) = std::complex<double>(pars(45), pars(46));
   Ud(1,2) = std::complex<double>(pars(47), pars(48));
   Ud(2,0) = std::complex<double>(pars(49), pars(50));
   Ud(2,1) = std::complex<double>(pars(51), pars(52));
   Ud(2,2) = std::complex<double>(pars(53), pars(54));
   Vu(0,0) = std::complex<double>(pars(55), pars(56));
   Vu(0,1) = std::complex<double>(pars(57), pars(58));
   Vu(0,2) = std::complex<double>(pars(59), pars(60));
   Vu(1,0) = std::complex<double>(pars(61), pars(62));
   Vu(1,1) = std::complex<double>(pars(63), pars(64));
   Vu(1,2) = std::complex<double>(pars(65), pars(66));
   Vu(2,0) = std::complex<double>(pars(67), pars(68));
   Vu(2,1) = std::complex<double>(pars(69), pars(70));
   Vu(2,2) = std::complex<double>(pars(71), pars(72));
   Uu(0,0) = std::complex<double>(pars(73), pars(74));
   Uu(0,1) = std::complex<double>(pars(75), pars(76));
   Uu(0,2) = std::complex<double>(pars(77), pars(78));
   Uu(1,0) = std::complex<double>(pars(79), pars(80));
   Uu(1,1) = std::complex<double>(pars(81), pars(82));
   Uu(1,2) = std::complex<double>(pars(83), pars(84));
   Uu(2,0) = std::complex<double>(pars(85), pars(86));
   Uu(2,1) = std::complex<double>(pars(87), pars(88));
   Uu(2,2) = std::complex<double>(pars(89), pars(90));
   Ve(0,0) = std::complex<double>(pars(91), pars(92));
   Ve(0,1) = std::complex<double>(pars(93), pars(94));
   Ve(0,2) = std::complex<double>(pars(95), pars(96));
   Ve(1,0) = std::complex<double>(pars(97), pars(98));
   Ve(1,1) = std::complex<double>(pars(99), pars(100));
   Ve(1,2) = std::complex<double>(pars(101), pars(102));
   Ve(2,0) = std::complex<double>(pars(103), pars(104));
   Ve(2,1) = std::complex<double>(pars(105), pars(106));
   Ve(2,2) = std::complex<double>(pars(107), pars(108));
   Ue(0,0) = std::complex<double>(pars(109), pars(110));
   Ue(0,1) = std::complex<double>(pars(111), pars(112));
   Ue(0,2) = std::complex<double>(pars(113), pars(114));
   Ue(1,0) = std::complex<double>(pars(115), pars(116));
   Ue(1,1) = std::complex<double>(pars(117), pars(118));
   Ue(1,2) = std::complex<double>(pars(119), pars(120));
   Ue(2,0) = std::complex<double>(pars(121), pars(122));
   Ue(2,1) = std::complex<double>(pars(123), pars(124));
   Ue(2,2) = std::complex<double>(pars(125), pars(126));
   ZZ(0,0) = pars(127);
   ZZ(0,1) = pars(128);
   ZZ(1,0) = pars(129);
   ZZ(1,1) = pars(130);

}

Eigen::ArrayXd CLASSNAME::get_DRbar_masses_and_mixings() const
{
   Eigen::ArrayXd pars(get_DRbar_masses());

   pars.conservativeResize(131);

   pars(19) = Re(Vd(0,0));
   pars(20) = Im(Vd(0,0));
   pars(21) = Re(Vd(0,1));
   pars(22) = Im(Vd(0,1));
   pars(23) = Re(Vd(0,2));
   pars(24) = Im(Vd(0,2));
   pars(25) = Re(Vd(1,0));
   pars(26) = Im(Vd(1,0));
   pars(27) = Re(Vd(1,1));
   pars(28) = Im(Vd(1,1));
   pars(29) = Re(Vd(1,2));
   pars(30) = Im(Vd(1,2));
   pars(31) = Re(Vd(2,0));
   pars(32) = Im(Vd(2,0));
   pars(33) = Re(Vd(2,1));
   pars(34) = Im(Vd(2,1));
   pars(35) = Re(Vd(2,2));
   pars(36) = Im(Vd(2,2));
   pars(37) = Re(Ud(0,0));
   pars(38) = Im(Ud(0,0));
   pars(39) = Re(Ud(0,1));
   pars(40) = Im(Ud(0,1));
   pars(41) = Re(Ud(0,2));
   pars(42) = Im(Ud(0,2));
   pars(43) = Re(Ud(1,0));
   pars(44) = Im(Ud(1,0));
   pars(45) = Re(Ud(1,1));
   pars(46) = Im(Ud(1,1));
   pars(47) = Re(Ud(1,2));
   pars(48) = Im(Ud(1,2));
   pars(49) = Re(Ud(2,0));
   pars(50) = Im(Ud(2,0));
   pars(51) = Re(Ud(2,1));
   pars(52) = Im(Ud(2,1));
   pars(53) = Re(Ud(2,2));
   pars(54) = Im(Ud(2,2));
   pars(55) = Re(Vu(0,0));
   pars(56) = Im(Vu(0,0));
   pars(57) = Re(Vu(0,1));
   pars(58) = Im(Vu(0,1));
   pars(59) = Re(Vu(0,2));
   pars(60) = Im(Vu(0,2));
   pars(61) = Re(Vu(1,0));
   pars(62) = Im(Vu(1,0));
   pars(63) = Re(Vu(1,1));
   pars(64) = Im(Vu(1,1));
   pars(65) = Re(Vu(1,2));
   pars(66) = Im(Vu(1,2));
   pars(67) = Re(Vu(2,0));
   pars(68) = Im(Vu(2,0));
   pars(69) = Re(Vu(2,1));
   pars(70) = Im(Vu(2,1));
   pars(71) = Re(Vu(2,2));
   pars(72) = Im(Vu(2,2));
   pars(73) = Re(Uu(0,0));
   pars(74) = Im(Uu(0,0));
   pars(75) = Re(Uu(0,1));
   pars(76) = Im(Uu(0,1));
   pars(77) = Re(Uu(0,2));
   pars(78) = Im(Uu(0,2));
   pars(79) = Re(Uu(1,0));
   pars(80) = Im(Uu(1,0));
   pars(81) = Re(Uu(1,1));
   pars(82) = Im(Uu(1,1));
   pars(83) = Re(Uu(1,2));
   pars(84) = Im(Uu(1,2));
   pars(85) = Re(Uu(2,0));
   pars(86) = Im(Uu(2,0));
   pars(87) = Re(Uu(2,1));
   pars(88) = Im(Uu(2,1));
   pars(89) = Re(Uu(2,2));
   pars(90) = Im(Uu(2,2));
   pars(91) = Re(Ve(0,0));
   pars(92) = Im(Ve(0,0));
   pars(93) = Re(Ve(0,1));
   pars(94) = Im(Ve(0,1));
   pars(95) = Re(Ve(0,2));
   pars(96) = Im(Ve(0,2));
   pars(97) = Re(Ve(1,0));
   pars(98) = Im(Ve(1,0));
   pars(99) = Re(Ve(1,1));
   pars(100) = Im(Ve(1,1));
   pars(101) = Re(Ve(1,2));
   pars(102) = Im(Ve(1,2));
   pars(103) = Re(Ve(2,0));
   pars(104) = Im(Ve(2,0));
   pars(105) = Re(Ve(2,1));
   pars(106) = Im(Ve(2,1));
   pars(107) = Re(Ve(2,2));
   pars(108) = Im(Ve(2,2));
   pars(109) = Re(Ue(0,0));
   pars(110) = Im(Ue(0,0));
   pars(111) = Re(Ue(0,1));
   pars(112) = Im(Ue(0,1));
   pars(113) = Re(Ue(0,2));
   pars(114) = Im(Ue(0,2));
   pars(115) = Re(Ue(1,0));
   pars(116) = Im(Ue(1,0));
   pars(117) = Re(Ue(1,1));
   pars(118) = Im(Ue(1,1));
   pars(119) = Re(Ue(1,2));
   pars(120) = Im(Ue(1,2));
   pars(121) = Re(Ue(2,0));
   pars(122) = Im(Ue(2,0));
   pars(123) = Re(Ue(2,1));
   pars(124) = Im(Ue(2,1));
   pars(125) = Re(Ue(2,2));
   pars(126) = Im(Ue(2,2));
   pars(127) = ZZ(0,0);
   pars(128) = ZZ(0,1);
   pars(129) = ZZ(1,0);
   pars(130) = ZZ(1,1);


   return pars;
}

void CLASSNAME::set_extra_parameters(const Eigen::ArrayXd& pars)
{

}

Eigen::ArrayXd CLASSNAME::get_extra_parameters() const
{
   return Eigen::ArrayXd();

}

std::string CLASSNAME::name() const
{
   return "HSSUSY";
}

void CLASSNAME::run_to(double scale, double eps)
{
   if (eps < 0.0)
      eps = precision;
   HSSUSY_soft_parameters::run_to(scale, eps);
}







double CLASSNAME::get_mass_matrix_VG() const
{
   const double mass_matrix_VG = Re(0);

   return mass_matrix_VG;
}

void CLASSNAME::calculate_MVG()
{
   const auto mass_matrix_VG = get_mass_matrix_VG();
   MVG = mass_matrix_VG;
}

double CLASSNAME::get_mass_matrix_Hp() const
{
   const double mass_matrix_Hp = Re(-mu2 + 0.5*Lambdax*Sqr(v) + 0.25*Sqr(
      g2)*Sqr(v));

   return mass_matrix_Hp;
}

void CLASSNAME::calculate_MHp()
{
   const auto mass_matrix_Hp = get_mass_matrix_Hp();
   MHp = mass_matrix_Hp;

   if (MHp < 0.) {
      problems.flag_running_tachyon(HSSUSY_info::Hp);
   }

   MHp = AbsSqrt(MHp);
}

Eigen::Matrix<double,3,3> CLASSNAME::get_mass_matrix_Fv() const
{
   Eigen::Matrix<double,3,3> mass_matrix_Fv;

   mass_matrix_Fv(0,0) = 0;
   mass_matrix_Fv(0,1) = 0;
   mass_matrix_Fv(0,2) = 0;
   mass_matrix_Fv(1,1) = 0;
   mass_matrix_Fv(1,2) = 0;
   mass_matrix_Fv(2,2) = 0;

   Symmetrize(mass_matrix_Fv);

   return mass_matrix_Fv;
}

void CLASSNAME::calculate_MFv()
{
   MFv.setConstant(0);
}

double CLASSNAME::get_mass_matrix_Ah() const
{
   const double mass_matrix_Ah = Re(0.25*(-4*mu2 + 2*Lambdax*Sqr(v) + Sqr
      (v)*Sqr(g2*Cos(ThetaW()) + 0.7745966692414834*g1*Sin(ThetaW()))));

   return mass_matrix_Ah;
}

void CLASSNAME::calculate_MAh()
{
   const auto mass_matrix_Ah = get_mass_matrix_Ah();
   MAh = mass_matrix_Ah;

   if (MAh < 0.) {
      problems.flag_running_tachyon(HSSUSY_info::Ah);
   }

   MAh = AbsSqrt(MAh);
}

double CLASSNAME::get_mass_matrix_hh() const
{
   const double mass_matrix_hh = Re(-mu2 + 1.5*Lambdax*Sqr(v));

   return mass_matrix_hh;
}

void CLASSNAME::calculate_Mhh()
{
   const auto mass_matrix_hh = get_mass_matrix_hh();
   Mhh = mass_matrix_hh;

   if (Mhh < 0.) {
      problems.flag_running_tachyon(HSSUSY_info::hh);
   }

   Mhh = AbsSqrt(Mhh);
}

Eigen::Matrix<double,3,3> CLASSNAME::get_mass_matrix_Fd() const
{
   Eigen::Matrix<double,3,3> mass_matrix_Fd;

   mass_matrix_Fd(0,0) = 0.7071067811865475*v*Yd(0,0);
   mass_matrix_Fd(0,1) = 0.7071067811865475*v*Yd(1,0);
   mass_matrix_Fd(0,2) = 0.7071067811865475*v*Yd(2,0);
   mass_matrix_Fd(1,0) = 0.7071067811865475*v*Yd(0,1);
   mass_matrix_Fd(1,1) = 0.7071067811865475*v*Yd(1,1);
   mass_matrix_Fd(1,2) = 0.7071067811865475*v*Yd(2,1);
   mass_matrix_Fd(2,0) = 0.7071067811865475*v*Yd(0,2);
   mass_matrix_Fd(2,1) = 0.7071067811865475*v*Yd(1,2);
   mass_matrix_Fd(2,2) = 0.7071067811865475*v*Yd(2,2);

   return mass_matrix_Fd;
}

void CLASSNAME::calculate_MFd()
{
   const auto mass_matrix_Fd(get_mass_matrix_Fd());


#ifdef CHECK_EIGENVALUE_ERROR
   double eigenvalue_error;
   fs_svd(mass_matrix_Fd, MFd, Vd, Ud, eigenvalue_error);
   problems.flag_bad_mass(HSSUSY_info::Fd, eigenvalue_error > precision *
      Abs(MFd(0)));
#else
   fs_svd(mass_matrix_Fd, MFd, Vd, Ud);
#endif

}

Eigen::Matrix<double,3,3> CLASSNAME::get_mass_matrix_Fu() const
{
   Eigen::Matrix<double,3,3> mass_matrix_Fu;

   mass_matrix_Fu(0,0) = -0.7071067811865475*v*Yu(0,0);
   mass_matrix_Fu(0,1) = -0.7071067811865475*v*Yu(1,0);
   mass_matrix_Fu(0,2) = -0.7071067811865475*v*Yu(2,0);
   mass_matrix_Fu(1,0) = -0.7071067811865475*v*Yu(0,1);
   mass_matrix_Fu(1,1) = -0.7071067811865475*v*Yu(1,1);
   mass_matrix_Fu(1,2) = -0.7071067811865475*v*Yu(2,1);
   mass_matrix_Fu(2,0) = -0.7071067811865475*v*Yu(0,2);
   mass_matrix_Fu(2,1) = -0.7071067811865475*v*Yu(1,2);
   mass_matrix_Fu(2,2) = -0.7071067811865475*v*Yu(2,2);

   return mass_matrix_Fu;
}

void CLASSNAME::calculate_MFu()
{
   const auto mass_matrix_Fu(get_mass_matrix_Fu());


#ifdef CHECK_EIGENVALUE_ERROR
   double eigenvalue_error;
   fs_svd(mass_matrix_Fu, MFu, Vu, Uu, eigenvalue_error);
   problems.flag_bad_mass(HSSUSY_info::Fu, eigenvalue_error > precision *
      Abs(MFu(0)));
#else
   fs_svd(mass_matrix_Fu, MFu, Vu, Uu);
#endif

}

Eigen::Matrix<double,3,3> CLASSNAME::get_mass_matrix_Fe() const
{
   Eigen::Matrix<double,3,3> mass_matrix_Fe;

   mass_matrix_Fe(0,0) = 0.7071067811865475*v*Ye(0,0);
   mass_matrix_Fe(0,1) = 0.7071067811865475*v*Ye(1,0);
   mass_matrix_Fe(0,2) = 0.7071067811865475*v*Ye(2,0);
   mass_matrix_Fe(1,0) = 0.7071067811865475*v*Ye(0,1);
   mass_matrix_Fe(1,1) = 0.7071067811865475*v*Ye(1,1);
   mass_matrix_Fe(1,2) = 0.7071067811865475*v*Ye(2,1);
   mass_matrix_Fe(2,0) = 0.7071067811865475*v*Ye(0,2);
   mass_matrix_Fe(2,1) = 0.7071067811865475*v*Ye(1,2);
   mass_matrix_Fe(2,2) = 0.7071067811865475*v*Ye(2,2);

   return mass_matrix_Fe;
}

void CLASSNAME::calculate_MFe()
{
   const auto mass_matrix_Fe(get_mass_matrix_Fe());


#ifdef CHECK_EIGENVALUE_ERROR
   double eigenvalue_error;
   fs_svd(mass_matrix_Fe, MFe, Ve, Ue, eigenvalue_error);
   problems.flag_bad_mass(HSSUSY_info::Fe, eigenvalue_error > precision *
      Abs(MFe(0)));
#else
   fs_svd(mass_matrix_Fe, MFe, Ve, Ue);
#endif

}

double CLASSNAME::get_mass_matrix_VWp() const
{
   const double mass_matrix_VWp = Re(0.25*Sqr(g2)*Sqr(v));

   return mass_matrix_VWp;
}

void CLASSNAME::calculate_MVWp()
{
   const auto mass_matrix_VWp = get_mass_matrix_VWp();
   MVWp = mass_matrix_VWp;

   if (MVWp < 0.) {
      problems.flag_running_tachyon(HSSUSY_info::VWp);
   }

   MVWp = AbsSqrt(MVWp);
}

Eigen::Matrix<double,2,2> CLASSNAME::get_mass_matrix_VPVZ() const
{
   Eigen::Matrix<double,2,2> mass_matrix_VPVZ;

   mass_matrix_VPVZ(0,0) = 0.15*Sqr(g1)*Sqr(v);
   mass_matrix_VPVZ(0,1) = -0.19364916731037085*g1*g2*Sqr(v);
   mass_matrix_VPVZ(1,1) = 0.25*Sqr(g2)*Sqr(v);

   Symmetrize(mass_matrix_VPVZ);

   return mass_matrix_VPVZ;
}

void CLASSNAME::calculate_MVPVZ()
{
   const auto mass_matrix_VPVZ(get_mass_matrix_VPVZ());
   Eigen::Array<double,2,1> MVPVZ;


#ifdef CHECK_EIGENVALUE_ERROR
   double eigenvalue_error;
   fs_diagonalize_hermitian(mass_matrix_VPVZ, MVPVZ, ZZ, eigenvalue_error
      );
#else
   fs_diagonalize_hermitian(mass_matrix_VPVZ, MVPVZ, ZZ);
#endif
   ZZ.transposeInPlace();
   normalize_to_interval(ZZ);


   MVPVZ = AbsSqrt(MVPVZ);

   MVP = 0.;
   MVZ = MVPVZ(1);
}


double CLASSNAME::get_ewsb_eq_hh_1() const
{
   double result = Re(-(mu2*v) + 0.5*Cube(v)*Lambdax);

   return result;
}



double CLASSNAME::CpHpconjHpconjVWpVWp() const
{
   const double result = 0.5*Sqr(g2);

   return result;
}

std::complex<double> CLASSNAME::CpHpconjHpVZVZ() const
{
   const std::complex<double> result = 0.1*(-7.745966692414834*g1*g2*Cos(ThetaW
      ())*Sin(ThetaW()) + 5*Sqr(g2)*Sqr(Cos(ThetaW())) + 3*Sqr(g1)*Sqr(Sin(ThetaW(
      ))));

   return result;
}

double CLASSNAME::CpAhAhAhAh() const
{
   const double result = -3*Lambdax;

   return result;
}

double CLASSNAME::CpAhAhhhhh() const
{
   const double result = -Lambdax;

   return result;
}

double CLASSNAME::CpAhAhconjVWpVWp() const
{
   const double result = 0.5*Sqr(g2);

   return result;
}

std::complex<double> CLASSNAME::CpAhAhVZVZ() const
{
   const std::complex<double> result = 0.1*(g1*Sin(ThetaW())*(7.745966692414834
      *g2*Cos(ThetaW()) + 3*g1*Sin(ThetaW())) + 5*Sqr(g2)*Sqr(Cos(ThetaW())));

   return result;
}

double CLASSNAME::Cphhhhhh() const
{
   const double result = -3*v*Lambdax;

   return result;
}

double CLASSNAME::CphhVZVZ() const
{
   const double result = 0.5*v*Sqr(g2*Cos(ThetaW()) + 0.7745966692414834*g1*Sin
      (ThetaW()));

   return result;
}

double CLASSNAME::CphhconjVWpVWp() const
{
   const double result = 0.5*v*Sqr(g2);

   return result;
}

double CLASSNAME::Cphhhhhhhh() const
{
   const double result = -3*Lambdax;

   return result;
}

double CLASSNAME::CphhhhconjVWpVWp() const
{
   const double result = 0.5*Sqr(g2);

   return result;
}

std::complex<double> CLASSNAME::CphhhhVZVZ() const
{
   const std::complex<double> result = 0.1*(g1*Sin(ThetaW())*(7.745966692414834
      *g2*Cos(ThetaW()) + 3*g1*Sin(ThetaW())) + 5*Sqr(g2)*Sqr(Cos(ThetaW())));

   return result;
}

std::complex<double> CLASSNAME::CpbarUFdFdAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = IF(gO2 < 3,std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Vd(gI1,j2))*Yd(gO2,j2)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarUFdFdAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = IF(gO1 < 3,std::complex<double>(0.,
      -0.7071067811865475)*SUM(j1,0,2,Conj(Yd(j1,gO1))*Ud(gI1,j1)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarUFuFuAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = IF(gO2 < 3,std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Vu(gI1,j2))*Yu(gO2,j2)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarUFuFuAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = IF(gO1 < 3,std::complex<double>(0.,
      -0.7071067811865475)*SUM(j1,0,2,Conj(Yu(j1,gO1))*Uu(gI1,j1)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarUFeFeAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = IF(gO2 < 3,std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Ve(gI1,j2))*Ye(gO2,j2)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarUFeFeAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = IF(gO1 < 3,std::complex<double>(0.,
      -0.7071067811865475)*SUM(j1,0,2,Conj(Ye(j1,gO1))*Ue(gI1,j1)),0);

   return result;
}

std::complex<double> CLASSNAME::CpbarFdFdAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Vd(gI1,j2))*SUM(j1,0,2,Conj(Ud(gO2,j1))*
      Yd(j1,j2)));

   return result;
}

std::complex<double> CLASSNAME::CpbarFdFdAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      -0.7071067811865475)*SUM(j2,0,2,SUM(j1,0,2,Conj(Yd(j1,j2))*Ud(gI1,j1))*Vd(
      gO1,j2));

   return result;
}

std::complex<double> CLASSNAME::CpbarFeFeAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Ve(gI1,j2))*SUM(j1,0,2,Conj(Ue(gO2,j1))*
      Ye(j1,j2)));

   return result;
}

std::complex<double> CLASSNAME::CpbarFeFeAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      -0.7071067811865475)*SUM(j2,0,2,SUM(j1,0,2,Conj(Ye(j1,j2))*Ue(gI1,j1))*Ve(
      gO1,j2));

   return result;
}

std::complex<double> CLASSNAME::CpbarFuFuAhPL(int gO2, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      0.7071067811865475)*SUM(j2,0,2,Conj(Vu(gI1,j2))*SUM(j1,0,2,Conj(Uu(gO2,j1))*
      Yu(j1,j2)));

   return result;
}

std::complex<double> CLASSNAME::CpbarFuFuAhPR(int gO1, int gI1) const
{
   const std::complex<double> result = std::complex<double>(0.,
      -0.7071067811865475)*SUM(j2,0,2,SUM(j1,0,2,Conj(Yu(j1,j2))*Uu(gI1,j1))*Vu(
      gO1,j2));

   return result;
}


std::complex<double> CLASSNAME::self_energy_Hp_1loop(double p ) const
{
   std::complex<double> result;

   result += AbsSqr(Cp(hh,Hp,Conj(Hp)))*B0(Sqr(p),Sqr(MHp),Sqr(Mhh));
   result += -(B0(Sqr(p),Sqr(MVZ),Sqr(MVWp))*Cp(bar(VWp),VZ,Conj(Hp))*Cp(bar(VZ
      ),VWp,Hp));
   result += -(B0(Sqr(p),Sqr(MVWp),Sqr(MVZ))*Cp(bar(VWp),VZ,Hp)*Cp(bar(VZ),VWp,
      Conj(Hp)));
   result += 2*AbsSqr(Cp(Conj(Hp),VP,VWp))*(-1 + 2*B0(Sqr(p),0,Sqr(MVWp)));
   result += 2*AbsSqr(Cp(Conj(Hp),VWp,VZ))*(-1 + 2*B0(Sqr(p),Sqr(MVWp),Sqr(MVZ)
      ));
   result += -0.5*A0(Sqr(MAh))*Cp(Ah,Ah,Hp,Conj(Hp));
   result += -0.5*A0(Sqr(Mhh))*Cp(hh,hh,Hp,Conj(Hp));
   result += -(A0(Sqr(MHp))*Cp(Hp,Hp,Conj(Hp),Conj(Hp)));
   result += AbsSqr(Cp(Ah,Conj(Hp),VWp))*F0(Sqr(p),Sqr(MAh),Sqr(MVWp));
   result += AbsSqr(Cp(hh,Conj(Hp),VWp))*F0(Sqr(p),Sqr(Mhh),Sqr(MVWp));
   result += AbsSqr(Cp(Hp,Conj(Hp),VP))*F0(Sqr(p),Sqr(MHp),0);
   result += AbsSqr(Cp(Hp,Conj(Hp),VZ))*F0(Sqr(p),Sqr(MHp),Sqr(MVZ));
   result += 4*A0(Sqr(MVWp))*CpHpconjHpconjVWpVWp() - 2*CpHpconjHpconjVWpVWp()*
      Sqr(MVWp);
   result += CpHpconjHpVZVZ()*(2*A0(Sqr(MVZ)) - Sqr(MVZ));
   result += -6*SUM(gI1,0,2,MFd(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI1)),Sqr(
      MFu(gI2)))*MFu(gI2)*(Conj(Cp(bar(Fd(gI1)),Fu(gI2),Conj(Hp))(PR))*Cp(bar(Fd(
      gI1)),Fu(gI2),Conj(Hp))(PL) + Conj(Cp(bar(Fd(gI1)),Fu(gI2),Conj(Hp))(PL))*Cp
      (bar(Fd(gI1)),Fu(gI2),Conj(Hp))(PR))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fd(gI1)),Fu(gI2),Conj(Hp)
      )(PL)) + AbsSqr(Cp(bar(Fd(gI1)),Fu(gI2),Conj(Hp))(PR)))*G0(Sqr(p),Sqr(MFd(
      gI1)),Sqr(MFu(gI2)))));
   result += -2*SUM(gI1,0,2,MFe(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFe(gI1)),Sqr(
      MFv(gI2)))*MFv(gI2)*(Conj(Cp(bar(Fe(gI1)),Fv(gI2),Conj(Hp))(PR))*Cp(bar(Fe(
      gI1)),Fv(gI2),Conj(Hp))(PL) + Conj(Cp(bar(Fe(gI1)),Fv(gI2),Conj(Hp))(PL))*Cp
      (bar(Fe(gI1)),Fv(gI2),Conj(Hp))(PR))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fe(gI1)),Fv(gI2),Conj(Hp))(
      PL)) + AbsSqr(Cp(bar(Fe(gI1)),Fv(gI2),Conj(Hp))(PR)))*G0(Sqr(p),Sqr(MFe(gI1)
      ),Sqr(MFv(gI2)))));

   return result * oneOver16PiSqr;

}

std::complex<double> CLASSNAME::self_energy_Ah_1loop(double p ) const
{
   std::complex<double> result;

   result += AbsSqr(Cp(Ah,Ah,hh))*B0(Sqr(p),Sqr(Mhh),Sqr(MAh));
   result += -2*B0(Sqr(p),Sqr(MVWp),Sqr(MVWp))*Sqr(Cp(bar(VWp),VWp,Ah));
   result += -(A0(Sqr(MHp))*Cp(Ah,Ah,Hp,Conj(Hp)));
   result += -0.5*A0(Sqr(MAh))*CpAhAhAhAh();
   result += -0.5*A0(Sqr(Mhh))*CpAhAhhhhh();
   result += AbsSqr(Cp(Ah,hh,VZ))*F0(Sqr(p),Sqr(Mhh),Sqr(MVZ));
   result += 2*AbsSqr(Cp(Ah,Hp,Conj(VWp)))*F0(Sqr(p),Sqr(MHp),Sqr(MVWp));
   result += 4*A0(Sqr(MVWp))*CpAhAhconjVWpVWp() - 2*CpAhAhconjVWpVWp()*Sqr(MVWp
      );
   result += CpAhAhVZVZ()*(2*A0(Sqr(MVZ)) - Sqr(MVZ));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(CpbarFdFdAhPL(gI1,gI2)) + AbsSqr
      (CpbarFdFdAhPR(gI1,gI2)))*G0(Sqr(p),Sqr(MFd(gI1)),Sqr(MFd(gI2)))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(CpbarFeFeAhPL(gI1,gI2)) + AbsSqr(
      CpbarFeFeAhPR(gI1,gI2)))*G0(Sqr(p),Sqr(MFe(gI1)),Sqr(MFe(gI2)))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(CpbarFuFuAhPL(gI1,gI2)) + AbsSqr
      (CpbarFuFuAhPR(gI1,gI2)))*G0(Sqr(p),Sqr(MFu(gI1)),Sqr(MFu(gI2)))));
   result += -6*SUM(gI1,0,2,MFd(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI1)),Sqr(
      MFd(gI2)))*(Conj(CpbarFdFdAhPR(gI1,gI2))*CpbarFdFdAhPL(gI1,gI2) + Conj(
      CpbarFdFdAhPL(gI1,gI2))*CpbarFdFdAhPR(gI1,gI2))*MFd(gI2)));
   result += -2*SUM(gI1,0,2,MFe(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFe(gI1)),Sqr(
      MFe(gI2)))*(Conj(CpbarFeFeAhPR(gI1,gI2))*CpbarFeFeAhPL(gI1,gI2) + Conj(
      CpbarFeFeAhPL(gI1,gI2))*CpbarFeFeAhPR(gI1,gI2))*MFe(gI2)));
   result += -6*SUM(gI1,0,2,MFu(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI1)),Sqr(
      MFu(gI2)))*(Conj(CpbarFuFuAhPR(gI1,gI2))*CpbarFuFuAhPL(gI1,gI2) + Conj(
      CpbarFuFuAhPL(gI1,gI2))*CpbarFuFuAhPR(gI1,gI2))*MFu(gI2)));

   return result * oneOver16PiSqr;

}

std::complex<double> CLASSNAME::self_energy_hh_1loop(double p ) const
{
   std::complex<double> result;

   result += 0.5*AbsSqr(Cp(Ah,Ah,hh))*B0(Sqr(p),Sqr(MAh),Sqr(MAh));
   result += AbsSqr(Cp(hh,Hp,Conj(Hp)))*B0(Sqr(p),Sqr(MHp),Sqr(MHp));
   result += -2*B0(Sqr(p),Sqr(MVWp),Sqr(MVWp))*Sqr(Cp(bar(VWp),VWp,hh));
   result += -(B0(Sqr(p),Sqr(MVZ),Sqr(MVZ))*Sqr(Cp(bar(VZ),VZ,hh)));
   result += -(A0(Sqr(MHp))*Cp(hh,hh,Hp,Conj(Hp)));
   result += -0.5*A0(Sqr(MAh))*CpAhAhhhhh();
   result += 2*AbsSqr(CphhconjVWpVWp())*(-1 + 2*B0(Sqr(p),Sqr(MVWp),Sqr(MVWp)))
      ;
   result += 0.5*AbsSqr(Cphhhhhh())*B0(Sqr(p),Sqr(Mhh),Sqr(Mhh));
   result += -0.5*A0(Sqr(Mhh))*Cphhhhhhhh();
   result += AbsSqr(CphhVZVZ())*(-1 + 2*B0(Sqr(p),Sqr(MVZ),Sqr(MVZ)));
   result += AbsSqr(Cp(Ah,hh,VZ))*F0(Sqr(p),Sqr(MAh),Sqr(MVZ));
   result += 2*AbsSqr(Cp(hh,Hp,Conj(VWp)))*F0(Sqr(p),Sqr(MHp),Sqr(MVWp));
   result += 4*A0(Sqr(MVWp))*CphhhhconjVWpVWp() - 2*CphhhhconjVWpVWp()*Sqr(MVWp
      );
   result += CphhhhVZVZ()*(2*A0(Sqr(MVZ)) - Sqr(MVZ));
   result += -6*SUM(gI1,0,2,MFd(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI1)),Sqr(
      MFd(gI2)))*MFd(gI2)*(Conj(Cp(bar(Fd(gI1)),Fd(gI2),hh)(PR))*Cp(bar(Fd(gI1)),
      Fd(gI2),hh)(PL) + Conj(Cp(bar(Fd(gI1)),Fd(gI2),hh)(PL))*Cp(bar(Fd(gI1)),Fd(
      gI2),hh)(PR))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fd(gI1)),Fd(gI2),hh)(PL))
      + AbsSqr(Cp(bar(Fd(gI1)),Fd(gI2),hh)(PR)))*G0(Sqr(p),Sqr(MFd(gI1)),Sqr(MFd(
      gI2)))));
   result += -2*SUM(gI1,0,2,MFe(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFe(gI1)),Sqr(
      MFe(gI2)))*MFe(gI2)*(Conj(Cp(bar(Fe(gI1)),Fe(gI2),hh)(PR))*Cp(bar(Fe(gI1)),
      Fe(gI2),hh)(PL) + Conj(Cp(bar(Fe(gI1)),Fe(gI2),hh)(PL))*Cp(bar(Fe(gI1)),Fe(
      gI2),hh)(PR))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fe(gI1)),Fe(gI2),hh)(PL)) +
      AbsSqr(Cp(bar(Fe(gI1)),Fe(gI2),hh)(PR)))*G0(Sqr(p),Sqr(MFe(gI1)),Sqr(MFe(
      gI2)))));
   result += -6*SUM(gI1,0,2,MFu(gI1)*SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI1)),Sqr(
      MFu(gI2)))*MFu(gI2)*(Conj(Cp(bar(Fu(gI1)),Fu(gI2),hh)(PR))*Cp(bar(Fu(gI1)),
      Fu(gI2),hh)(PL) + Conj(Cp(bar(Fu(gI1)),Fu(gI2),hh)(PL))*Cp(bar(Fu(gI1)),Fu(
      gI2),hh)(PR))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fu(gI1)),Fu(gI2),hh)(PL))
      + AbsSqr(Cp(bar(Fu(gI1)),Fu(gI2),hh)(PR)))*G0(Sqr(p),Sqr(MFu(gI1)),Sqr(MFu(
      gI2)))));

   return result * oneOver16PiSqr;

}

std::complex<double> CLASSNAME::self_energy_VZ_1loop(double p ) const
{
   std::complex<double> result;

   result += -4*AbsSqr(Cp(Ah,hh,VZ))*B00(Sqr(p),Sqr(MAh),Sqr(Mhh));
   result += -4*AbsSqr(Cp(Hp,Conj(Hp),VZ))*B00(Sqr(p),Sqr(MHp),Sqr(MHp));
   result += 2*AbsSqr(Cp(Hp,Conj(VWp),VZ))*B0(Sqr(p),Sqr(MVWp),Sqr(MHp));
   result += 2*AbsSqr(Cp(bar(VWp),VWp,VZ))*B00(Sqr(p),Sqr(MVWp),Sqr(MVWp));
   result += 0.5*A0(Sqr(MAh))*CpAhAhVZVZ();
   result += 0.5*A0(Sqr(Mhh))*CphhhhVZVZ();
   result += AbsSqr(CphhVZVZ())*B0(Sqr(p),Sqr(MVZ),Sqr(Mhh));
   result += A0(Sqr(MHp))*CpHpconjHpVZVZ();
   result += -0.6666666666666666*AbsSqr(Cp(Conj(VWp),VWp,VZ))*(3*A0(Sqr(MVWp))
      + 15*B00(Sqr(p),Sqr(MVWp),Sqr(MVWp)) - 6*Sqr(MVWp) + Sqr(p) + 3*B0(Sqr(p),
      Sqr(MVWp),Sqr(MVWp))*(Sqr(MVWp) + 2*Sqr(p)));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fd(gI1)),Fd(gI2),VZ)(PL))
      + AbsSqr(Cp(bar(Fd(gI1)),Fd(gI2),VZ)(PR)))*H0(Sqr(p),Sqr(MFd(gI1)),Sqr(MFd(
      gI2))) + 4*B0(Sqr(p),Sqr(MFd(gI1)),Sqr(MFd(gI2)))*MFd(gI1)*MFd(gI2)*Re(Conj(
      Cp(bar(Fd(gI1)),Fd(gI2),VZ)(PL))*Cp(bar(Fd(gI1)),Fd(gI2),VZ)(PR))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fe(gI1)),Fe(gI2),VZ)(PL)) +
      AbsSqr(Cp(bar(Fe(gI1)),Fe(gI2),VZ)(PR)))*H0(Sqr(p),Sqr(MFe(gI1)),Sqr(MFe(
      gI2))) + 4*B0(Sqr(p),Sqr(MFe(gI1)),Sqr(MFe(gI2)))*MFe(gI1)*MFe(gI2)*Re(Conj(
      Cp(bar(Fe(gI1)),Fe(gI2),VZ)(PL))*Cp(bar(Fe(gI1)),Fe(gI2),VZ)(PR))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fu(gI1)),Fu(gI2),VZ)(PL))
      + AbsSqr(Cp(bar(Fu(gI1)),Fu(gI2),VZ)(PR)))*H0(Sqr(p),Sqr(MFu(gI1)),Sqr(MFu(
      gI2))) + 4*B0(Sqr(p),Sqr(MFu(gI1)),Sqr(MFu(gI2)))*MFu(gI1)*MFu(gI2)*Re(Conj(
      Cp(bar(Fu(gI1)),Fu(gI2),VZ)(PL))*Cp(bar(Fu(gI1)),Fu(gI2),VZ)(PR))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fv(gI1)),Fv(gI2),VZ)(PL)) +
      AbsSqr(Cp(bar(Fv(gI1)),Fv(gI2),VZ)(PR)))*H0(Sqr(p),Sqr(MFv(gI1)),Sqr(MFv(
      gI2))) + 4*B0(Sqr(p),Sqr(MFv(gI1)),Sqr(MFv(gI2)))*MFv(gI1)*MFv(gI2)*Re(Conj(
      Cp(bar(Fv(gI1)),Fv(gI2),VZ)(PL))*Cp(bar(Fv(gI1)),Fv(gI2),VZ)(PR))));
   result += 2*Sqr(MVWp)*Cp(Conj(VWp),VWp,VZ,VZ)(1);
   result += -(A0(Sqr(MVWp))*(4*Cp(Conj(VWp),VWp,VZ,VZ)(1) + Cp(Conj(VWp),VWp,
      VZ,VZ)(2) + Cp(Conj(VWp),VWp,VZ,VZ)(3)));

   return result * oneOver16PiSqr;

}

std::complex<double> CLASSNAME::self_energy_VWp_1loop(double p ) const
{
   std::complex<double> result;

   result += -4*AbsSqr(Cp(Ah,Hp,Conj(VWp)))*B00(Sqr(p),Sqr(MAh),Sqr(MHp));
   result += -4*AbsSqr(Cp(hh,Hp,Conj(VWp)))*B00(Sqr(p),Sqr(Mhh),Sqr(MHp));
   result += AbsSqr(Cp(Hp,Conj(VWp),VP))*B0(Sqr(p),0,Sqr(MHp));
   result += AbsSqr(Cp(Hp,Conj(VWp),VZ))*B0(Sqr(p),Sqr(MVZ),Sqr(MHp));
   result += AbsSqr(Cp(bar(VP),VWp,Conj(VWp)))*B00(Sqr(p),Sqr(MVWp),Sqr(MVP));
   result += AbsSqr(Cp(bar(VWp),VP,Conj(VWp)))*B00(Sqr(p),Sqr(MVP),Sqr(MVWp));
   result += AbsSqr(Cp(bar(VWp),VZ,Conj(VWp)))*B00(Sqr(p),Sqr(MVZ),Sqr(MVWp));
   result += AbsSqr(Cp(bar(VZ),VWp,Conj(VWp)))*B00(Sqr(p),Sqr(MVWp),Sqr(MVZ));
   result += 0.5*A0(Sqr(MAh))*CpAhAhconjVWpVWp();
   result += AbsSqr(CphhconjVWpVWp())*B0(Sqr(p),Sqr(MVWp),Sqr(Mhh));
   result += 0.5*A0(Sqr(Mhh))*CphhhhconjVWpVWp();
   result += A0(Sqr(MHp))*CpHpconjHpconjVWpVWp();
   result += -(AbsSqr(Cp(Conj(VWp),VP,VWp))*(A0(Sqr(MVWp)) + 10*B00(Sqr(p),Sqr(
      MVWp),0) - 2*Sqr(MVWp) + 0.6666666666666666*Sqr(p) + B0(Sqr(p),Sqr(MVWp),0)*
      (Sqr(MVWp) + 4*Sqr(p))));
   result += -(AbsSqr(Cp(Conj(VWp),VWp,VZ))*(A0(Sqr(MVWp)) + A0(Sqr(MVZ)) + 10*
      B00(Sqr(p),Sqr(MVZ),Sqr(MVWp)) - 2*(Sqr(MVWp) + Sqr(MVZ) -
      0.3333333333333333*Sqr(p)) + B0(Sqr(p),Sqr(MVZ),Sqr(MVWp))*(Sqr(MVWp) + Sqr(
      MVZ) + 4*Sqr(p))));
   result += 3*SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fd(gI1)),Fu(gI2),Conj(VWp
      ))(PL)) + AbsSqr(Cp(bar(Fd(gI1)),Fu(gI2),Conj(VWp))(PR)))*H0(Sqr(p),Sqr(MFd(
      gI1)),Sqr(MFu(gI2))) + 4*B0(Sqr(p),Sqr(MFd(gI1)),Sqr(MFu(gI2)))*MFd(gI1)*MFu
      (gI2)*Re(Conj(Cp(bar(Fd(gI1)),Fu(gI2),Conj(VWp))(PL))*Cp(bar(Fd(gI1)),Fu(gI2
      ),Conj(VWp))(PR))));
   result += SUM(gI1,0,2,SUM(gI2,0,2,(AbsSqr(Cp(bar(Fe(gI1)),Fv(gI2),Conj(VWp))
      (PL)) + AbsSqr(Cp(bar(Fe(gI1)),Fv(gI2),Conj(VWp))(PR)))*H0(Sqr(p),Sqr(MFe(
      gI1)),Sqr(MFv(gI2))) + 4*B0(Sqr(p),Sqr(MFe(gI1)),Sqr(MFv(gI2)))*MFe(gI1)*MFv
      (gI2)*Re(Conj(Cp(bar(Fe(gI1)),Fv(gI2),Conj(VWp))(PL))*Cp(bar(Fe(gI1)),Fv(gI2
      ),Conj(VWp))(PR))));
   result += 0;
   result += 0.5*(2*Sqr(MVZ)*Cp(Conj(VWp),VWp,VZ,VZ)(1) - A0(Sqr(MVZ))*(4*Cp(
      Conj(VWp),VWp,VZ,VZ)(1) + Cp(Conj(VWp),VWp,VZ,VZ)(2) + Cp(Conj(VWp),VWp,VZ,
      VZ)(3)));
   result += 2*Sqr(MVWp)*Cp(Conj(VWp),Conj(VWp),VWp,VWp)(2);
   result += -(A0(Sqr(MVWp))*(Cp(Conj(VWp),Conj(VWp),VWp,VWp)(1) + 4*Cp(Conj(
      VWp),Conj(VWp),VWp,VWp)(2) + Cp(Conj(VWp),Conj(VWp),VWp,VWp)(3)));

   return result * oneOver16PiSqr;

}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_1(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(CpbarUFdFdAhPL(
      gO2,gI1))*CpbarUFdFdAhPR(gO1,gI1)*MFd(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFd(gO2)
      ),Fd(gI2),hh)(PL))*MFd(gI2)*Cp(bar(UFd(gO1)),Fd(gI2),hh)(PR));
   result += -5.333333333333333*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),0))
      *Conj(Cp(bar(UFd(gO2)),Fd(gI2),VG)(PR))*MFd(gI2)*Cp(bar(UFd(gO1)),Fd(gI2),VG
      )(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),0))*Conj(Cp(bar(UFd
      (gO2)),Fd(gI2),VP)(PR))*MFd(gI2)*Cp(bar(UFd(gO1)),Fd(gI2),VP)(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(UFd(gO2)),Fd(gI2),VZ)(PR))*MFd(gI2)*Cp(bar(UFd(gO1)),Fd(gI2),VZ)(PL));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(UFd(gO2)
      ),Fu(gI2),Conj(Hp))(PL))*MFu(gI2)*Cp(bar(UFd(gO1)),Fu(gI2),Conj(Hp))(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(UFd(gO2)),Fu(gI2),Conj(VWp))(PR))*MFu(gI2)*Cp(bar(UFd(gO1)),Fu(gI2),
      Conj(VWp))(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_1(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_1(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_PR(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(
      CpbarUFdFdAhPR(gO2,gI1))*CpbarUFdFdAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFd
      (gO2)),Fd(gI2),hh)(PR))*Cp(bar(UFd(gO1)),Fd(gI2),hh)(PR));
   result += -1.3333333333333333*SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),0))
      *Conj(Cp(bar(UFd(gO2)),Fd(gI2),VG)(PL))*Cp(bar(UFd(gO1)),Fd(gI2),VG)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),0))*Conj(Cp(bar(UFd(
      gO2)),Fd(gI2),VP)(PL))*Cp(bar(UFd(gO1)),Fd(gI2),VP)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFd(gO2)),Fd(gI2),VZ)(PL))*Cp(bar(UFd(gO1)),Fd(gI2),VZ)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(UFd
      (gO2)),Fu(gI2),Conj(Hp))(PR))*Cp(bar(UFd(gO1)),Fu(gI2),Conj(Hp))(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFd(gO2)),Fu(gI2),Conj(VWp))(PL))*Cp(bar(UFd(gO1)),Fu(gI2),Conj(VWp))(PL
      ));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_PR(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_PR(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_PL(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(
      CpbarUFdFdAhPL(gO2,gI1))*CpbarUFdFdAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFd
      (gO2)),Fd(gI2),hh)(PL))*Cp(bar(UFd(gO1)),Fd(gI2),hh)(PL));
   result += -1.3333333333333333*SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),0))
      *Conj(Cp(bar(UFd(gO2)),Fd(gI2),VG)(PR))*Cp(bar(UFd(gO1)),Fd(gI2),VG)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),0))*Conj(Cp(bar(UFd(
      gO2)),Fd(gI2),VP)(PR))*Cp(bar(UFd(gO1)),Fd(gI2),VP)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFd(gO2)),Fd(gI2),VZ)(PR))*Cp(bar(UFd(gO1)),Fd(gI2),VZ)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(UFd
      (gO2)),Fu(gI2),Conj(Hp))(PL))*Cp(bar(UFd(gO1)),Fu(gI2),Conj(Hp))(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFd(gO2)),Fu(gI2),Conj(VWp))(PR))*Cp(bar(UFd(gO1)),Fu(gI2),Conj(VWp))(PR
      ));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_PL(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_PL(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_1(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(CpbarUFuFuAhPL(
      gO2,gI1))*CpbarUFuFuAhPR(gO1,gI1)*MFu(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu(gO2)
      ),Fd(gI2),Hp)(PL))*MFd(gI2)*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(UFu(gO2)),Fd(gI2),VWp)(PR))*MFd(gI2)*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PL))
      ;
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu(gO2)
      ),Fu(gI2),hh)(PL))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PR));
   result += -5.333333333333333*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),0))
      *Conj(Cp(bar(UFu(gO2)),Fu(gI2),VG)(PR))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),VG
      )(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),VP)(PR))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(UFu(gO2)),Fu(gI2),VZ)(PR))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_1(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_1(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PR(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarUFuFuAhPR(gO2,gI1))*CpbarUFuFuAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu
      (gO2)),Fd(gI2),Hp)(PR))*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFu(gO2)),Fd(gI2),VWp)(PL))*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),hh)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PR));
   result += -1.3333333333333333*SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))
      *Conj(Cp(bar(UFu(gO2)),Fu(gI2),VG)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),VG)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu(
      gO2)),Fu(gI2),VP)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFu(gO2)),Fu(gI2),VZ)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PR(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PR(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PL(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarUFuFuAhPL(gO2,gI1))*CpbarUFuFuAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu
      (gO2)),Fd(gI2),Hp)(PL))*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFu(gO2)),Fd(gI2),VWp)(PR))*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),hh)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PL));
   result += -1.3333333333333333*SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))
      *Conj(Cp(bar(UFu(gO2)),Fu(gI2),VG)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),VG)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu(
      gO2)),Fu(gI2),VP)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFu(gO2)),Fu(gI2),VZ)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PR));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PL(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PL(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_1(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(CpbarUFeFeAhPL(
      gO2,gI1))*CpbarUFeFeAhPR(gO1,gI1)*MFe(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFe(gO2)
      ),Fe(gI2),hh)(PL))*MFe(gI2)*Cp(bar(UFe(gO1)),Fe(gI2),hh)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFe(gI2)),0))*Conj(Cp(bar(UFe
      (gO2)),Fe(gI2),VP)(PR))*MFe(gI2)*Cp(bar(UFe(gO1)),Fe(gI2),VP)(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(UFe(gO2)),Fe(gI2),VZ)(PR))*MFe(gI2)*Cp(bar(UFe(gO1)),Fe(gI2),VZ)(PL));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(UFe(gO2)
      ),Fv(gI2),Conj(Hp))(PL))*MFv(gI2)*Cp(bar(UFe(gO1)),Fv(gI2),Conj(Hp))(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(UFe(gO2)),Fv(gI2),Conj(VWp))(PR))*MFv(gI2)*Cp(bar(UFe(gO1)),Fv(gI2),
      Conj(VWp))(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_1(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_1(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_PR(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(
      CpbarUFeFeAhPR(gO2,gI1))*CpbarUFeFeAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFe
      (gO2)),Fe(gI2),hh)(PR))*Cp(bar(UFe(gO1)),Fe(gI2),hh)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),0))*Conj(Cp(bar(UFe(
      gO2)),Fe(gI2),VP)(PL))*Cp(bar(UFe(gO1)),Fe(gI2),VP)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFe(gO2)),Fe(gI2),VZ)(PL))*Cp(bar(UFe(gO1)),Fe(gI2),VZ)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(UFe
      (gO2)),Fv(gI2),Conj(Hp))(PR))*Cp(bar(UFe(gO1)),Fv(gI2),Conj(Hp))(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFe(gO2)),Fv(gI2),Conj(VWp))(PL))*Cp(bar(UFe(gO1)),Fv(gI2),Conj(VWp))(PL
      ));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_PR(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_PR(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_PL(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(
      CpbarUFeFeAhPL(gO2,gI1))*CpbarUFeFeAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFe
      (gO2)),Fe(gI2),hh)(PL))*Cp(bar(UFe(gO1)),Fe(gI2),hh)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),0))*Conj(Cp(bar(UFe(
      gO2)),Fe(gI2),VP)(PR))*Cp(bar(UFe(gO1)),Fe(gI2),VP)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFe(gO2)),Fe(gI2),VZ)(PR))*Cp(bar(UFe(gO1)),Fe(gI2),VZ)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(UFe
      (gO2)),Fv(gI2),Conj(Hp))(PL))*Cp(bar(UFe(gO1)),Fv(gI2),Conj(Hp))(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFe(gO2)),Fv(gI2),Conj(VWp))(PR))*Cp(bar(UFe(gO1)),Fv(gI2),Conj(VWp))(PR
      ));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_PL(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_PL(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_1_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(CpbarFdFdAhPL(
      gO2,gI1))*CpbarFdFdAhPR(gO1,gI1)*MFd(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fd(gO2))
      ,Fd(gI2),hh)(PL))*MFd(gI2)*Cp(bar(Fd(gO1)),Fd(gI2),hh)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(Fd(gO2)),Fd(gI2),VZ)(PR))*MFd(gI2)*Cp(bar(Fd(gO1)),Fd(gI2),VZ)(PL));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(Fd(gO2))
      ,Fu(gI2),Conj(Hp))(PL))*MFu(gI2)*Cp(bar(Fd(gO1)),Fu(gI2),Conj(Hp))(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(Fd(gO2)),Fu(gI2),Conj(VWp))(PR))*MFu(gI2)*Cp(bar(Fd(gO1)),Fu(gI2),Conj(
      VWp))(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_1_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_1_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_PR_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(
      CpbarFdFdAhPR(gO2,gI1))*CpbarFdFdAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fd(
      gO2)),Fd(gI2),hh)(PR))*Cp(bar(Fd(gO1)),Fd(gI2),hh)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fd(gO2)),Fd(gI2),VZ)(PL))*Cp(bar(Fd(gO1)),Fd(gI2),VZ)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(Fd(
      gO2)),Fu(gI2),Conj(Hp))(PR))*Cp(bar(Fd(gO1)),Fu(gI2),Conj(Hp))(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fd(gO2)),Fu(gI2),Conj(VWp))(PL))*Cp(bar(Fd(gO1)),Fu(gI2),Conj(VWp))(PL))
      ;

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_PR_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_PR_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fd_1loop_PL_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFd(gI1)),Sqr(MAh))*Conj(
      CpbarFdFdAhPL(gO2,gI1))*CpbarFdFdAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fd(
      gO2)),Fd(gI2),hh)(PL))*Cp(bar(Fd(gO1)),Fd(gI2),hh)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fd(gO2)),Fd(gI2),VZ)(PR))*Cp(bar(Fd(gO1)),Fd(gI2),VZ)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MHp))*Conj(Cp(bar(Fd(
      gO2)),Fu(gI2),Conj(Hp))(PL))*Cp(bar(Fd(gO1)),Fu(gI2),Conj(Hp))(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fd(gO2)),Fu(gI2),Conj(VWp))(PR))*Cp(bar(Fd(gO1)),Fu(gI2),Conj(VWp))(PR))
      ;

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fd_1loop_PL_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fd_1loop_PL_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_1_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(CpbarFeFeAhPL(
      gO2,gI1))*CpbarFeFeAhPR(gO1,gI1)*MFe(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fe(gO2))
      ,Fe(gI2),hh)(PL))*MFe(gI2)*Cp(bar(Fe(gO1)),Fe(gI2),hh)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(Fe(gO2)),Fe(gI2),VZ)(PR))*MFe(gI2)*Cp(bar(Fe(gO1)),Fe(gI2),VZ)(PL));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(Fe(gO2))
      ,Fv(gI2),Conj(Hp))(PL))*MFv(gI2)*Cp(bar(Fe(gO1)),Fv(gI2),Conj(Hp))(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(Fe(gO2)),Fv(gI2),Conj(VWp))(PR))*MFv(gI2)*Cp(bar(Fe(gO1)),Fv(gI2),Conj(
      VWp))(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_1_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_1_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_PR_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(
      CpbarFeFeAhPR(gO2,gI1))*CpbarFeFeAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fe(
      gO2)),Fe(gI2),hh)(PR))*Cp(bar(Fe(gO1)),Fe(gI2),hh)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fe(gO2)),Fe(gI2),VZ)(PL))*Cp(bar(Fe(gO1)),Fe(gI2),VZ)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(Fe(
      gO2)),Fv(gI2),Conj(Hp))(PR))*Cp(bar(Fe(gO1)),Fv(gI2),Conj(Hp))(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fe(gO2)),Fv(gI2),Conj(VWp))(PL))*Cp(bar(Fe(gO1)),Fv(gI2),Conj(VWp))(PL))
      ;

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_PR_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_PR_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fe_1loop_PL_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFe(gI1)),Sqr(MAh))*Conj(
      CpbarFeFeAhPL(gO2,gI1))*CpbarFeFeAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFe(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fe(
      gO2)),Fe(gI2),hh)(PL))*Cp(bar(Fe(gO1)),Fe(gI2),hh)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFe(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fe(gO2)),Fe(gI2),VZ)(PR))*Cp(bar(Fe(gO1)),Fe(gI2),VZ)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MHp))*Conj(Cp(bar(Fe(
      gO2)),Fv(gI2),Conj(Hp))(PL))*Cp(bar(Fe(gO1)),Fv(gI2),Conj(Hp))(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFv(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fe(gO2)),Fv(gI2),Conj(VWp))(PR))*Cp(bar(Fe(gO1)),Fv(gI2),Conj(VWp))(PR))
      ;

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fe_1loop_PL_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fe_1loop_PL_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_1_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(CpbarFuFuAhPL(
      gO2,gI1))*CpbarFuFuAhPR(gO1,gI1)*MFu(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(Fu(gO2))
      ,Fd(gI2),Hp)(PL))*MFd(gI2)*Cp(bar(Fu(gO1)),Fd(gI2),Hp)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(Fu(gO2)),Fd(gI2),VWp)(PR))*MFd(gI2)*Cp(bar(Fu(gO1)),Fd(gI2),VWp)(PL));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fu(gO2))
      ,Fu(gI2),hh)(PL))*MFu(gI2)*Cp(bar(Fu(gO1)),Fu(gI2),hh)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(Fu(
      gO2)),Fu(gI2),VP)(PR))*MFu(gI2)*Cp(bar(Fu(gO1)),Fu(gI2),VP)(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(Fu(gO2)),Fu(gI2),VZ)(PR))*MFu(gI2)*Cp(bar(Fu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_1_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_1_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PR_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarFuFuAhPR(gO2,gI1))*CpbarFuFuAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(Fu(
      gO2)),Fd(gI2),Hp)(PR))*Cp(bar(Fu(gO1)),Fd(gI2),Hp)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fu(gO2)),Fd(gI2),VWp)(PL))*Cp(bar(Fu(gO1)),Fd(gI2),VWp)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fu(
      gO2)),Fu(gI2),hh)(PR))*Cp(bar(Fu(gO1)),Fu(gI2),hh)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(Fu(gO2
      )),Fu(gI2),VP)(PL))*Cp(bar(Fu(gO1)),Fu(gI2),VP)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fu(gO2)),Fu(gI2),VZ)(PL))*Cp(bar(Fu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PR_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PR_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PL_heavy_rotated(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarFuFuAhPL(gO2,gI1))*CpbarFuFuAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(Fu(
      gO2)),Fd(gI2),Hp)(PL))*Cp(bar(Fu(gO1)),Fd(gI2),Hp)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(Fu(gO2)),Fd(gI2),VWp)(PR))*Cp(bar(Fu(gO1)),Fd(gI2),VWp)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(Fu(
      gO2)),Fu(gI2),hh)(PL))*Cp(bar(Fu(gO1)),Fu(gI2),hh)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(Fu(gO2
      )),Fu(gI2),VP)(PR))*Cp(bar(Fu(gO1)),Fu(gI2),VP)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (Fu(gO2)),Fu(gI2),VZ)(PR))*Cp(bar(Fu(gO1)),Fu(gI2),VZ)(PR));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PL_heavy_rotated(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PL_heavy_rotated(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_1_heavy(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += SUM(gI1,0,2,B0(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(CpbarUFuFuAhPL(
      gO2,gI1))*CpbarUFuFuAhPR(gO1,gI1)*MFu(gI1));
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu(gO2)
      ),Fd(gI2),Hp)(PL))*MFd(gI2)*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp
      (bar(UFu(gO2)),Fd(gI2),VWp)(PR))*MFd(gI2)*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PL))
      ;
   result += SUM(gI2,0,2,B0(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu(gO2)
      ),Fu(gI2),hh)(PL))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PR));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),VP)(PR))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PL));
   result += -4*SUM(gI2,0,2,(-0.5 + B0(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(
      bar(UFu(gO2)),Fu(gI2),VZ)(PR))*MFu(gI2)*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_1_heavy(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_1_heavy(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PR_heavy(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarUFuFuAhPR(gO2,gI1))*CpbarUFuFuAhPR(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu
      (gO2)),Fd(gI2),Hp)(PR))*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFu(gO2)),Fd(gI2),VWp)(PL))*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PL));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),hh)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu(
      gO2)),Fu(gI2),VP)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFu(gO2)),Fu(gI2),VZ)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PL));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PR_heavy(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PR_heavy(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::self_energy_Fu_1loop_PL_heavy(double p , int gO1, int gO2) const
{
   std::complex<double> result;

   result += -0.5*SUM(gI1,0,2,B1(Sqr(p),Sqr(MFu(gI1)),Sqr(MAh))*Conj(
      CpbarUFuFuAhPL(gO2,gI1))*CpbarUFuFuAhPL(gO1,gI1));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MHp))*Conj(Cp(bar(UFu
      (gO2)),Fd(gI2),Hp)(PL))*Cp(bar(UFu(gO1)),Fd(gI2),Hp)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFd(gI2)),Sqr(MVWp)))*Conj(Cp(
      bar(UFu(gO2)),Fd(gI2),VWp)(PR))*Cp(bar(UFu(gO1)),Fd(gI2),VWp)(PR));
   result += -0.5*SUM(gI2,0,2,B1(Sqr(p),Sqr(MFu(gI2)),Sqr(Mhh))*Conj(Cp(bar(UFu
      (gO2)),Fu(gI2),hh)(PL))*Cp(bar(UFu(gO1)),Fu(gI2),hh)(PL));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),0))*Conj(Cp(bar(UFu(
      gO2)),Fu(gI2),VP)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),VP)(PR));
   result += -SUM(gI2,0,2,(0.5 + B1(Sqr(p),Sqr(MFu(gI2)),Sqr(MVZ)))*Conj(Cp(bar
      (UFu(gO2)),Fu(gI2),VZ)(PR))*Cp(bar(UFu(gO1)),Fu(gI2),VZ)(PR));

   return result * oneOver16PiSqr;

}

Eigen::Matrix<std::complex<double>,3,3> CLASSNAME::self_energy_Fu_1loop_PL_heavy(double p) const
{
   Eigen::Matrix<std::complex<double>,3,3> self_energy;

   for (int i = 0; i < 3; i++)
      for (int k = 0; k < 3; k++)
         self_energy(i, k) = self_energy_Fu_1loop_PL_heavy(p, i, k);

   return self_energy;
}

std::complex<double> CLASSNAME::tadpole_hh_1loop() const
{
   std::complex<double> result;

   result += -0.5*A0(Sqr(MAh))*Cp(Ah,Ah,hh);
   result += -(A0(Sqr(MHp))*Cp(hh,Hp,Conj(Hp)));
   result += 2*A0(Sqr(MVWp))*Cp(bar(VWp),VWp,hh);
   result += A0(Sqr(MVZ))*Cp(bar(VZ),VZ,hh);
   result += -0.5*A0(Sqr(Mhh))*Cphhhhhh();
   result += 4*A0(Sqr(MVWp))*CphhconjVWpVWp() - 2*CphhconjVWpVWp()*Sqr(MVWp);
   result += CphhVZVZ()*(2*A0(Sqr(MVZ)) - Sqr(MVZ));
   result += 6*SUM(gI1,0,2,A0(Sqr(MFd(gI1)))*MFd(gI1)*(Cp(bar(Fd(gI1)),Fd(gI1),
      hh)(PL) + Cp(bar(Fd(gI1)),Fd(gI1),hh)(PR)));
   result += 2*SUM(gI1,0,2,A0(Sqr(MFe(gI1)))*MFe(gI1)*(Cp(bar(Fe(gI1)),Fe(gI1),
      hh)(PL) + Cp(bar(Fe(gI1)),Fe(gI1),hh)(PR)));
   result += 6*SUM(gI1,0,2,A0(Sqr(MFu(gI1)))*MFu(gI1)*(Cp(bar(Fu(gI1)),Fu(gI1),
      hh)(PL) + Cp(bar(Fu(gI1)),Fu(gI1),hh)(PR)));

   return result * oneOver16PiSqr;

}





double CLASSNAME::self_energy_hh_2loop(double p) const
{
   using namespace flexiblesusy::sm_twoloophiggs;

   const double p2 = Sqr(p);
   const double mt = MFu(2);
   const double yt = Yu(2,2);
   const double gs = g3;
   const double scale = get_scale();
   double self_energy = 0.;

   if (HIGGS_2LOOP_CORRECTION_AT_AT) {
      self_energy += self_energy_higgs_2loop_at_at_sm(p2, scale, mt, yt);
   }

   if (HIGGS_2LOOP_CORRECTION_AT_AS) {
      self_energy += self_energy_higgs_2loop_at_as_sm(p2, scale, mt, yt, gs);
   }

   return self_energy;
}





double CLASSNAME::self_energy_hh_3loop() const
{
   using namespace flexiblesusy::sm_threeloophiggs;

   const double mt = MFu(2);
   const double yt = Yu(2,2);
   const double gs = g3;
   const double mh = Mhh;
   const double scale = get_scale();
   double self_energy = 0.;

   if (HIGGS_3LOOP_CORRECTION_AT_AT_AT) {
      self_energy += self_energy_higgs_3loop_at_at_at_sm(scale, mt, yt, mh);
   }

   if (HIGGS_3LOOP_CORRECTION_AT_AT_AS) {
      self_energy += self_energy_higgs_3loop_at_at_as_sm(scale, mt, yt, gs);
   }

   if (HIGGS_3LOOP_CORRECTION_AT_AS_AS) {
      self_energy += self_energy_higgs_3loop_at_as_as_sm(scale, mt, yt, gs);
   }

   return self_energy;
}



void CLASSNAME::calculate_MVG_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MVG) old_MVG(MVG), new_MVG(MVG);

   do {
      PHYSICAL(MVG) = 0.;

      new_MVG = PHYSICAL(MVG);
      diff = MaxRelDiff(new_MVG, old_MVG);
      old_MVG = new_MVG;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::VG);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::VG);
}

void CLASSNAME::calculate_MFv_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MFv) old_MFv(MFv), new_MFv(MFv);

   do {
      PHYSICAL(MFv).setConstant(0.);

      new_MFv = PHYSICAL(MFv);
      diff = MaxRelDiff(new_MFv, old_MFv);
      old_MFv = new_MFv;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::Fv);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::Fv);
}

void CLASSNAME::calculate_Mhh_pole()
{
   if (!force_output && problems.is_running_tachyon(HSSUSY_info::hh))
      return;

   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(Mhh) old_Mhh(Mhh), new_Mhh(Mhh);

   do {
      const double M_tree(get_mass_matrix_hh());
      const double p = old_Mhh;
      double self_energy = Re(self_energy_hh_1loop(p));
      if (pole_mass_loop_order > 1)
         self_energy += self_energy_hh_2loop(p);
      if (pole_mass_loop_order > 2)
         self_energy += self_energy_hh_3loop();
      const double mass_sqr = M_tree - self_energy;

      PHYSICAL(Mhh) = SignedAbsSqrt(mass_sqr);

      new_Mhh = PHYSICAL(Mhh);
      diff = MaxRelDiff(new_Mhh, old_Mhh);
      old_Mhh = new_Mhh;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::hh);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::hh);
}

void CLASSNAME::calculate_MVP_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MVP) old_MVP(MVP), new_MVP(MVP);

   do {
      PHYSICAL(MVP) = 0.;

      new_MVP = PHYSICAL(MVP);
      diff = MaxRelDiff(new_MVP, old_MVP);
      old_MVP = new_MVP;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::VP);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::VP);
}

void CLASSNAME::calculate_MVZ_pole()
{
   if (!force_output && problems.is_running_tachyon(HSSUSY_info::VZ))
      return;

   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MVZ) old_MVZ(MVZ), new_MVZ(MVZ);

   do {
      const double M_tree(Sqr(MVZ));
      const double p = old_MVZ;
      const double self_energy = Re(self_energy_VZ_1loop(p));
      const double mass_sqr = M_tree - self_energy;

      if (mass_sqr < 0.)
         problems.flag_pole_tachyon(HSSUSY_info::VZ);

      PHYSICAL(MVZ) = AbsSqrt(mass_sqr);

      new_MVZ = PHYSICAL(MVZ);
      diff = MaxRelDiff(new_MVZ, old_MVZ);
      old_MVZ = new_MVZ;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::VZ);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::VZ);
}

void CLASSNAME::calculate_MFd_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MFd) old_MFd(MFd), new_MFd(MFd);

   do {
      const Eigen::Matrix<double,3,3> M_tree(get_mass_matrix_Fd());
      for (int es = 0; es < 3; ++es) {
         const double p = Abs(old_MFd(es));
         const Eigen::Matrix<double,3,3> self_energy_1  = Re(
            self_energy_Fd_1loop_1(p));
         const Eigen::Matrix<double,3,3> self_energy_PL = Re(
            self_energy_Fd_1loop_PL(p));
         const Eigen::Matrix<double,3,3> self_energy_PR = Re(
            self_energy_Fd_1loop_PR(p));
         const Eigen::Matrix<double,3,3> delta_M(- self_energy_PR *
            M_tree - M_tree * self_energy_PL - self_energy_1);
         const Eigen::Matrix<double,3,3> M_loop(M_tree + delta_M);
         Eigen::Array<double,3,1> eigen_values;
         decltype(Vd) mix_Vd;
         decltype(Ud) mix_Ud;
      #ifdef CHECK_EIGENVALUE_ERROR
         double eigenvalue_error;
         fs_svd(M_loop, eigen_values, mix_Vd, mix_Ud,
            eigenvalue_error);
         problems.flag_bad_mass(HSSUSY_info::Fd, eigenvalue_error >
            precision * Abs(eigen_values(0)));
      #else
         fs_svd(M_loop, eigen_values, mix_Vd, mix_Ud);
      #endif
         if (es == 0) {
            PHYSICAL(Vd) = mix_Vd;
            PHYSICAL(Ud) = mix_Ud;
         }
         PHYSICAL(MFd(es)) = Abs(eigen_values(es));
      }

      new_MFd = PHYSICAL(MFd);
      diff = MaxRelDiff(new_MFd, old_MFd);
      old_MFd = new_MFd;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::Fd);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::Fd);
}

void CLASSNAME::calculate_MFu_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MFu) old_MFu(MFu), new_MFu(MFu);

   do {
      double qcd_1l = 0.;

      {
         const double currentScale = get_scale();
         qcd_1l = -0.008443431970194815*(4. - 3.*Log(Sqr(MFu(2))
            /Sqr(currentScale)))*Sqr(g3);
      }

      double qcd_2l = 0.;

      if (pole_mass_loop_order > 1 && TOP_POLE_QCD_CORRECTION > 0) {
         const double currentScale = get_scale();
         qcd_2l = -0.005284774766427138*Quad(g3) -
            0.0032348537833770956*Log(Sqr(currentScale)/Sqr(MFu(2)))*Quad(g3) -
            0.0008822328500119351*Quad(g3)*Sqr(Log(Sqr(currentScale)/Sqr(MFu(2
            ))));
      }

      double qcd_3l = 0.;

      if (pole_mass_loop_order > 2 && TOP_POLE_QCD_CORRECTION > 1) {
         const double currentScale = get_scale();
         qcd_3l = -0.00003352082872926087*Power6(g3)*(
            35.702577217116016 + 1.*Cube(Log(Sqr(currentScale)/Sqr(MFu(2)))) +
            15.387410814884797*Log(Sqr(currentScale)/Sqr(MFu(2))) +
            5.378787878787879*Sqr(Log(Sqr(currentScale)/Sqr(MFu(2)))));
      }

      const Eigen::Matrix<double,3,3> M_tree(get_mass_matrix_Fu());
      for (int es = 0; es < 3; ++es) {
         const double p = Abs(old_MFu(es));
         Eigen::Matrix<double,3,3> self_energy_1;
         Eigen::Matrix<double,3,3> self_energy_PL;
         Eigen::Matrix<double,3,3> self_energy_PR;
         for (int i1 = 0; i1 < 3; ++i1) {
            for (int i2 = 0; i2 < 3; ++i2) {
               if (i1 == 2 && i2 == 2) {
                  self_energy_1(i1,i2)  = Re(
                     self_energy_Fu_1loop_1_heavy(p,i1,i2));
                  self_energy_PL(i1,i2) = Re(
                     self_energy_Fu_1loop_PL_heavy(p,i1,i2));
                  self_energy_PR(i1,i2) = Re(
                     self_energy_Fu_1loop_PR_heavy(p,i1,i2));
               } else {
                  self_energy_1(i1,i2)  = Re(
                     self_energy_Fu_1loop_1(p,i1,i2));
                  self_energy_PL(i1,i2) = Re(
                     self_energy_Fu_1loop_PL(p,i1,i2));
                  self_energy_PR(i1,i2) = Re(
                     self_energy_Fu_1loop_PR(p,i1,i2));
               }
            }
         }
         Eigen::Matrix<double,3,3> delta_M(- self_energy_PR *
            M_tree - M_tree * self_energy_PL - self_energy_1);
         delta_M(2,2) -= M_tree(2,2) * (qcd_1l + qcd_2l + qcd_3l);
         const Eigen::Matrix<double,3,3> M_loop(M_tree + delta_M);
         Eigen::Array<double,3,1> eigen_values;
         decltype(Vu) mix_Vu;
         decltype(Uu) mix_Uu;
      #ifdef CHECK_EIGENVALUE_ERROR
         double eigenvalue_error;
         fs_svd(M_loop, eigen_values, mix_Vu, mix_Uu,
            eigenvalue_error);
         problems.flag_bad_mass(HSSUSY_info::Fu, eigenvalue_error >
            precision * Abs(eigen_values(0)));
      #else
         fs_svd(M_loop, eigen_values, mix_Vu, mix_Uu);
      #endif
         if (es == 0) {
            PHYSICAL(Vu) = mix_Vu;
            PHYSICAL(Uu) = mix_Uu;
         }
         PHYSICAL(MFu(es)) = Abs(eigen_values(es));
      }

      new_MFu = PHYSICAL(MFu);
      diff = MaxRelDiff(new_MFu, old_MFu);
      old_MFu = new_MFu;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::Fu);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::Fu);
}

void CLASSNAME::calculate_MFe_pole()
{
   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MFe) old_MFe(MFe), new_MFe(MFe);

   do {
      const Eigen::Matrix<double,3,3> M_tree(get_mass_matrix_Fe());
      for (int es = 0; es < 3; ++es) {
         const double p = Abs(old_MFe(es));
         const Eigen::Matrix<double,3,3> self_energy_1  = Re(
            self_energy_Fe_1loop_1(p));
         const Eigen::Matrix<double,3,3> self_energy_PL = Re(
            self_energy_Fe_1loop_PL(p));
         const Eigen::Matrix<double,3,3> self_energy_PR = Re(
            self_energy_Fe_1loop_PR(p));
         const Eigen::Matrix<double,3,3> delta_M(- self_energy_PR *
            M_tree - M_tree * self_energy_PL - self_energy_1);
         const Eigen::Matrix<double,3,3> M_loop(M_tree + delta_M);
         Eigen::Array<double,3,1> eigen_values;
         decltype(Ve) mix_Ve;
         decltype(Ue) mix_Ue;
      #ifdef CHECK_EIGENVALUE_ERROR
         double eigenvalue_error;
         fs_svd(M_loop, eigen_values, mix_Ve, mix_Ue,
            eigenvalue_error);
         problems.flag_bad_mass(HSSUSY_info::Fe, eigenvalue_error >
            precision * Abs(eigen_values(0)));
      #else
         fs_svd(M_loop, eigen_values, mix_Ve, mix_Ue);
      #endif
         if (es == 0) {
            PHYSICAL(Ve) = mix_Ve;
            PHYSICAL(Ue) = mix_Ue;
         }
         PHYSICAL(MFe(es)) = Abs(eigen_values(es));
      }

      new_MFe = PHYSICAL(MFe);
      diff = MaxRelDiff(new_MFe, old_MFe);
      old_MFe = new_MFe;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::Fe);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::Fe);
}

void CLASSNAME::calculate_MVWp_pole()
{
   if (!force_output && problems.is_running_tachyon(HSSUSY_info::VWp))
      return;

   // diagonalization with high precision
   const auto number_of_mass_iterations = get_number_of_mass_iterations()
      ;
   int iteration = 0;
   double diff = 0.0;
   decltype(MVWp) old_MVWp(MVWp), new_MVWp(MVWp);

   do {
      const double M_tree(Sqr(MVWp));
      const double p = old_MVWp;
      const double self_energy = Re(self_energy_VWp_1loop(p));
      const double mass_sqr = M_tree - self_energy;

      if (mass_sqr < 0.)
         problems.flag_pole_tachyon(HSSUSY_info::VWp);

      PHYSICAL(MVWp) = AbsSqrt(mass_sqr);

      new_MVWp = PHYSICAL(MVWp);
      diff = MaxRelDiff(new_MVWp, old_MVWp);
      old_MVWp = new_MVWp;
      iteration++;
   } while (diff > precision
            && iteration < number_of_mass_iterations);

   if (diff > precision)
      problems.flag_no_pole_mass_convergence(HSSUSY_info::VWp);
   else
      problems.unflag_no_pole_mass_convergence(HSSUSY_info::VWp);
}

double CLASSNAME::calculate_MVWp_pole(double p)
{
   if (!force_output && problems.is_running_tachyon(HSSUSY_info::VWp))
      return 0.;

   const double self_energy = Re(self_energy_VWp_1loop(p));
   const double mass_sqr = Sqr(MVWp) - self_energy;

   if (mass_sqr < 0.)
      problems.flag_pole_tachyon(HSSUSY_info::VWp);

   return AbsSqrt(mass_sqr);
}

double CLASSNAME::calculate_MVZ_pole(double p)
{
   if (!force_output && problems.is_running_tachyon(HSSUSY_info::VZ))
      return 0.;

   const double self_energy = Re(self_energy_VZ_1loop(p));
   const double mass_sqr = Sqr(MVZ) - self_energy;

   if (mass_sqr < 0.)
      problems.flag_pole_tachyon(HSSUSY_info::VZ);

   return AbsSqrt(mass_sqr);
}


double CLASSNAME::calculate_MFv_DRbar(double, int) const
{
   return 0.0;
}

double CLASSNAME::calculate_MFe_DRbar(double m_sm_msbar, int idx) const
{
   const double p = m_sm_msbar;
   const double self_energy_1  = Re(self_energy_Fe_1loop_1_heavy_rotated(
      p, idx, idx));
   const double self_energy_PL = Re(self_energy_Fe_1loop_PL_heavy_rotated
      (p, idx, idx));
   const double self_energy_PR = Re(self_energy_Fe_1loop_PR_heavy_rotated
      (p, idx, idx));
   const double drbar_conversion = 1;
   const double m_sm_drbar = m_sm_msbar * drbar_conversion;
   const double delta_mf_1loop = - self_energy_1/m_sm_drbar -
      self_energy_PL - self_energy_PR;

   const double m_susy_drbar = m_sm_drbar / (1.0 + delta_mf_1loop);

   return m_susy_drbar;
}

double CLASSNAME::calculate_MFu_DRbar(double m_pole, int idx) const
{
   const double p = m_pole;
   const double self_energy_1  = Re(self_energy_Fu_1loop_1_heavy_rotated(
      p, idx, idx));
   const double self_energy_PL = Re(self_energy_Fu_1loop_PL_heavy_rotated
      (p, idx, idx));
   const double self_energy_PR = Re(self_energy_Fu_1loop_PR_heavy_rotated
      (p, idx, idx));

   const double currentScale = get_scale();
   double qcd_1l = 0., qcd_2l = 0., qcd_3l = 0.;

   qcd_1l = -0.008443431970194815*(4. - 3.*Log(Sqr(MFu(idx))/Sqr(
      currentScale)))*Sqr(g3);

   if (get_thresholds() > 1 && threshold_corrections.mt > 1) {
      const double q_2l = 0.005284774766427138*Quad(g3) +
         0.0032348537833770956*Log(Sqr(currentScale)/Sqr(MFu(idx)))*Quad(g3) +
         0.0008822328500119351*Quad(g3)*Sqr(Log(Sqr(currentScale)/Sqr(MFu(idx))
         ));

      qcd_2l = -q_2l + qcd_1l * qcd_1l;
   }

   if (get_thresholds() > 2 && threshold_corrections.mt > 2) {
      qcd_3l = -0.0008783313853540776*Power6(g3) -
         5.078913443827405e-6*Cube(Log(Sqr(currentScale)/Sqr(MFu(idx))))*Power6
         (g3) - 0.0004114970933517977*Log(Sqr(currentScale)/Sqr(MFu(idx)))*
         Power6(g3) - 0.0002952541682011665*Log(Sqr(MFu(idx))/Sqr(currentScale)
         )*Power6(g3) + 0.00005282069981580501*Power6(g3)*Sqr(Log(Sqr(MFu(idx))
         /Sqr(currentScale))) - 0.00007466002762426286*Power6(g3)*Sqr(Log(Sqr(
         currentScale)/Sqr(MFu(idx))));
   }

   const double m_susy_drbar = m_pole + self_energy_1 + m_pole * (
      self_energy_PL + self_energy_PR + qcd_1l + qcd_2l + qcd_3l);

   return m_susy_drbar;
}

double CLASSNAME::calculate_MFd_DRbar(double m_sm_msbar, int idx) const
{
   const double p = m_sm_msbar;
   const double self_energy_1  = Re(self_energy_Fd_1loop_1_heavy_rotated(
      p, idx, idx));
   const double self_energy_PL = Re(self_energy_Fd_1loop_PL_heavy_rotated
      (p, idx, idx));
   const double self_energy_PR = Re(self_energy_Fd_1loop_PR_heavy_rotated
      (p, idx, idx));
   const double m_tree = MFd(idx);
   const double drbar_conversion = 1;
   const double m_sm_drbar = m_sm_msbar * drbar_conversion;
   const double delta_mb_1loop = - self_energy_1/m_tree - self_energy_PL
      - self_energy_PR;
   double qcd_2l = 0.;

   const double m_susy_drbar = m_sm_drbar / (1.0 + delta_mb_1loop +
      qcd_2l);

   return m_susy_drbar;
}

double CLASSNAME::calculate_MVP_DRbar(double)
{
   return 0.0;
}

double CLASSNAME::calculate_MVZ_DRbar(double m_pole)
{
   const double p = m_pole;
   const double self_energy = Re(self_energy_VZ_1loop(p));
   const double mass_sqr = Sqr(m_pole) + self_energy;

   if (mass_sqr < 0.) {
      problems.flag_pole_tachyon(HSSUSY_info::VZ);
      return m_pole;
   }

   return AbsSqrt(mass_sqr);
}

double CLASSNAME::calculate_MVWp_DRbar(double m_pole)
{
   const double p = m_pole;
   const double self_energy = Re(self_energy_VWp_1loop(p));
   const double mass_sqr = Sqr(m_pole) + self_energy;

   if (mass_sqr < 0.) {
      problems.flag_pole_tachyon(HSSUSY_info::VWp);
      return m_pole;
   }

   return AbsSqrt(mass_sqr);
}


double CLASSNAME::ThetaW() const
{
   return ArcCos(Abs(ZZ(0,0)));
}


std::ostream& operator<<(std::ostream& ostr, const HSSUSY_mass_eigenstates& model)
{
   model.print(ostr);
   return ostr;
}

} // namespace flexiblesusy
