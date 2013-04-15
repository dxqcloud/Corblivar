/*
 * =====================================================================================
 *
 *    Description:  Corblivar design net
 *
 *         Author:  Johann Knechtel, johann.knechtel@ifte.de
 *        Company:  Institute of Electromechanical and Electronic Design, www.ifte.de
 *
 * =====================================================================================
 */
#ifndef _CORBLIVAR_NET
#define _CORBLIVAR_NET

// library includes
#include "Corblivar.incl.hpp"
// Corblivar includes, if any
#include "Block.hpp"
// forward declarations, if any

class Net {
	// debugging code switch (private)
	private:

	// private data, functions
	private:

	// constructors, destructors, if any non-implicit
	public:
		Net(int const& id_i) {
			id = id_i;
			hasExternalPin = false;
		};

	// public data, functions
	public:
		int id;
		bool hasExternalPin;
		vector<Block const*> blocks;
		int layer_bottom, layer_top;

		inline void setLayerBoundaries(int const& globalUpperLayer) {
			this->layer_bottom = globalUpperLayer;
			this->layer_top = 0;

			if (this->blocks.empty()) {
				return;
			}
			else {
				for (Block const* b : this->blocks) {
					this->layer_bottom = min(this->layer_bottom, b->layer);
					this->layer_top = max(this->layer_top, b->layer);

				}
			}
		};
};

#endif