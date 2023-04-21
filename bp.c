/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <cmath>

enum State{SNT,WNT,WT,ST};


typedef struct 
{
	unsigned int tag;  // we will do a mask and, based on tag_size, we will stay only the relevant bits.
	unsigned int target; // full 32 bits address
	unsigned int history_reg; //also will multiplied with mask, based on historysize, we will stay with the true register size
					//need mask's multiplication every update! because we shift left the bits every iteration
} BTB_row;

struct BTB_table{
    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned fsmState;
    bool isGlobalHist;
    bool isGlobalTable;
    int Shared;

    int *state_chooser;   //2^history_size
}



/*
class BP {

    unsigned btbSize;
    unsigned historySize;
    unsigned tagSize;
    unsigned fsmState;
    bool isGlobalHist;
    bool isGlobalTable;
    int Shared;
    int *state_chooser;   //2^history_size
    int history_reg; 


public:// we need constructor here and destructor and helper functions... but the required functions for the excersie..redundant
    BP(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
        bool isGlobalHist, bool isGlobalTable, int Shared){



    }

    //bool BP_predict(uint32_t pc, uint32_t *dst);

    //void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

    //void BP_GetStats(SIM_stats *curStats);

    ~ BP(){

    }

}



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
*/

// we will act like in ADT init style
struct BTB_table *table_ptr; 

////zzzzz...zzzz yyyy 00 remeber direct mapping!!
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){


	table_ptr = new BTB_row[btbSize];



	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

//we can do the destructor **inside BP_GetStats
void BP_GetStats(SIM_stats *curStats){
	return;
}

