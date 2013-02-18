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
// $Id$
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#ifndef ANTIOCH_CEA_THERMO_H
#define ANTIOCH_CEA_THERMO_H

// C++
#include <iomanip>
#include <vector>
#include <cmath>

// Antioch
#include "antioch/cea_curve_fit.h"
#include "antioch/cea_thermo_ascii_parsing.h"
#include "antioch/chemical_mixture.h"
#include "antioch/chemical_species.h"
#include "antioch/input_utils.h"

namespace Antioch
{
  // Forward declarations
  template<class NumericType>
  class CEACurveFit;

  template<class NumericType>
  class CEAThermodynamics
  {
  public:

    CEAThermodynamics( const ChemicalMixture<NumericType>& chem_mixture );

    //! Destructor
    /*! virtual so this can be subclassed by the user. */
    virtual ~CEAThermodynamics();

    class Cache 
    {
    public:
      const NumericType &T;
      NumericType T2;
      NumericType T3;
      NumericType T4;
      NumericType lnT;
      
      Cache(const NumericType &T_in) 
	: T(T_in)
      {
	T2  = T*T;
	T3  = T2*T;
	T4  = T2*T2;
	lnT = std::log(T);	
      }
    private:
      Cache();      
    };
    
    void add_curve_fit( const std::string& species_name, const std::vector<NumericType>& coeffs );

    //! Checks that curve fits have been specified for all species in the mixture.
    bool check() const;

    NumericType cp( const Cache &cache, unsigned int species ) const;

    NumericType cp( const Cache &cache, const std::vector<NumericType>& mass_fractions ) const;

    NumericType cv( const Cache &cache, unsigned int species ) const;

    NumericType cv( const Cache &cache, const std::vector<NumericType>& mass_fractions ) const;

    NumericType h( const Cache &cache, unsigned int species ) const;

    void h( const Cache &cache, std::vector<NumericType>& h ) const;

    NumericType h_RT_minus_s_R( const Cache &cache, unsigned int species ) const;

    void h_RT_minus_s_R( const Cache &cache, std::vector<NumericType>& h_RT_minus_s_R ) const;

    NumericType cp_over_R( const Cache &cache, unsigned int species ) const;

    NumericType h_over_RT( const Cache &cache, unsigned int species ) const;

    NumericType s_over_R( const Cache &cache, unsigned int species ) const;

    const ChemicalMixture<NumericType>& chemical_mixture() const;

  protected:

    const ChemicalMixture<NumericType>& _chem_mixture;

    std::vector<CEACurveFit<NumericType>* > _species_curve_fits;

    std::vector<NumericType> _cp_at_200p1;

  private:
    
    //! Default constructor
    /*! Private to force to user to supply a ChemicalMixture object.*/
    CEAThermodynamics();
  };

  
  /* ------------------------- Inline Functions -------------------------*/

  template<class NumericType>
  inline
  CEAThermodynamics<NumericType>::CEAThermodynamics( const ChemicalMixture<NumericType>& chem_mixture )
    : _chem_mixture(chem_mixture),
      _species_curve_fits(chem_mixture.n_species(), NULL)
  {
    // Read in CEA coefficients. Note this assumes chem_mixture is fully constructed.
    /*! \todo Generalize this to optionally read in a file instead of using the default here.
        The method is there for the reading, just need to do input file foo. */
    read_cea_thermo_data_ascii_default(*this);

    // Cache cp values at small temperatures
    _cp_at_200p1.reserve( _species_curve_fits.size() );
    for( unsigned int s = 0; s < _species_curve_fits.size(); s++ )
      {
	_cp_at_200p1.push_back( this->cp( 200.1, s ) );
      }

    return;
  }


  template<class NumericType>
  inline
  CEAThermodynamics<NumericType>::~CEAThermodynamics()
  {
    // Clean up all the CEACurveFits we created
    for( typename std::vector<CEACurveFit<NumericType>* >::iterator it = _species_curve_fits.begin();
	 it < _species_curve_fits.end(); ++it )
      {
	delete (*it);
      }

    return;
  }


  template<class NumericType>
  inline
  void CEAThermodynamics<NumericType>::add_curve_fit( const std::string& species_name,
						      const std::vector<NumericType>& coeffs )
  {
    antioch_assert( _chem_mixture.active_species_name_map().find(species_name) !=
		    _chem_mixture.active_species_name_map().end() );

    unsigned int s = _chem_mixture.active_species_name_map().find(species_name)->second;

    antioch_assert_less_equal( s, _species_curve_fits.size() );
    antioch_assert( !_species_curve_fits[s] );

    _species_curve_fits[s] = new CEACurveFit<NumericType>(coeffs);
    return;
  }


  template<class NumericType>
  inline
  bool CEAThermodynamics<NumericType>::check() const
  {
    bool valid = true;

    for( typename std::vector<CEACurveFit<NumericType>* >::const_iterator it = _species_curve_fits.begin();
	 it != _species_curve_fits.end(); ++ it )
      {
	if( !(*it) )
	  valid = false;
      }

    return valid;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::cp( const Cache &cache, unsigned int species ) const
  {
    NumericType cp = 0.0;

    if( cache.T < 200.1 )
      {
	cp =  _cp_at_200p1[species];
      }
    else
      {
	cp = this->_chem_mixture.R(species)*this->cp_over_R(cache.T, species);
      }
    
    return cp;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::cp( const Cache &cache,
						  const std::vector<NumericType>& mass_fractions ) const
  {
    antioch_assert_equal_to( mass_fractions.size(), _species_curve_fits.size() );

    NumericType cp = 0.0;

    for( unsigned int s = 0; s < _species_curve_fits.size(); s++ )
      {
	cp += mass_fractions[s]*this->cp(cache,s);
      }

    return cp;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::h( const Cache &cache, unsigned int species ) const
  {
    return this->_chem_mixture.R(species)*cache.T*this->h_over_RT(cache,species);
  }


  template<class NumericType>
  inline
  void CEAThermodynamics<NumericType>::h( const Cache &cache, std::vector<NumericType>& h ) const
  {
    antioch_assert_equal_to( h.size(), _species_curve_fits.size() );

    for( unsigned int s = 0; s < _species_curve_fits.size(); s++ )
      {
	h[s] = this->_chem_mixture.R(s)*cache.T*this->h_over_RT(cache,s);
      }

    return;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::cp_over_R( const Cache &cache, unsigned int species ) const
  {
    antioch_assert_less( species, _species_curve_fits.size() );
    antioch_assert_less( _species_curve_fits[species]->interval(cache.T),
			 _species_curve_fits[species]->n_intervals() );

    const unsigned int interval = this->_species_curve_fits[species]->interval(cache.T);
    
    const NumericType *a = this->_species_curve_fits[species]->coefficients(interval);
    
    /* cp/R =  a0*T^-2   + a1*T^-1     + a2     + a3*T   + a4*T^2   + a5*T^3  + a6*T^4 */
    return a[0]/cache.T2 + a[1]/cache.T + a[2] + a[3]*cache.T + a[4]*cache.T2 + a[5]*cache.T3 + a[6]*cache.T4;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::h_over_RT( const Cache &cache, unsigned int species ) const
  {
    antioch_assert_less( species, _species_curve_fits.size() );
    antioch_assert_less( _species_curve_fits[species]->interval(cache.T),
			 _species_curve_fits[species]->n_intervals() );

    const unsigned int interval = this->_species_curve_fits[species]->interval(cache.T);
    
    const NumericType *a = this->_species_curve_fits[species]->coefficients(interval);
    
    /* h/RT = -a0*T^-2   + a1*T^-1*lnT + a2     + a3*T/2 + a4*T^2/3 + a5*T^3/4 + a6*T^4/5 + a8/T */
    return -a[0]/cache.T2 + a[1]*cache.lnT/cache.T + a[2] + a[3]*cache.T/2.0 + a[4]*cache.T2/3.0 + a[5]*cache.T3/4.0 + a[6]*cache.T4/5.0 + a[8]/cache.T;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::s_over_R( const Cache &cache, unsigned int species ) const
  {
    antioch_assert_less( species, _species_curve_fits.size() );
    antioch_assert_less( _species_curve_fits[species]->interval(cache.T),
			 _species_curve_fits[species]->n_intervals() );

    const unsigned int interval = this->_species_curve_fits[species]->interval(cache.T);
    
    const NumericType *a = this->_species_curve_fits[species]->coefficients(interval);
    
    /* s/R = -a0*T^-2/2 - a1*T^-1     + a2*lnT + a3*T   + a4*T^2/2 + a5*T^3/3 + a6*T^4/4 + a9 */
    return -a[0]/cache.T2/2.0 - a[1]/cache.T + a[2]*cache.lnT + a[3]*cache.T + a[4]*cache.T2/2.0 + a[5]*cache.T3/3.0 + a[6]*cache.T4/4.0 + a[9];
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::h_RT_minus_s_R( const Cache &cache, unsigned int species ) const
  {
    antioch_assert_less( species, _species_curve_fits.size() );
    antioch_assert_less( _species_curve_fits[species]->interval(cache.T),
			 _species_curve_fits[species]->n_intervals() );

    const unsigned int interval = this->_species_curve_fits[species]->interval(cache.T);
    
    const NumericType *a = this->_species_curve_fits[species]->coefficients(interval);
    
    /* h/RT = -a[0]/T2    + a[1]*lnT/T + a[2]     + a[3]*T/2. + a[4]*T2/3. + a[5]*T3/4. + a[6]*T4/5. + a[8]/T,
       s/R  = -a[0]/T2/2. - a[1]/T     + a[2]*lnT + a[3]*T    + a[4]*T2/2. + a[5]*T3/3. + a[6]*T4/4. + a[9]   */
    return -a[0]/cache.T2/2.0 + (a[1] + a[8])/cache.T + a[1]*cache.lnT/cache.T - a[2]*cache.lnT + (a[2] - a[9]) - a[3]*cache.T/2.0 - a[4]*cache.T2/6.0 - a[5]*cache.T3/12.0 - a[6]*cache.T4/20.0;
  }


  template<class NumericType>
  inline
  void CEAThermodynamics<NumericType>::h_RT_minus_s_R( const Cache &cache,
						       std::vector<NumericType>& h_RT_minus_s_R ) const
  {
    antioch_assert_equal_to( h_RT_minus_s_R.size(), _species_curve_fits.size() );

    for( unsigned int s = 0; s < _species_curve_fits.size(); s++ )
      {
	h_RT_minus_s_R[s] = this->h_RT_minus_s_R(cache,s);
      }

    return;
  }


  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::cv( const Cache &cache, unsigned int species ) const
  {
    return this->cp(cache,species) - _chem_mixture.R(species);
  }

  template<class NumericType>
  inline
  NumericType CEAThermodynamics<NumericType>::cv( const Cache &cache,
						  const std::vector<NumericType>& mass_fractions ) const
  {
    return this->cp(cache,mass_fractions) - _chem_mixture.R(mass_fractions);
  }

  template<class NumericType>
  inline
  const ChemicalMixture<NumericType>& CEAThermodynamics<NumericType>::chemical_mixture() const
  {
    return _chem_mixture;
  }

} // end namespace Antioch

#endif //ANTIOCH_CEA_THERMO_H
