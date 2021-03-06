//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// Antioch - A Gas Dynamics Thermochemistry Library
//
// Copyright (C) 2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-
//
// $Id: berthelot_rate_vec_unit.C 38747 2013-04-17 23:26:39Z splessis $
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

// valarray has to be declared before Antioch or gcc can't find the
// right versions of exp() and pow() to use??

#include "antioch_config.h"

#include <valarray>

#ifdef ANTIOCH_HAVE_EIGEN
#include "Eigen/Dense"
#endif

#ifdef ANTIOCH_HAVE_METAPHYSICL
#include "metaphysicl/numberarray.h"
#endif

#include "antioch/eigen_utils_decl.h"
#include "antioch/metaphysicl_utils_decl.h"
#include "antioch/valarray_utils_decl.h"

#include "antioch/berthelot_rate.h"

#include "antioch/eigen_utils.h"
#include "antioch/metaphysicl_utils.h"
#include "antioch/valarray_utils.h"

#include <cmath>
#include <limits>

template <typename PairScalars>
int vectester(const PairScalars& example)
{
  using std::abs;
  using std::exp;

  typedef typename Antioch::value_type<PairScalars>::type Scalar;

  const Scalar Cf = 1.4;
  const Scalar D = -2.5;

  Antioch::BerthelotRate<Scalar> berthelot_rate(Cf,D);

  // Construct from example to avoid resizing issues
  PairScalars T = example;
  T[0] = 1500.1;
  T[1] = 1600.1;
  
  const Scalar rate_exact0 = Cf*exp(D*1500.1);
  const Scalar rate_exact1 = Cf*exp(D*1600.1);
  const Scalar derive_exact0 = D * Cf * exp(D*Scalar(1500.1));
  const Scalar derive_exact1 = D * Cf * exp(D*Scalar(1600.1));

  int return_flag = 0;

  const PairScalars rate = berthelot_rate(T);
  const PairScalars deriveRate = berthelot_rate.derivative(T);

  const Scalar tol = std::numeric_limits<Scalar>::epsilon()*10;

  if( abs( (rate[0] - rate_exact0)/rate_exact0 ) > tol )
    {
      std::cout << "Error: Mismatch in rate values." << std::endl
		<< "rate(T0)   = " << rate[0] << std::endl
		<< "rate_exact = " << rate_exact0 << std::endl
		<< "difference = " << rate[0] - rate_exact0 << std::endl;

      return_flag = 1;
    }

  if( abs( (rate[1] - rate_exact1)/rate_exact1 ) > tol )
    {
      std::cout << "Error: Mismatch in rate values." << std::endl
		<< "rate(T1)   = " << rate[1] << std::endl
		<< "rate_exact = " << rate_exact1 << std::endl
		<< "difference = " << rate[1] - rate_exact1 << std::endl;

      return_flag = 1;
    }
  if( abs( (deriveRate[0] - derive_exact0)/derive_exact0 ) > tol )
    {
      std::cout << std::scientific << std::setprecision(16)
                << "Error: Mismatch in rate derivative values." << std::endl
		<< "drate_dT(T0) = " << deriveRate[0] << std::endl
		<< "derive_exact = " << derive_exact0 << std::endl;

      return_flag = 1;
    }
  if( abs( (deriveRate[1] - derive_exact1)/derive_exact1 ) > tol )
    {
      std::cout << std::scientific << std::setprecision(16)
                << "Error: Mismatch in rate derivative values." << std::endl
		<< "drate_dT(T1) = " << deriveRate[1] << std::endl
		<< "derive_exact = " << derive_exact1 << std::endl;

      return_flag = 1;
    }

  std::cout << "Berthelot rate: " << berthelot_rate << std::endl;

  return return_flag;
}


int main()
{
  int returnval = 0;

  returnval = returnval ||
    vectester (std::valarray<float>(2));
  returnval = returnval ||
    vectester (std::valarray<double>(2));
  returnval = returnval ||
    vectester (std::valarray<long double>(2));
#ifdef ANTIOCH_HAVE_EIGEN
  returnval = returnval ||
    vectester (Eigen::Array2f());
  returnval = returnval ||
    vectester (Eigen::Array2d());
  returnval = returnval ||
    vectester (Eigen::Array<long double, 2, 1>());
#endif
#ifdef ANTIOCH_HAVE_METAPHYSICL
  returnval = returnval ||
    vectester (MetaPhysicL::NumberArray<2, float> (0));
  returnval = returnval ||
    vectester (MetaPhysicL::NumberArray<2, double> (0));
  returnval = returnval ||
    vectester (MetaPhysicL::NumberArray<2, long double> (0));
#endif

  return returnval;
}
