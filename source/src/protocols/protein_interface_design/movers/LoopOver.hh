// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington CoMotion, email: license@uw.edu.

/// @file protocols/protein_interface_design/movers/LoopOver.hh
/// @author Sarel Fleishman (sarelf@u.washington.edu), Jacob Corn (jecorn@u.washington.edu)

#ifndef INCLUDED_protocols_protein_interface_design_movers_LoopOver_hh
#define INCLUDED_protocols_protein_interface_design_movers_LoopOver_hh

#include <core/types.hh>
#include <core/pose/Pose.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <protocols/moves/Mover.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/moves/MoverStatus.hh>



namespace protocols {
namespace protein_interface_design {
namespace movers {

/// @brief essentially the same as the WhileMover but allows parsing and cloning. Will be removed at a future point.
/// This should now be incorporated into WhileMover
class LoopOver : public protocols::moves::Mover {
public:
	LoopOver();
	LoopOver(
		core::Size max_iterations,
		protocols::moves::MoverCOP mover,
		protocols::filters::FilterCOP condition,
		protocols::moves::MoverStatus ms_whenfail = protocols::moves::MS_SUCCESS
	);

	protocols::moves::MoverOP clone() const override {
		return utility::pointer::make_shared< LoopOver >( *this );
	}
	protocols::moves::MoverOP fresh_instance() const override { return utility::pointer::make_shared< LoopOver >(); }
	void apply( core::pose::Pose & pose ) override;
	void parse_my_tag( utility::tag::TagCOP tag, basic::datacache::DataMap & ) override;
	~LoopOver() override;

	std::string
	get_name() const override;

	static
	std::string
	mover_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

public: //Functions needed for the citation manager

	/// @brief Provide the citation.
	void provide_citation_info(basic::citation_manager::CitationCollectionList & ) const override;

private:
	core::Size max_iterations_;
	protocols::moves::MoverOP mover_;
	protocols::filters::FilterOP condition_;
	bool drift_; // do we want to allow the pose to drift or return to its original state after each iteration
	protocols::moves::MoverStatus ms_whenfail_; // MoverStatus when filter failed after the iterations ( default MS_SUCCESS )
};

} // movers
} // protein_interface_design
} // protocols


#endif /*INCLUDED_protocols_protein_interface_design_movers_LoopOver_HH*/
