/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <cmath>
#include <cstring>

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

    BTB_row* rows;
    int **state_chooser;   //pointer to state-chooser arrays or array(global case) of size: 2^history_size
};

// we will act like in ADT init style
struct BTB_table *table_ptr; 

////zzzzz...zzzz yyyy 00 remeber direct mapping!!
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

	//initiate parameters, check their validity
	table_ptr = new BTB_table;
	if(!table_ptr) return -1;

	table_ptr -> rows = new BTB_row[btbSize];
	if(!(table_ptr -> rows)) return -1;
	memset(table_ptr -> rows, 0, sizeof(BTB_row) * btbSize);

	table_ptr->btbSize = btbSize;
	int flag_Size = 0;
	for(int i=1; i<=32; i=i*2){
		if(btbSize == i){
			flag_Size = 1;
		}
	}
	if(!flag_Size) return -1;

	table_ptr->historySize = historySize;
	if(historySize < 1 || historySize > 8) return -1;

	table_ptr->tagSize = tagSize;
	if((tagSize < 0) || (tagSize > 30 - static_cast<int>(log2(btbSize)) ) ) return -1;

	table_ptr->fsmState = fsmState;
	if( (fsmState < 0) || (fsmState > 3) ) return -1;

	table_ptr->isGlobalHist = isGlobalHist;
	table_ptr->isGlobalTable = isGlobalTable;
	table_ptr->Shared = Shared;

	//initiate state-chooser arrays
	if(!isGlobalTable){//local case

		//access: *((state_chooser + yyyy) + history_reg
		table_ptr->state_chooser = new int*[btbSize];
		for(int i=0; i<btbSize; i++ ){
			(table_ptr->state_chooser)[i] = new int[pow(2,historySize)];
			memset((table_ptr->state_chooser)[i], fsmState, (pow(2, historySize))*sizeof(int));
		}
	}
	else{//global
		table_ptr->state_chooser = new int*[1];
		*(table_ptr->state_chooser) = new int[pow(2, historySize)-1];
		memset(*(table_ptr->state_chooser), fsmState, (pow(2, historySize))*sizeof(int));
	}


	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	uint32_t shift_pc = pc >> 2;
	uint32_t index_mask =  pow(2,table_ptr->btbSize)-1;
	uint32_t tag_mask = pow(2,table_ptr->tagSize)-1;
	uint32_t history_mask = pow(2,table_ptr->historySize)-1;
	uint32_t BTB_index = shift_pc & index_mask;
	uint32_t shift_tag = shift_pc >> int(log2(table_ptr->btbSize));
	uint32_t pc_tag = shift_tag & tag_mask;
	//index of hist and state - global or local
	int hist_index = 0;
	int state_index = 0;
	if (!(table_ptr->isGlobalHist)){
		hist_index = BTB_index;
	}
	if(!(table_ptr->isGlobalTable)){
		state_index = BTB_index;
	}
	
	//shared
	uint32_t history_p = 0;
	history_p = *(table_ptr->state_chooser[hist_index]);
	if(table_ptr->Shared == 1){
		history_p = shift_pc ^ history_p;
	} else if (table_ptr->Shared == 2){
		uint32_t shift_pc_16 = 0;
		shift_pc_16 = pc >> 16;
		history_p = shift_pc_16 ^ history_p;
	}
	history_p = history_p & history_mask;
	
	//check if target is known
	if ((table_ptr->rows[BTB_index]).target == 0){
		*dst = pc + 4;
		return false;
	}
	//check if tag matches
	if ((table_ptr->rows[BTB_index]).tag != pc_tag){
		*dst = pc + 4;
		return false;
	}

	//tag matches, jump according to state	
	int state = table_ptr->state_chooser[state_index][history_p];
	if ((state == WNT) | (state == SNT)){
		*dst = pc + 4;
		return false;
	} else if ((state == WT) | (state == ST)){
		*dst = table_ptr->rows[BTB_index].target;
		return true;
	}
	
	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

//we can do the destructor **inside BP_GetStats
void BP_GetStats(SIM_stats *curStats){
	return;
}

