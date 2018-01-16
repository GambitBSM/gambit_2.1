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

// File generated at Wed 25 Oct 2017 18:35:30

/**
 * @file MSSMNoFV_two_scale_model.hpp
 * @brief contains class for model with routines needed to solve boundary
 *        value problem using the two_scale solver by solving EWSB
 *        and determine the pole masses and mixings
 *
 * This file was generated at Wed 25 Oct 2017 18:35:30 with FlexibleSUSY
 * 2.0.1 (git commit: unknown) and SARAH 4.11.0 .
 */

#ifndef MSSMNoFV_TWO_SCALE_H
#define MSSMNoFV_TWO_SCALE_H

#include "MSSMNoFV_model.hpp"
#include "MSSMNoFV_mass_eigenstates.hpp"

#include "model.hpp"

namespace flexiblesusy {

class Two_scale;
/**
 * @class MSSMNoFV<Two_scale>
 * @brief model class with routines for determining masses and mixings and EWSB
 */
template<>
class MSSMNoFV<Two_scale> : public Model, public MSSMNoFV_mass_eigenstates {
public:
   explicit MSSMNoFV(const MSSMNoFV_input_parameters& input_ = MSSMNoFV_input_parameters());
   MSSMNoFV(const MSSMNoFV&) = default;
   MSSMNoFV(MSSMNoFV&&) = default;
   virtual ~MSSMNoFV() = default;
   MSSMNoFV& operator=(const MSSMNoFV&) = default;
   MSSMNoFV& operator=(MSSMNoFV&&) = default;

   // interface functions
   virtual void calculate_spectrum() override;
   virtual void clear_problems() override;
   virtual std::string name() const override;
   virtual void run_to(double scale, double eps = -1.0) override;
   virtual void print(std::ostream& out = std::cerr) const override;
   virtual void set_precision(double) override;
};

std::ostream& operator<<(std::ostream&, const MSSMNoFV<Two_scale>&);

} // namespace flexiblesusy

#endif
