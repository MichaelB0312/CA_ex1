/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

enum State{SNT,WNT,WT,ST};


BP::BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
        bool isGlobalHist, bool isGlobalTable, int Shared){

	//
	btbSize = btbSize;
	historySize = historySize;
	tagSize = tagSize;
	fsmState = fsmState;
	isGlobalHist = isGlobalHist;
	isGlobalTable = isGlobalTable;

	state_chooser = new int[pow(2,historySize)-1];  

	// initiate state_chooser to SNT
	for (int i=0 ; i < pow(2,historySize) ; i++){
		state_chooser[i] = fsmState;
	} 

	history_reg = 0;
}


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

