// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/cyclic_peptide/crosslinker/1_4_BBMB_Helper.cc
/// @brief A derived class of the CrosslinkerMoverHelper base class, used to set up
/// the 1,4-bis(bromomethyl)benzene (BBMB) cross-linker.
/// @author Vikram K. Mulligan (vmullig@uw.edu)

// Unit headers
#include <protocols/cyclic_peptide/crosslinker/1_4_BBMB_Helper.hh>

// Core headers
#include <core/pose/Pose.hh>
#include <core/conformation/symmetry/SymmetricConformation.hh>
#include <core/conformation/symmetry/SymmetryInfo.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/ResidueFactory.hh>
#include <core/chemical/ResidueType.hh>
#include <core/chemical/ResidueTypeSet.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/select/residue_selector/ResidueIndexSelector.hh>
#include <core/scoring/rms_util.hh>
#include <core/id/AtomID.hh>
#include <core/chemical/AA.hh>

// Protocols headers
#include <protocols/simple_moves/ModifyVariantTypeMover.hh>
#include <protocols/simple_moves/DeclareBond.hh>
#include <protocols/cyclic_peptide/CreateDistanceConstraint.hh>
#include <protocols/cyclic_peptide/CreateTorsionConstraint.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>

static basic::Tracer TR( "protocols.cyclic_peptide.crosslinker.1_4_BBMB_Helper" );

namespace protocols {
namespace cyclic_peptide {
namespace crosslinker {

/////////////////////
/// Constructors  ///
/////////////////////

/// @brief Default constructor
One_Four_BBMB_Helper::One_Four_BBMB_Helper() = default;

////////////////////////////////////////////////////////////////////////////////
/// @brief Copy constructor
////////////////////////////////////////////////////////////////////////////////
One_Four_BBMB_Helper::One_Four_BBMB_Helper( One_Four_BBMB_Helper const &/*src*/ ) = default;

////////////////////////////////////////////////////////////////////////////////
/// @brief Destructor (important for properly forward-declaring smart-pointer
/// members)
////////////////////////////////////////////////////////////////////////////////
One_Four_BBMB_Helper::~One_Four_BBMB_Helper()= default;

//////////////////////
/// Public Methods ///
//////////////////////

/// @brief Provide an opportunity to provide a citation for this crosslinker type.
/// @details The base class implementation does nothing.  This override indicates that this helper is
/// unpublished.
void
One_Four_BBMB_Helper::provide_citation_info(
	basic::citation_manager::CitationCollectionList & citations
) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"One_Four_BBMB_Helper", basic::citation_manager::CitedModuleType::CrosslinkerMoverHelper,
		"Vikram K. Mulligan", "Systems Biology Group, Center for Computational Biology, Flatiron Institute",
		"vmulligan@flatironinstitute.org", "Added support for 1,4-bis(bromomethyl)benzene crosslinking to the CrosslinkerMover."
		)
	);
}

/// @brief Given a pose and a selection of exactly two residues, add the BBMB linker,
/// align it crudely to the selected residues, and set up covalent bonds.
void
One_Four_BBMB_Helper::add_linker_asymmetric(
	core::pose::Pose &pose,
	core::select::residue_selector::ResidueSubset const & selection
) const {
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2);
	core::Size cys1( res_indices[1] ), cys2( res_indices[2] );

	runtime_assert_string_msg( pose.residue_type(cys1).aa() == core::chemical::aa_cys || pose.residue_type(cys1).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_asymmetric(): The first residue selected by the ResidueSelector is not an L- or D-cysteine.");
	runtime_assert_string_msg( pose.residue_type(cys2).aa() == core::chemical::aa_cys || pose.residue_type(cys2).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_asymmetric(): The second residue selected by the ResidueSelector is not an L- or D-cysteine.");

	//Mutate the cysteines to CYX:
	protocols::simple_moves::ModifyVariantTypeMover mut1;
	core::select::residue_selector::ResidueIndexSelectorOP index_selector(
		utility::pointer::make_shared< core::select::residue_selector::ResidueIndexSelector >()
	);
	for ( auto const index : res_indices ) {
		index_selector->append_index( index );
	}
	mut1.set_residue_selector( index_selector );
	mut1.set_additional_type_to_add( "SIDECHAIN_CONJUGATION" );
	mut1.apply(pose);

	//Create the BBMB and put it into a pose of its own:
	core::chemical::ResidueTypeSetCOP standard_residues( core::chemical::ChemicalManager::get_instance()->residue_type_set( core::chemical::FA_STANDARD ) );
	core::conformation::ResidueOP new_rsd( core::conformation::ResidueFactory::create_residue( standard_residues->name_map("paraBBMB") ) );
	core::pose::Pose bbmb_pose;
	bbmb_pose.append_residue_by_jump(*new_rsd, 1);

	//Align the BBMB pose to the original pose:
	std::map <core::id::AtomID, core::id::AtomID> alignment_atoms;
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("V1"), 1) ] = core::id::AtomID( pose.residue_type(cys1).atom_index("SG"), cys1);
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("V2"), 1) ] = core::id::AtomID( pose.residue_type(cys2).atom_index("SG"), cys2);
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("CM1"), 1) ] = core::id::AtomID( pose.residue_type(cys1).atom_index("V1"), cys1);
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("CM2"), 1) ] = core::id::AtomID( pose.residue_type(cys2).atom_index("V1"), cys2);
	core::scoring::superimpose_pose( bbmb_pose, pose, alignment_atoms );

	//Merge the poses:
	pose.append_residue_by_jump( bbmb_pose.residue(1), cys1 );
	core::Size const bbmb_res( pose.total_residue() );

	//Declare covalent bonds:
	add_linker_bonds_asymmetric( pose, res_indices, bbmb_res );
}

/// @brief Given a pose and a linker, add bonds between the linker and the residues that coordinate the linker.
/// @details Called by add_linker_asymmetric().  Version for asymmetric poses.
void
One_Four_BBMB_Helper::add_linker_bonds_asymmetric(
	core::pose::Pose &pose,
	utility::vector1< core::Size > const &res_indices,
	core::Size const linker_index
) const {
	runtime_assert_string_msg( res_indices.size() == 2, "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_bonds_asymmetric(): The wrong number of residues was passed to this function.  A vector of exactly two residues is expected." );

	//Declare covalent bonds:
	protocols::simple_moves::DeclareBond bond1, bond2;
	bond1.set( linker_index, "CM1", res_indices[1], "SG", false );
	bond2.set( linker_index, "CM2", res_indices[2], "SG", false );
	bond1.apply(pose);
	bond2.apply(pose);
}

/// @brief Given a pose and a selection of exactly two residues, add the BBMB linker,
/// align it crudely to the selected residues, and set up covalent bonds.
/// @details Version for symmetric poses.
void
One_Four_BBMB_Helper::add_linker_symmetric(
	core::pose::Pose &pose,
	core::select::residue_selector::ResidueSubset const & selection
) const {
	std::string const errmsg( "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_symmetric(): " );
	confirm_symmetry(errmsg);

	core::select::residue_selector::ResidueIndexSelectorOP index_selector(
		utility::pointer::make_shared< core::select::residue_selector::ResidueIndexSelector >()
	);
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2);
	core::Size cys1( res_indices[1] ), cys2( res_indices[2] );
	index_selector->append_index( cys1 );

	runtime_assert_string_msg( pose.residue_type(cys1).aa() == core::chemical::aa_cys || pose.residue_type(cys1).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_asymmetric(): The first residue selected by the ResidueSelector is not an L- or D-cysteine.");
	runtime_assert_string_msg( pose.residue_type(cys2).aa() == core::chemical::aa_cys || pose.residue_type(cys2).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_asymmetric(): The second residue selected by the ResidueSelector is not an L- or D-cysteine.");

	//Mutate the cysteines to CYX:
	{
		protocols::simple_moves::ModifyVariantTypeMover mut1;
		mut1.set_residue_selector( index_selector );
		mut1.set_additional_type_to_add( "SIDECHAIN_CONJUGATION" );
		mut1.apply(pose);
	}

	//Create the BBMB and put it into a pose of its own:
	core::chemical::ResidueTypeSetCOP standard_residues( core::chemical::ChemicalManager::get_instance()->residue_type_set( core::chemical::FA_STANDARD ) );
	core::conformation::ResidueOP new_rsd( core::conformation::ResidueFactory::create_residue( standard_residues->name_map("paraBBMB_symm") ) );
	core::pose::Pose bbmb_pose;
	bbmb_pose.append_residue_by_jump(*new_rsd, 1);

	//Align the BBMB pose to the original pose:
	std::map <core::id::AtomID, core::id::AtomID> alignment_atoms;
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("V1"), 1) ] = core::id::AtomID( pose.residue_type(cys1).atom_index("SG"), cys1);
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("CM1"), 1) ] = core::id::AtomID( pose.residue_type(cys1).atom_index("V1"), cys1);
	alignment_atoms[ core::id::AtomID( new_rsd->type().atom_index("V4"), 1) ] = core::id::AtomID( pose.residue_type(cys2).atom_index("V1"), cys2); //Imperfect alignment, but minimization will take care of it.
	core::scoring::superimpose_pose( bbmb_pose, pose, alignment_atoms );

	//Merge the poses and store indices of the new linker residues:
	core::conformation::symmetry::SymmetricConformationCOP symmconf( utility::pointer::dynamic_pointer_cast< core::conformation::symmetry::SymmetricConformation const>( pose.conformation_ptr() ) );
	debug_assert( symmconf != nullptr ); //Should be true.
	core::conformation::symmetry::SymmetryInfoCOP symminfo( symmconf->Symmetry_Info() );
	debug_assert( symminfo != nullptr ); //Should also be true.
	core::Size const res_per_subunit( symminfo->num_independent_residues() );
	core::Size const bbmb_res1( res_per_subunit + 1 );
	core::Size const bbmb_res2( 2*res_per_subunit + 2 );
	pose.append_residue_by_jump( bbmb_pose.residue(1), cys1 );

	add_linker_bonds_symmetric(pose, cys1, bbmb_res1, bbmb_res2);
}

/// @brief Given a pose and a linker, add bonds between the BBMB linker and the residues that coordinate the linker.
/// @details Called by add_linker_symmetric().  Version for symmetric poses.
void
One_Four_BBMB_Helper::add_linker_bonds_symmetric(
	core::pose::Pose &pose,
	core::Size const res1,
	core::Size const linker_index1,
	core::Size const linker_index2
) const {
	std::string errmsg("Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_bonds_symmetric(): ");
	confirm_symmetry(errmsg);

	//Declare covalent bonds:
	protocols::simple_moves::DeclareBond bond1, bond2;
	bond1.set( linker_index1, "CM1", res1, "SG", false );
	bond2.set( linker_index1, "C1", linker_index2, "C5", false );
	bond1.apply(pose);
	bond2.apply(pose);
}

/// @brief Given a selection of exactly two residues that have already been connected to a 1,4-bis(bromomethyl)benzene crosslinker,
/// add constraints for the crosslinker.
void
One_Four_BBMB_Helper::add_linker_constraints_asymmetric(
	core::pose::Pose &pose,
	core::select::residue_selector::ResidueSubset const & selection
) const {
	//Get indices of residues
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2 );
	core::Size const bbmb_index( get_linker_index_asymmetric( pose, res_indices) );

	//Set up distance constraints between BBMB and cysteines:
	{ //Begin scope
		std::string const dist_cst_string("HARMONIC 0.0 0.01");
		protocols::cyclic_peptide::CreateDistanceConstraint dist_csts;
		utility::vector1< core::Size > res1(4), res2(4);
		utility::vector1< std::string > atom1(4), atom2(4);
		utility::vector1< std::string > cst_fxn(4);
		res1[1] = res_indices[1]; res1[2] = res_indices[2];
		atom2[1] = "V1"; atom2[2] = "V2";
		res1[3] = res_indices[1]; res1[4] = res_indices[2];
		atom2[3] = "CM1"; atom2[4] = "CM2";
		for ( core::Size i=1; i<=2; ++i ) {
			atom1[i]="SG"; res2[i]=bbmb_index; cst_fxn[i]=dist_cst_string;
		}
		for ( core::Size i=3; i<=4; ++i ) {
			atom1[i]="V1"; res2[i]=bbmb_index; cst_fxn[i]=dist_cst_string;
		}
		if ( TR.Debug.visible() ) {
			TR.Debug << "R1\tA1\tR2\tA2\tFUNC\n";
			for ( core::Size i=1; i<=6; ++i ) {
				TR.Debug << res1[i] << "\t";
				TR.Debug << atom1[i] << "\t";
				TR.Debug << res2[i] << "\t";
				TR.Debug << atom2[i] << "\t";
				TR.Debug << cst_fxn[i] << "\n";
			}
			TR.Debug << std::endl;
		}
		dist_csts.set( res1, atom1, res2, atom2, cst_fxn );
		dist_csts.apply(pose);
	} //End of scope

	//Set up torsion constraints:
	{ //Begin scope
		protocols::cyclic_peptide::CreateTorsionConstraint tors_csts;
		utility::vector1 < core::Size > res1(6), res2(6), res3(6), res4(6);
		utility::vector1 < std::string > atom1(6), atom2(6), atom3(6), atom4(6);
		utility::vector1 < std::string > cst_fxns(6);

		//CM#-SG-CB-CA should be three-well potential:
		res1[1] = bbmb_index; res2[1] = res_indices[1]; res3[1] = res_indices[1]; res4[1] = res_indices[1];
		atom1[1] = "CM1"; atom2[1] = "SG"; atom3[1] = "CB"; atom4[1] = "CA";
		cst_fxns[1] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)
		res1[2] = bbmb_index; res2[2] = res_indices[2]; res3[2] = res_indices[2]; res4[2] = res_indices[2];
		atom1[2] = "CM2"; atom2[2] = "SG"; atom3[2] = "CB"; atom4[2] = "CA";
		cst_fxns[2] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		//C#-C#-CM#-SG should be two-well potential (above/below plane).
		res1[3] = bbmb_index; res2[3] = bbmb_index; res3[3] = bbmb_index; res4[3] = res_indices[1];
		atom1[3] = "C2"; atom2[3] = "C1"; atom3[3] = "CM1"; atom4[3] = "SG";
		cst_fxns[3] = "AMBERPERIODIC 0 2 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)
		res1[4] = bbmb_index; res2[4] = bbmb_index; res3[4] = bbmb_index; res4[4] = res_indices[2];
		atom1[4] = "C5"; atom2[4] = "C4"; atom3[4] = "CM2"; atom4[4] = "SG";
		cst_fxns[4] = "AMBERPERIODIC 0 2 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		//C#-CM#-SG-CB should be three-well potential:
		res1[5] = bbmb_index; res2[5] = bbmb_index; res3[5] = res_indices[1]; res4[5] = res_indices[1];
		atom1[5] = "C1"; atom2[5] = "CM1"; atom3[5] = "SG"; atom4[5] = "CB";
		cst_fxns[5] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)
		res1[6] = bbmb_index; res2[6] = bbmb_index; res3[6] = res_indices[2]; res4[6] = res_indices[2];
		atom1[6] = "C4"; atom2[6] = "CM2"; atom3[6] = "SG"; atom4[6] = "CB";
		cst_fxns[6] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		if ( TR.Debug.visible() ) {
			TR.Debug << "R1\tA1\tR2\tA2\tR3\tA3\tR4\tA4\tFUNC\n";
			for ( core::Size i=1; i<=9; ++i ) {
				TR.Debug << res1[i] << "\t";
				TR.Debug << atom1[i] << "\t";
				TR.Debug << res2[i] << "\t";
				TR.Debug << atom2[i] << "\t";
				TR.Debug << res3[i] << "\t";
				TR.Debug << atom3[i] << "\t";
				TR.Debug << res4[i] << "\t";
				TR.Debug << atom4[i] << "\t";
				TR.Debug << cst_fxns[i] << "\n";
			}
			TR.Debug << std::endl;
		}

		tors_csts.set( res1, atom1, res2, atom2, res3, atom3, res4, atom4, cst_fxns );
		tors_csts.apply( pose );
	} //End of scope

}

/// @brief Given a selection of exactly two residues that have already been connected to a 1,4-bis(bromomethyl)benzene crosslinker,
/// add constraints for the crosslinker.  This version is for symmetric poses.
void
One_Four_BBMB_Helper::add_linker_constraints_symmetric(
	core::pose::Pose &pose,
	core::select::residue_selector::ResidueSubset const & selection,
	bool const linker_was_added
) const {
	std::string errmsg( "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::add_linker_constraints_symmetric(): " );
	confirm_symmetry(errmsg);

	//Get indices of residues
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2 );
	if ( linker_was_added ) {
		res_indices[2] += 1;
	}
	utility::vector1< core::Size > bbmb_indices;
	get_linker_indices_symmetric( pose, res_indices, bbmb_indices );
	runtime_assert( bbmb_indices.size() == 2 );

	//Set up distance constraints:
	for ( core::Size i=1; i<=2; ++i ) {
		core::Size r1, t1, t2;
		if ( i==1 ) { r1=res_indices[1] ; t1=bbmb_indices[1]; t2=bbmb_indices[2]; }
		else { r1=res_indices[2]; t1=bbmb_indices[2]; t2=bbmb_indices[1]; }

		std::string const dist_cst_string("HARMONIC 0.0 0.01");
		protocols::cyclic_peptide::CreateDistanceConstraint dist_csts;
		utility::vector1< core::Size > res1(5), res2(5);
		utility::vector1< std::string > atom1(5), atom2(5);
		utility::vector1< std::string > cst_fxn(5);

		res1[1] = r1; res2[1] = t1; atom1[1]="SG"; atom2[1] = "V1";
		res1[2] = r1; res2[2] = t1; atom1[2]="V1"; atom2[2] = "CM1";
		res1[3] = t1; res2[3] = t2; atom1[3]="C1"; atom2[3] = "V4";
		res1[4] = t1; res2[4] = t2; atom1[4]="C6"; atom2[4] = "V3";
		res1[5] = t1; res2[5] = t2; atom1[5]="C5"; atom2[5] = "V2";

		for ( core::Size ii=1; ii<=5; ++ii ) { cst_fxn[ii] = dist_cst_string; }

		if ( TR.Debug.visible() ) {
			TR.Debug << "\nSYMM REPEAT " << i << ":\nR1\tA1\tR2\tA2\tFUNC\n";
			for ( core::Size ii=1; ii<=3; ++ii ) {
				TR.Debug << res1[ii] << "\t";
				TR.Debug << atom1[ii] << "\t";
				TR.Debug << res2[ii] << "\t";
				TR.Debug << atom2[ii] << "\t";
				TR.Debug << cst_fxn[ii] << "\n";
			}
			TR.Debug << std::endl;
		}
		dist_csts.set( res1, atom1, res2, atom2, cst_fxn );
		dist_csts.apply(pose);
	} //End of for loop from 1 to 3 for distance constraints.

	//Set up torsion constraints:
	for ( core::Size i=1; i<=2; ++i ) {
		core::Size r1, t1;
		if ( i==1 ) { r1=res_indices[1]; t1=bbmb_indices[1]; }
		else { r1=res_indices[2]; t1=bbmb_indices[2]; }

		protocols::cyclic_peptide::CreateTorsionConstraint tors_csts;
		utility::vector1 < core::Size > res1(3), res2(3), res3(3), res4(3);
		utility::vector1 < std::string > atom1(3), atom2(3), atom3(3), atom4(3);
		utility::vector1 < std::string > cst_fxns(3);

		//CM#-SG-CB-CA should be three-well potential:
		res1[1] = t1; res2[1] = r1; res3[1] = r1; res4[1] = r1;
		atom1[1] = "CM1"; atom2[1] = "SG"; atom3[1] = "CB"; atom4[1] = "CA";
		cst_fxns[1] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		//C#-C#-CM#-SG should be two-well potential (above/below plane).
		res1[2] = t1; res2[2] = t1; res3[2] = t1; res4[2] = r1;
		atom1[2] = "C6"; atom2[2] = "C1"; atom3[2] = "CM1"; atom4[2] = "SG";
		cst_fxns[2] = "AMBERPERIODIC 0 2 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		//C#-CM#-SG-CB should be three-well potential:
		res1[3] = t1; res2[3] = t1; res3[3] = r1; res4[3] = r1;
		atom1[3] = "C1"; atom2[3] = "CM1"; atom3[3] = "SG"; atom4[3] = "CB";
		cst_fxns[3] = "AMBERPERIODIC 0 3 2"; //AMBERPERIODIC has maximum at x=x0; params are x0 (offset), N (periodicity), K (amplitude)

		if ( TR.Debug.visible() ) {
			TR.Debug << "\nSYMM SET " << i << ":\nR1\tA1\tR2\tA2\tR3\tA3\tR4\tA4\tFUNC\n";
			for ( core::Size ii=1; ii<=3; ++ii ) {
				TR.Debug << res1[ii] << "\t";
				TR.Debug << atom1[ii] << "\t";
				TR.Debug << res2[ii] << "\t";
				TR.Debug << atom2[ii] << "\t";
				TR.Debug << res3[ii] << "\t";
				TR.Debug << atom3[ii] << "\t";
				TR.Debug << res4[ii] << "\t";
				TR.Debug << atom4[ii] << "\t";
				TR.Debug << cst_fxns[ii] << "\n";
			}
			TR.Debug << std::endl;
		}

		tors_csts.set( res1, atom1, res2, atom2, res3, atom3, res4, atom4, cst_fxns );
		tors_csts.apply( pose );
	} //End of scope for torsion constraints.

}

/// @brief Given indices of two cysteine residues that are already linked to a BBMB, get the index
/// of the BBMB residue.
/// @details Throws an error if the two cysteines are not all linked to the same BBMB residue.
core::Size
One_Four_BBMB_Helper::get_linker_index_asymmetric(
	core::pose::Pose const &pose,
	utility::vector1< core::Size > const & res_indices
) const {
	std::string const errmsg( "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::get_linker_index_asymmetric(): " );
	runtime_assert_string_msg( res_indices.size() == 2, errmsg + "The wrong number of residues was passed to this function.  A vector of exactly two residues is expected." );

	core::Size const nconn1( pose.residue(res_indices[1]).n_possible_residue_connections() );
	core::Size const nconn2( pose.residue(res_indices[2]).n_possible_residue_connections() );

	core::Size bbmb_index( pose.residue(res_indices[1]).residue_connection_partner(nconn1) );

	runtime_assert_string_msg( pose.residue(bbmb_index).name3() == "BB4",
		errmsg + "The residue connected to the side-chain of the first cysteine is not 1,4-BBMB." );
	runtime_assert_string_msg( pose.residue(res_indices[2]).residue_connection_partner(nconn2) == bbmb_index,
		errmsg + "The residue connected to the side-chain of the first cysteine is not the same as the residue connected to the side-chain of the second cysteine." );

	return bbmb_index;
}

/// @brief Given indices of two cysteine residues that are already linked to pieces of a linker, get
/// of the indices of the symmetric pieces of the linker.
/// @details Throws an error if a residue is not linked to something.  Must be defined by derived classes.
void
One_Four_BBMB_Helper::get_linker_indices_symmetric(
	core::pose::Pose const &pose,
	utility::vector1< core::Size > const & res_indices,
	utility::vector1< core::Size > & linker_indices
) const {
	std::string const errmsg( "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::get_linker_indices_symmetric(): " );
	confirm_symmetry(errmsg);

	runtime_assert_string_msg( res_indices.size() == 2, errmsg + "The wrong number of residues was passed to this function.  A vector of exactly two residues is expected." );

	core::Size const nconn1( pose.residue(res_indices[1]).n_possible_residue_connections() );
	core::Size const nconn2( pose.residue(res_indices[2]).n_possible_residue_connections() );

	linker_indices.resize(2);
	linker_indices[1] = pose.residue(res_indices[1]).residue_connection_partner(nconn1);
	linker_indices[2] = pose.residue(res_indices[2]).residue_connection_partner(nconn2);

	for ( core::Size i(1); i<=2; ++i ) {
		runtime_assert_string_msg( linker_indices[i] > 0, errmsg + "One of the cysteine residues is not connected to anything!" );
		runtime_assert_string_msg( pose.residue(linker_indices[i]).name3() == "B4S",
			errmsg + "The residue connected to the side-chain of one of the cysteine residues is not a symmetric fragment of 1,4-BBMB." );
	}
}

/// @brief Given a pose with residues selected to be linked by a 1,4-bis(bromomethyl)benzene crosslinker,
/// determine whether the residues are too far apart.
/// @details Returns TRUE for failure (residues too far apart) and FALSE for success.
bool
One_Four_BBMB_Helper::filter_by_sidechain_distance_asymmetric(
	core::pose::Pose const &pose,
	core::select::residue_selector::ResidueSubset const & selection,
	core::Real const &filter_multiplier
) const {

	core::Real const hardcoded_cutoff( 12.0 * filter_multiplier );
	core::Real const hardcoded_cutoff_sq( hardcoded_cutoff*hardcoded_cutoff );

	//Get indices of residues
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2 );

	runtime_assert_string_msg( pose.residue_type(res_indices[1]).aa() == core::chemical::aa_cys || pose.residue_type(res_indices[1]).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_sidechain_distance_asymmetric(): The first residue selected is not D- or L-cysteine." );
	runtime_assert_string_msg( pose.residue_type(res_indices[2]).aa() == core::chemical::aa_cys || pose.residue_type(res_indices[2]).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_sidechain_distance_asymmetric(): The second residue selected is not D- or L-cysteine." );

	core::Real const d1( pose.residue(res_indices[1]).xyz("CB").distance_squared( pose.residue(res_indices[2]).xyz("CB") ) );
	if ( TR.Debug.visible() ) {
		TR.Debug << "D1: " << sqrt(d1) << " MAX: " << hardcoded_cutoff << std::endl;
	}
	return ( d1 > hardcoded_cutoff_sq );
}

/// @brief Given a pose with residues selected to be linked by a 1,4-bis(bromomethyl)benzene crosslinker,
/// determine whether the residues are too far apart.  This version is for symmetric poses.
/// @details Returns TRUE for failure (residues too far apart) and FALSE for success.
bool
One_Four_BBMB_Helper::filter_by_sidechain_distance_symmetric(
	core::pose::Pose const &pose,
	core::select::residue_selector::ResidueSubset const & selection,
	core::Real const &filter_multiplier
) const {
	std::string const errmsg("Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_sidechain_distance_symmetric(): ");
	confirm_symmetry(errmsg);

	core::Real const hardcoded_cutoff( 12.0 * filter_multiplier );
	core::Real const hardcoded_cutoff_sq( hardcoded_cutoff*hardcoded_cutoff );

	//Get indices of residues
	utility::vector1< core::Size > res_indices;
	CrosslinkerMoverHelper::get_sidechain_indices( selection, res_indices );
	runtime_assert( res_indices.size() == 2 );

	runtime_assert_string_msg( pose.residue_type(res_indices[1]).aa() == core::chemical::aa_cys || pose.residue_type(res_indices[1]).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_sidechain_distance_symmetric(): The first residue selected is not D- or L-cysteine." );
	runtime_assert_string_msg( pose.residue_type(res_indices[2]).aa() == core::chemical::aa_cys || pose.residue_type(res_indices[2]).aa() == core::chemical::aa_dcs,
		"Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_sidechain_distance_symmetric(): The second residue selected is not D- or L-cysteine." );

	core::Real const d1( pose.residue(res_indices[1]).xyz("CB").distance_squared( pose.residue(res_indices[2]).xyz("CB") ) );
	if ( TR.Debug.visible() ) {
		TR.Debug << "D1: " << sqrt(d1) << " MAX: " << hardcoded_cutoff << std::endl;
	}
	return ( d1 > hardcoded_cutoff_sq );
}

/// @brief Determine whether the sidechain-crosslinker system has too high a constraints score.
/// @details Returns TRUE for failure (too high a constraints score) and FALSE for success.
bool
One_Four_BBMB_Helper::filter_by_constraints_energy_asymmetric(
	core::pose::Pose const &pose,
	core::select::residue_selector::ResidueSubset const & selection,
	core::Real const &filter_multiplier
) const {
	return filter_by_constraints_energy( pose, selection, false, false, filter_multiplier );
}

/// @brief Determine whether the sidechain-crosslinker system has too high a constraints score.  This version is for symmetric poses.
/// @details Returns TRUE for failure (too high a constraints score) and FALSE for success.
bool
One_Four_BBMB_Helper::filter_by_constraints_energy_symmetric(
	core::pose::Pose const &pose,
	core::select::residue_selector::ResidueSubset const & selection,
	bool const linker_was_added,
	core::Real const &filter_multiplier
) const {
	std::string const errmsg( "Error in protocols::cyclic_peptide::crosslinker::One_Four_BBMB_Helper::filter_by_constraints_energy_symmetric(): " );
	confirm_symmetry(errmsg);

	return filter_by_constraints_energy( pose, selection, true, linker_was_added, filter_multiplier );
}

/********************************************
PRIVATE FUNCTIONS
*********************************************/

/// @brief Confirm that the symmetry type is correct.
/// @details Throws with an error message if is it is not.
void
One_Four_BBMB_Helper::confirm_symmetry(
	std::string const & errmsg_header
) const {
	runtime_assert_string_msg(
		(symm_count() == 2 && symm_type() == 'C') || (symm_count() == 2 && symm_type() == 'S'),
		errmsg_header + "BBMB requires a C2- or S2-symmetric pose."
	);
}

} //crosslinker
} //protocols
} //cyclic_peptide
