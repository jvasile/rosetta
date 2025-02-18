// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file   src/core/scoring/dna/DNA_EnvPairPotential.fwd.hh
/// @brief  dna scoring
/// @author Phil Bradley
/// @author Vikram K. Mulligan (vmullig@uw.edu) -- defined owning pointers for DNA_EnvPairPotential

#ifndef INCLUDED_core_scoring_dna_DNA_EnvPairPotential_FWD_HH
#define INCLUDED_core_scoring_dna_DNA_EnvPairPotential_FWD_HH

/// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace scoring {
namespace dna {


class DNA_EnvPairPotential;

typedef utility::pointer::shared_ptr< DNA_EnvPairPotential > DNA_EnvPairPotentialOP;
typedef utility::pointer::shared_ptr< DNA_EnvPairPotential const > DNA_EnvPairPotentialCOP;

} // namespace dna
} // scoring
} // core

#endif
