// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file    protocols/grafting/DeleteRegionMover.hh
/// @brief   Base class for graftmovers
/// @author  Jared Adolf-Bryfogle

#ifndef INCLUDED_protocols_grafting_DeleteRegionMover_HH
#define INCLUDED_protocols_grafting_DeleteRegionMover_HH

#include <protocols/grafting/simple_movers/DeleteRegionMover.fwd.hh>

#include <protocols/moves/Mover.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/select/residue_selector/ResidueSelector.fwd.hh>


namespace protocols {
namespace grafting {
namespace simple_movers {

/// @brief Delete a region of a pose. Mover Wrapper to grafting utility function.
class DeleteRegionMover : public  protocols::moves::Mover {

public:

	DeleteRegionMover();
	DeleteRegionMover( core::Size const res_start, core::Size const res_end );

	DeleteRegionMover( DeleteRegionMover const & src );

	~DeleteRegionMover() override;

	void
	apply( core::pose::Pose & pose ) override;


public:

	/// @brief Set the region of the pose where deletion will occur
	void
	region( std::string const & res_start, std::string const & res_end );

	/// @brief Sets the residue selector
	void
	set_residue_selector( core::select::residue_selector::ResidueSelectorCOP selector );

public:

	void set_rechain( bool rechain ) { rechain_ = rechain; }
	void set_add_terminal_types_on_rechain( bool add ) { add_terminal_types_on_rechain_ = add; }
	void set_add_jump_on_rechain( bool add ) { add_jump_on_rechain_ = add; }

	protocols::moves::MoverOP
	clone() const override;

	protocols::moves::MoverOP
	fresh_instance() const override;

	void
	parse_my_tag(
		TagCOP tag,
		basic::datacache::DataMap & data
	) override;

	std::string
	get_name() const override;

	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );


public: //CitationManager

	/// @brief Provide the citation.
	void provide_citation_info(basic::citation_manager::CitationCollectionList & ) const override;

private:
	/// @brief Adds terminal variants to residues resid and resid-1
	/// @param[in,out] pose  Pose to be modified
	/// @param[in]     resid Residue number for the residue that would have the lower terminus variant
	/// @details Residue resid-1 will have upper_terminus variant, and residue resid will have
	///          lower_terminus variant
	void
	add_terminus_variants( core::pose::Pose & pose, core::Size const resid ) const;

	/// @brief Adds cutpoint variants to residues resid and resid-1
	/// @param[in,out] pose  Pose to be modified
	/// @param[in]     resid Residue number for the residue that would have the lower cutpoint variant
	/// @details Residue resid-1 will have upper_cutpoint variant, and residue resid will have
	///          lower_cutpoit variant
	void
	add_cutpoint_variants( core::pose::Pose & pose, core::Size const resid ) const;

private:
	core::select::residue_selector::ResidueSelectorCOP selector_;
	core::Size nter_overhang_;
	core::Size cter_overhang_;
	bool rechain_;
	bool add_terminal_types_on_rechain_;
	bool add_jump_on_rechain_;
	bool detect_disulfides_ = true;

};


}
}
}

#endif  // INCLUDED_protocols_grafting_DeleteRegionMover_HH
