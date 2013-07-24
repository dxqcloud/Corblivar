/*
 * =====================================================================================
 *
 *    Description:  Corblivar thermal analyzer, based on power blurring
 *
 *    Copyright (C) 2013 Johann Knechtel, johann.knechtel@ifte.de, www.ifte.de
 *
 *    This file is part of Corblivar.
 *    
 *    Corblivar is free software: you can redistribute it and/or modify it under the terms
 *    of the GNU General Public License as published by the Free Software Foundation,
 *    either version 3 of the License, or (at your option) any later version.
 *    
 *    Corblivar is distributed in the hope that it will be useful, but WITHOUT ANY
 *    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License along with
 *    Corblivar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */
#ifndef _CORBLIVAR_THERMALANALYZER
#define _CORBLIVAR_THERMALANALYZER

// library includes
#include "Corblivar.incl.hpp"
// Corblivar includes, if any
#include "Chip.hpp"
// forward declarations, if any
class Block;
class Point;

class ThermalAnalyzer {
	// debugging code switch (private)
	private:
		static constexpr bool DBG_CALLS = false;
		static constexpr bool DBG = false;

	// private data, functions
	private:
		/// material parameters for thermal 3D-IC simulation using HotSpot
		/// Note: properties for heat spread and heat sink also from [Park09] (equal default
		/// HotSpot configuration values)
		// [Park09]; derived from 700 J/(kg*K) to J/(m^3*K) considering Si density of 2330 kg/m^3
		static constexpr double HEAT_CAPACITY_SI = 1.631e06;
		// [Park09]
		static constexpr double THERMAL_RESISTIVITY_SI = 8.510638298e-03;
		// [Sridhar10]; derived considering a factor of appr. 1.35 for Si/BEOL heat capacity
		static constexpr double HEAT_CAPACITY_BEOL = HEAT_CAPACITY_SI / 1.35;
		// [Sridhar10]
		static constexpr double THERMAL_RESISTIVITY_BEOL = 0.4444;
		// [Park09]; BCB polymer
		static constexpr double HEAT_CAPACITY_BOND = 2.298537e06;
		// [Park09]; BCB polymer
		static constexpr double THERMAL_RESISTIVITY_BOND = 5.0;
		// [Park09]
		static constexpr double HEAT_CAPACITY_CU = 3.546401e06;
		// [Park09]
		static constexpr double THERMAL_RESISTIVITY_CU = 2.53164557e-03;
		// [Park09]
		static constexpr double DENSITY_SI = 2330.0;
		// [Park09]
		static constexpr double DENSITY_BOND = 1051.0;
		// [Park09]
		static constexpr double DENSITY_CU = 8933.0;

		// TSV-groups-related parameters; derived from values above and
		// considering TSV dimensions
		//
		// heat capacity of TSV-group-Si compound:  C_group = m_Cu / m_group * C_Cu + (1 - m_Cu / m_group) * C_Si =
		// l * [1 / (1 + (p_Si * A_Si) / (p_Cu * A_Cu)) * C_Cu + 1 / (1 + (p_Cu * A_Cu) / (p_Si * A_Si)) * C_si]
		// http://answers.yahoo.com/question/index?qid=20110804231514AApjFkc ,
		// http://www.google.de/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&ved=0CCwQFjAA&url=http%3A%2F%2Fpubs.acs.org%2Fdoi%2Fabs%2F10.1021%2Fma902122u&ei=6JXuUfD-K9GKswaPzIGgDw&usg=AFQjCNFX7TTz6SQ_ZlLkt5nwGcLh-abdzQ&sig2=Jd7U_ZTSDs_7KYWTmXaA7g
		static constexpr double HEAT_CAPACITY_TSV_GROUP_IN_SI_PASSIVE = Chip::THICKNESS_SI_PASSIVE * (
				HEAT_CAPACITY_CU / (1.0 + ( (DENSITY_SI / DENSITY_CU) / Chip::TSV_GROUP_CU_SI_AREA_RATIO) ) +
				HEAT_CAPACITY_SI / (1.0 + ( (DENSITY_CU / DENSITY_SI) * Chip::TSV_GROUP_CU_SI_AREA_RATIO) )
				);
		//
		// similar for Bond scenario; TSV group's area-ratio for Cu-Si applies to Cu-Bond as well
		static constexpr double HEAT_CAPACITY_TSV_GROUP_IN_BOND = Chip::THICKNESS_BOND * (
				HEAT_CAPACITY_CU / (1.0 + ( (DENSITY_BOND / DENSITY_CU) / Chip::TSV_GROUP_CU_SI_AREA_RATIO) ) +
				HEAT_CAPACITY_BOND / (1.0 + ( (DENSITY_CU / DENSITY_BOND) * Chip::TSV_GROUP_CU_SI_AREA_RATIO) )
				);
		//
		// thermal resistivity of compounds, derived from thermal conductivity
		// which can be calculated via Maxwell-Garnett's Equation
		// (http://en.wikipedia.org/wiki/Effective_medium_approximations#Maxwell-Garnett_Equation)
		static constexpr double THERMAL_RESISTIVITY_TSV_GROUP_IN_SI = 1.0 / (
			( (1.0 / THERMAL_RESISTIVITY_SI) * (2.0 + Chip::TSV_GROUP_CU_SI_AREA_RATIO) +
			  (1.0 / THERMAL_RESISTIVITY_CU) * (1.0 - Chip::TSV_GROUP_CU_SI_AREA_RATIO) ) /
			( THERMAL_RESISTIVITY_SI * (
						    (1.0 / THERMAL_RESISTIVITY_CU) * (1.0 + 2 * Chip::TSV_GROUP_CU_SI_AREA_RATIO) -
						    (1.0 / THERMAL_RESISTIVITY_SI) * (2 * Chip::TSV_GROUP_CU_SI_AREA_RATIO - 2.0)
						   ) )
			);
		static constexpr double THERMAL_RESISTIVITY_TSV_GROUP_IN_BOND = 1.0 / (
			( (1.0 / THERMAL_RESISTIVITY_BOND) * (2.0 + Chip::TSV_GROUP_CU_SI_AREA_RATIO) +
			  (1.0 / THERMAL_RESISTIVITY_CU) * (1.0 - Chip::TSV_GROUP_CU_SI_AREA_RATIO) ) /
			( THERMAL_RESISTIVITY_BOND * (
						    (1.0 / THERMAL_RESISTIVITY_CU) * (1.0 + 2 * Chip::TSV_GROUP_CU_SI_AREA_RATIO) -
						    (1.0 / THERMAL_RESISTIVITY_BOND) * (2 * Chip::TSV_GROUP_CU_SI_AREA_RATIO - 2.0)
						   ) )
			);

		// thermal modeling: dimensions
		// represents the thermal map's dimension
		static constexpr int THERMAL_MAP_DIM = 64;
		// represents the thermal mask's dimension (i.e., the 2D gauss function
		// representing the thermal impulse response);
		// note that value should be uneven!
		static constexpr int THERMAL_MASK_DIM = 25;
		// represents the center index of the center originated mask
		static constexpr int THERMAL_MASK_CENTER = THERMAL_MASK_DIM / 2;
		// represents the amount of padded bins at power maps' boundaries
		static constexpr int POWER_MAPS_PADDED_BINS = THERMAL_MASK_CENTER;
		// represents the power maps' dimension
		// (note that maps are padded at the boundaries according to mask
		// dim in order to handle boundary values for convolution)
		static constexpr int POWER_MAPS_DIM = THERMAL_MAP_DIM + (THERMAL_MASK_DIM - 1);

		// thermal modeling: vector masks and maps
		// mask[i][x/y], whereas mask[0] relates to the mask for layer 0 obtained
		// by considering heat source in layer 0, mask[1] relates to the mask for
		// layer 0 obtained by considering heat source in layer 1 and so forth.
		// Note that the masks are only 1D for the separated convolution
		vector< array<double,THERMAL_MASK_DIM> > thermal_masks;
		// map[i][x][y], whereas map[0] relates to the map for layer 0 and so forth.
		vector< array<array<double,POWER_MAPS_DIM>,POWER_MAPS_DIM> > power_maps;
		// thermal map for layer 0 (lowest layer), i.e., hottest layer
		array<array<double,THERMAL_MAP_DIM>,THERMAL_MAP_DIM> thermal_map;

		// thermal modeling: parameters for generating power maps
		double power_maps_dim_x, power_maps_dim_y;
		double power_maps_bin_area;
		double blocks_offset_x, blocks_offset_y;
		double padding_right_boundary_blocks_distance, padding_upper_boundary_blocks_distance;
		array<double, POWER_MAPS_DIM + 1> power_maps_bins_ll_x, power_maps_bins_ll_y;
		static constexpr double PADDING_ZONE_BLOCKS_DISTANCE_LIMIT = 0.01;

	// constructors, destructors, if any non-implicit
	public:

	// public data, functions
	public:
		friend class IO;

		// POD
		struct MaskParameters {
			double mask_boundary_value;
			double impulse_factor;
			double impulse_factor_scaling_exponent;
		};

		// thermal modeling: handlers
		void initThermalMasks(int const& layers, bool const& log, MaskParameters const& parameters);
		void initPowerMaps(int const& layers, Point const& die_outline);
		void generatePowerMaps(int const& layers, vector<Block> const& blocks, Point const& die_outline, double const& power_density_scaling_padding_zone = 1.0, bool const& extend_boundary_blocks_into_padding_zone = true);
		// thermal-analyzer routine based on power blurring,
		// i.e., convolution of thermals masks and power maps;
		// also sets max cost with return-by-reference
		double performPowerBlurring(int const& layers, double const& temp_offset, double& max_cost_temp, bool const& set_max_cost = false, bool const& normalize = true, bool const& return_max_temp = false);
};

#endif
