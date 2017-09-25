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

// File generated at Sun 24 Sep 2017 15:44:16

#ifndef MSSMatMSUSY_mAmu_TWO_SCALE_SUSY_SCALE_CONSTRAINT_H
#define MSSMatMSUSY_mAmu_TWO_SCALE_SUSY_SCALE_CONSTRAINT_H

#include "MSSMatMSUSY_mAmu_susy_scale_constraint.hpp"
#include "MSSMatMSUSY_mAmu_input_parameters.hpp"
#include "single_scale_constraint.hpp"
#include "lowe.h"

namespace flexiblesusy {

template <class T>
class MSSMatMSUSY_mAmu;

class Two_scale;

template<>
class MSSMatMSUSY_mAmu_susy_scale_constraint<Two_scale> : public Single_scale_constraint {
public:
   MSSMatMSUSY_mAmu_susy_scale_constraint() = default;
   MSSMatMSUSY_mAmu_susy_scale_constraint(MSSMatMSUSY_mAmu<Two_scale>*, const softsusy::QedQcd&);
   virtual ~MSSMatMSUSY_mAmu_susy_scale_constraint() = default;
   virtual void apply() override;
   virtual double get_scale() const override;
   virtual std::string name() const override { return "MSSMatMSUSY_mAmu SUSY-scale constraint"; }
   virtual void set_model(Model*) override;

   void clear();
   double get_initial_scale_guess() const;
   const MSSMatMSUSY_mAmu_input_parameters& get_input_parameters() const;
   MSSMatMSUSY_mAmu<Two_scale>* get_model() const;
   void initialize();
   const softsusy::QedQcd& get_sm_parameters() const;
   void set_sm_parameters(const softsusy::QedQcd&);

protected:
   void update_scale();

private:
   double scale{0.};
   double initial_scale_guess{0.};
   MSSMatMSUSY_mAmu<Two_scale>* model{nullptr};
   softsusy::QedQcd qedqcd{};

   void check_model_ptr() const;
};

} // namespace flexiblesusy

#endif
