/*
 * =====================================================================================
 *
 *    Description:  Corblivar core file (data structures, layout operations)
 *
 *         Author:  Johann Knechtel, johann.knechtel@ifte.de
 *        Company:  Institute of Electromechanical and Electronic Design, www.ifte.de
 *
 * =====================================================================================
 */
#include "Corblivar.hpp"

void CorblivarLayoutRep::initCorblivar(CorblivarFP &corb) {
	map<int, Block*>::iterator b;
	Block *cur_block;
	Direction cur_dir;
	int i, rand, cur_t;

	if (corb.logMed()) {
		cout << "Layout> ";
		cout << "Initializing Corblivar data for chip on " << corb.conf_layer << " layers..." << endl;
	}

	// init separate data structures for dies
	this->dies.clear();
	for (i = 0; i < corb.conf_layer; i++) {
		this->dies.push_back(new CorblivarDie(i));
	}

	// assign each block randomly to one die, generate L and T randomly as well
	for (b = corb.blocks.begin(); b != corb.blocks.end(); ++b) {
		cur_block = (*b).second;

		// consider random die
		rand = Math::randI(0, corb.conf_layer);

		// generate direction L
		if (Math::randB()) {
			cur_dir = DIRECTION_HOR;
		}
		else {
			cur_dir = DIRECTION_VERT;
		}
		// init T-junction to be overlapped as zero, results in initial layout to
		// be placed ``somewhat diagonally'' into outline
		cur_t = 0;

		// store into separate CBL sequences
		this->dies[rand]->CBL.S.push_back(cur_block);
		this->dies[rand]->CBL.L.push_back(cur_dir);
		this->dies[rand]->CBL.T.push_back(cur_t);
	}

#ifdef DBG_CORB
	unsigned d, CBLi;

	for (d = 0; d < this->dies.size(); d++) {
		cout << "DBG_CORB> ";
		cout << "Init CBL tuples for die " << d << "; " << this->dies[d]->CBL.size() << " tuples:" << endl;
		cout << this->dies[d]->CBL.itemString() << endl;
		cout << "DBG_CORB> ";
		cout << endl;
	}
#endif

	if (corb.logMed()) {
		cout << "Layout> ";
		cout << "Done" << endl << endl;
	}
}

void CorblivarLayoutRep::generateLayout(int log) {
	unsigned i;
	Block *cur_block;
	bool loop;

#ifdef DBG_CORB
	cout << "DBG_CORB> ";
	cout << "Performing layout generation..." << endl;
#endif

	// init die pointer
	this->p = this->dies[0];

	// reset die data
	for (i = 0; i < this->dies.size(); i++) {
		// reset progress pointer
		this->dies[i]->resetTuplePointer();
		// reset done flag
		this->dies[i]->done = false;
		// reset placement stacks
		while (!this->dies[i]->Hi.empty()) {
			this->dies[i]->Hi.pop();
		}
		while (!this->dies[i]->Vi.empty()) {
			this->dies[i]->Vi.pop();
		}
	}

	// perform layout generation in loop (until all blocks are placed)
	loop = true;
	while (loop) {
		// handle stalled die / resolve open alignment process by placing current block
		if (this->p->stalled) {
			// place block, increment progress pointer
			cur_block = this->p->placeCurrentBlock();
			// TODO mark current block as placed in AS
			//
			// mark die as not stalled anymore
			this->p->stalled = false;
		}
		// die is not stalled
		else {
			// TODO check for alignment tuples for current block
			//
			if (false) {
			}
			// no alignment tuple assigned for current block
			else {
				// place block, increment progress pointer
				this->p->placeCurrentBlock();
			}
		}

		// die done
		if (this->p->done) {
			// continue loop on yet unfinished die
			for (i = 0; i < this->dies.size(); i++) {
				if (!this->dies[i]->done) {
					this->p = this->dies[i];
					break;
				}
			}
			// all dies handled, stop loop
			if (this->p->done) {
				loop = false;
			}
		}
	}

#ifdef DBG_CORB
	cout << "DBG_CORB> ";
	cout << "Done" << endl;
#endif

}

Block* CorblivarDie::placeCurrentBlock() {
	Block *cur_block;
	Direction cur_dir;
	unsigned cur_juncts;
	vector<Block*> relevBlocks;
	unsigned relevBlocksCount, b;
	double x, y;
	bool add_to_stack;
	list<Block*> blocks_add_to_stack;
	list<Block*>::iterator ba;

	// sanity check for empty dies
	if (this->CBL.empty()) {
		this->done = true;
		return NULL;
	}

	cur_block = this->currentBlock();
	cur_dir = this->currentTupleDirection();
	cur_juncts = this->currentTupleJuncts();

	// assign layer to block
	cur_block->layer = this->id;

	// horizontal placement
	if (cur_dir == DIRECTION_HOR) {
		// pop relevant blocks from stack
		relevBlocksCount = min(cur_juncts + 1, this->Hi.size());
		while (relevBlocksCount > relevBlocks.size()) {
			relevBlocks.push_back(this->Hi.top());
			this->Hi.pop();
		}

		// determine y-coordinate for lower left corner of current block
		if (this->Hi.empty()) {
			y = 0;
		}
		else {
			// determine min value
			y = relevBlocks[0]->bb.ll.y;
			for (b = 1; b < relevBlocks.size(); b++) {
				if (relevBlocks[b]->bb.ll.y != Point::UNDEF) {
					y = min(y, relevBlocks[b]->bb.ll.y);
				}
			}
		}

		// update block's y-coordinates
		cur_block->bb.ll.y = y;
		cur_block->bb.ur.y = cur_block->bb.h + y;

		// determine x-coordinate for lower left corner of current block
		x = 0;
		for (b = 0; b < relevBlocks.size(); b++) {
			// only consider blocks which intersect in y-direction
			if (Rect::rectsIntersectVertical(cur_block->bb, relevBlocks[b]->bb)) {
				// determine right front
				x = max(x, relevBlocks[b]->bb.ur.x);
			}
		}

		// update block's x-coordinates
		cur_block->bb.ll.x = x;
		cur_block->bb.ur.x = cur_block->bb.w + x;

		// update vertical stack; add cur_block when no other relevant blocks
		// are to its top side, indepent of overlap in x-direction
		add_to_stack = true;
		for (b = 0; b < relevBlocks.size(); b++) {
			if (Rect::rectA_below_rectB(cur_block->bb, relevBlocks[b]->bb, false)) {
				add_to_stack = false;
			}
		}

		if (add_to_stack) {
			this->Vi.push(cur_block);
		}

		// update horizontal stack; add relevant blocks which have no block to the right,
		// can be simplified by checking against cur_block (only new block which
		// can be possibly right of others)
		for (b = 0; b < relevBlocks.size(); b++) {
			if (!Rect::rectA_leftOf_rectB(relevBlocks[b]->bb, cur_block->bb, true)) {
				// prepending blocks to list retains the (implicit)
				// ordering of blocks popped from stack Hi regarding their
				// insertion order; required for proper stack manipulation
				blocks_add_to_stack.push_front(relevBlocks[b]);
			}
		}
		// also consider new block cur_block as right of others
		blocks_add_to_stack.push_front(cur_block);

		for (ba = blocks_add_to_stack.begin(); ba != blocks_add_to_stack.end(); ++ba) {
			this->Hi.push(*ba);
		}
	}
	// vertical placement
	else {
		// pop relevant blocks from stack
		relevBlocksCount = min(cur_juncts + 1, this->Vi.size());
		while (relevBlocksCount > relevBlocks.size()) {
			relevBlocks.push_back(this->Vi.top());
			this->Vi.pop();
		}

		// determine x-coordinate for lower left corner of current block
		if (this->Vi.empty()) {
			x = 0;
		}
		else {
			// determine min value
			x = relevBlocks[0]->bb.ll.x;
			for (b = 1; b < relevBlocks.size(); b++) {
				if (relevBlocks[b]->bb.ll.x != Point::UNDEF) {
					x = min(x, relevBlocks[b]->bb.ll.x);
				}
			}
		}

		// update block's x-coordinates
		cur_block->bb.ll.x = x;
		cur_block->bb.ur.x = cur_block->bb.w + x;

		// determine y-coordinate for lower left corner of current block
		y = 0;
		for (b = 0; b < relevBlocks.size(); b++) {
			// only consider blocks which intersect in x-direction
			if (Rect::rectsIntersectHorizontal(cur_block->bb, relevBlocks[b]->bb)) {
				// determine upper front
				y = max(y, relevBlocks[b]->bb.ur.y);
			}
		}

		// update block's y-coordinates
		cur_block->bb.ll.y = y;
		cur_block->bb.ur.y = cur_block->bb.h + y;

		// update horizontal stack; add cur_block when no other relevant blocks
		// are to its right side, indepent of overlap in y-direction
		add_to_stack = true;
		for (b = 0; b < relevBlocks.size(); b++) {
			if (Rect::rectA_leftOf_rectB(cur_block->bb, relevBlocks[b]->bb, false)) {
				add_to_stack = false;
			}
		}

		if (add_to_stack) {
			this->Hi.push(cur_block);
		}

		// update vertical stack; add relevant blocks which have no block above,
		// can be simplified by checking against cur_block (only new block which
		// can be possibly above others)
		for (b = 0; b < relevBlocks.size(); b++) {
			if (!Rect::rectA_below_rectB(relevBlocks[b]->bb, cur_block->bb, true)) {
				// prepending blocks to list retains the (implicit)
				// ordering of blocks popped from stack Vi regarding their
				// insertion order; required for proper stack manipulation
				blocks_add_to_stack.push_front(relevBlocks[b]);
			}
		}
		// also consider new block cur_block as above others
		blocks_add_to_stack.push_front(cur_block);

		for (ba = blocks_add_to_stack.begin(); ba != blocks_add_to_stack.end(); ++ba) {
			this->Vi.push(*ba);
		}
	}

#ifdef DBG_CORB
	cout << "DBG_CORB> ";
	cout << "Processed (placed) CBL tuple " << this->currentTupleString() << " on die " << this->id << ":" << endl;
	cout << " LL=(" << cur_block->bb.ll.x << ", " << cur_block->bb.ll.y << "), ";
	cout << " UR=(" << cur_block->bb.ur.x << ", " << cur_block->bb.ur.y << ")" << endl;

	stack<Block*> tmp_Hi = this->Hi;
	cout << "DBG_CORB> stack Hi:" << endl;
	cout << " ";
	while (!tmp_Hi.empty()) {
		if (tmp_Hi.size() > 1) {
			cout << tmp_Hi.top()->id << ", ";
		}
		else {
			cout << tmp_Hi.top()->id << endl;
		}
		tmp_Hi.pop();
	}

	stack<Block*> tmp_Vi = this->Vi;
	cout << "DBG_CORB> stack Vi:" << endl;
	cout << " ";
	while (!tmp_Vi.empty()) {
		if (tmp_Vi.size() > 1) {
			cout << tmp_Vi.top()->id << ", ";
		}
		else {
			cout << tmp_Vi.top()->id << endl;
		}
		tmp_Vi.pop();
	}
#endif

	// increment progress pointer, consider next tuple (block)
	this->incrementTuplePointer();

	return cur_block;
}
