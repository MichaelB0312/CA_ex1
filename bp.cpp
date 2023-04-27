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
    int **state_chooser;  //pointer to state-chooser arrays or array(global case) of size: 2^history_size

    SIM_stats btb_stats; //aggregating statsitics
};

// we will act like in ADT init style
struct BTB_table *table_ptr; 


/*
 * helper function which use direc_mapping to retrieve some useful inidices, tag address and set history_reg
 * param[in] pc - the branch instruction address
 * param[out] BTB_index - the requested row in BTB table
 * param[out] hist_index - history_reg row index - for GlobalHist=0, fot LocalHist=BTB_index
 * param[out] state_index - which counter array to choose.
 * param[out] pc_tag - retrieve pc_tag.
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
void direct_map(uint32_t pc, uint32_t *BTB_index,int *hist_index, int *state_index, uint32_t *pc_tag){
///////// from this function, we use: BTB_index, hist_index, state_index,  pc_tag
	uint32_t shift_pc = pc >> 2;  //00zzz...zz yyyy
	uint32_t index_mask =  pow(2,table_ptr->btbSize)-1; //000..1111
	uint32_t tag_mask = pow(2,table_ptr->tagSize)-1; // 00..111111
	uint32_t history_mask = pow(2,table_ptr->historySize)-1;  
	*BTB_index = shift_pc & index_mask; //000..yyyy
	uint32_t shift_tag = shift_pc >> int(log2(table_ptr->btbSize)); //00...zzzzz
	*pc_tag = shift_tag & tag_mask; // 00...zzzzz

	//index of hist and state - global or local
	*hist_index = 0;
	*state_index = 0; //which counter_array to choose
	if (!(table_ptr->isGlobalHist)){
		*hist_index = *BTB_index;
	}
	if(!(table_ptr->isGlobalTable)){
		*state_index = *BTB_index;
	}

	//shared
	uint32_t history_p = 0;
	history_p = (table_ptr->rows[hist_index]).history_reg;
	if(table_ptr->Shared == 1){
		history_p = shift_pc ^ history_p;
	} else if (table_ptr->Shared == 2){
		uint32_t shift_pc_16 = 0;
		shift_pc_16 = pc >> 16;
		history_p = shift_pc_16 ^ history_p;
	}
	history_p = history_p & history_mask;
	(table_ptr->rows[hist_index]).history_reg = history_p;	

}

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
	unsigned flag_Size = 0;
	for(unsigned i=1; i<=32; i=i*2){
		if(btbSize == i){
			flag_Size = 1;
		}
	}
	if(!flag_Size) return -1;

	table_ptr->historySize = historySize;
	if(historySize < 1 || historySize > 8) return -1;

	table_ptr->tagSize = tagSize;
	if((tagSize < 0) || (tagSize > 30 - static_cast<unsigned>(log2(btbSize)) ) ) return -1;

	table_ptr->fsmState = fsmState;
	if( (fsmState < 0) || (fsmState > 3) ) return -1;

	table_ptr->isGlobalHist = isGlobalHist;
	table_ptr->isGlobalTable = isGlobalTable;
	table_ptr->Shared = Shared;

	//initiate state-chooser arrays
	if(!isGlobalTable){//local case

		//access: *((state_chooser + yyyy) + history_reg
		table_ptr->state_chooser = new int*[btbSize];
		for(unsigned i=0; i<btbSize; i++ ){
			(table_ptr->state_chooser)[i] = new int[(int)pow(2,historySize)];
			memset((table_ptr->state_chooser)[i], fsmState, (pow(2, historySize))*sizeof(int));
		}
	}
	else{//global
		table_ptr->state_chooser = new int*[1];
		*(table_ptr->state_chooser) = new int[(int)pow(2, historySize)-1];
		memset(*(table_ptr->state_chooser), fsmState, (pow(2, historySize))*sizeof(int));
	}

	table_ptr->btb_stats = {0,0,0};


	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){

	uint32_t BTB_index; uint32_t hist_index; uint32_t state_index; 
	uint32_t pc_tag;

	direct_map(pc, &BTB_index, &hist_index, &state_index, &pc_tag);	
	
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
	if ((state == WNT) || (state == SNT)){
		*dst = pc + 4;
		return false;
	} else if ((state == WT) || (state == ST)){
		*dst = table_ptr->rows[BTB_index].target;
		return true;
	}
	
	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){


	uint32_t BTB_index; uint32_t hist_index; uint32_t state_index; 
	uint32_t pc_tag;

	direct_map(pc, &BTB_index, &hist_index, &state_index, &pc_tag);

	uint32_t old_hist_reg = (table_ptr->rows[hist_index]).history_reg;

	//if tag not matches to any row, insert a new row
	if ((table_ptr->rows[BTB_index]).tag != pc_tag){
		//insert new row. In global_hist case, we use history_reg of first row to all rows in BTB
		// Namely, hist_index=0. Only tag and target are different in each row(=BTB_index).
		(table_ptr->rows[BTB_index]).tag = pc_tag;
		(table_ptr->rows[BTB_index]).target = targetPc;

		if(table_ptr->isGlobalHist){
			(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg << 1;
		}else{
			(table_ptr->rows[hist_index]).history_reg = 0;
			old_hist_reg = 0;
		}
		if(taken){
			(table_ptr->rows[hist_index]).history_reg +=1;
		}

		(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg & history_mask;

		if(isGlobalTable){
			memset((table_ptr->state_chooser)[state_index], table_ptr->fsmState, (pow(2, historySize))*sizeof(int));
		}
	}

	else{//we found a match with a tag

		(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg << 1;
		if(taken){
			(table_ptr->rows[hist_index]).history_reg +=1;
		}

		(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg & history_mask;

	}


	// upadte statitics and fsm state
	int old_state;
	if(table_ptr->isGlobalTable){

	 	old_state = table_ptr->state_chooser[state_index][old_hist_reg];
	}
	else{
		old_state = table_ptr->fsmState;
	}

	(table_ptr->btb_stats).br_num++;

	//taken case
	if(taken && ((old_state == SNT) || (old_state == WNT))){
		(table_ptr->btb_stats).flush_num++;
		int new_state;
		new_state = old_state + 1;
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
		(table_ptr->rows[hist_index]).target = targetPc; //for matched tag case
	}

	if(taken && ((old_state == ST) || (old_state == WT))){
		int new_state;
		if(old_state == ST){//stuck in a loop
			new_state = old_state;
		}
		else{
			new_state = old_state + 1;
		}
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}

	// Not Taken case:
	if((!taken) && ((old_state == SNT) || (old_state == WNT))){
		int new_state;
		if(old_state == SNT){//stuck in a loop
			new_state = old_state;
		}
		else{
			new_state = old_state - 1;
		}
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}

	if((!taken) && ((old_state == ST) || (old_state == WT))){
		(table_ptr->btb_stats).flush_num++;
		int new_state;
		new_state = old_state - 1;
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
		(table_ptr->rows[hist_index]).target = targetPc; //for matched tag case
	}	

	return;
}

//we can do the destructor **inside BP_GetStats
void BP_GetStats(SIM_stats *curStats){
	return;
}

