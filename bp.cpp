/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <cmath>
#include <cstring>
#define TARGET_SIZE 30
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
    unsigned **state_chooser;  //pointer to state-chooser arrays or array(global case) of size: 2^history_size

    SIM_stats btb_stats; //aggregating statsitics
};

// we will act like in ADT init style
struct BTB_table *table_ptr; 

////zzzzz...zzzz yyyy 00 remeber direct mapping!!
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){

	//initiate parameters, check their validity
	table_ptr = new BTB_table;
	if(!table_ptr) return -1;

	table_ptr->rows = new BTB_row[btbSize];
	if(!(table_ptr->rows)) return -1;
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
		table_ptr->state_chooser = new unsigned*[btbSize];
		for(unsigned i=0; i<btbSize; i++ ){
			(table_ptr->state_chooser)[i] = new unsigned[(int)pow(2,historySize)];
			for(unsigned j=0; j< (1U << historySize); j++){
				table_ptr->state_chooser[i][j] = fsmState;
			} 
		}
	} else {//global
		table_ptr->state_chooser = new unsigned*[1];
		table_ptr->state_chooser[0] = new unsigned[(int)pow(2, historySize)];
		for(unsigned j=0; j< (1U << historySize); j++){
			table_ptr->state_chooser[0][j] = fsmState;
		} 
	}

	table_ptr->btb_stats = {0,0,0};
	return 0;
}

/*
 * helper function which use direc_mapping to retrieve some useful inidices, tag address and set history_reg
 * param[in] pc - the branch instruction address
 * param[out] BTB_index - the requested row in BTB table
 * param[out] hist_index - history_reg row index - for GlobalHist=0, fot LocalHist=BTB_index
 * param[out] state_index - which counter array to choose.
 * param[out] pc_tag - retrieve pc_tag.
 * param[out] history_mask - mask in size of history register
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
uint32_t direct_map(uint32_t pc, uint32_t *BTB_index,int *hist_index, int *state_index, uint32_t *pc_tag, uint32_t *history_mask){
	// from this function, we use: BTB_index, hist_index, state_index,  pc_tag
	uint32_t shift_pc = pc >> 2;  //00zzz...zz yyyy
	uint32_t index_mask =  table_ptr->btbSize - 1; //000..1111
	uint32_t tag_mask = pow(2,table_ptr->tagSize)-1; // 00..111111
	*history_mask = pow(2,table_ptr->historySize)-1;  
	*BTB_index = (shift_pc & index_mask); //000..yyyy
	uint32_t shift_tag = shift_pc >> int(log2(table_ptr->btbSize)); //00...zzzzz
	*pc_tag = shift_tag & tag_mask; // 00...zzzzz

	//index of hist and state - global or local
	*hist_index = 0;
	*state_index = 0; 
	if (!(table_ptr->isGlobalHist)){
		*hist_index = *BTB_index;
	}
	if(!(table_ptr->isGlobalTable)){
		*state_index = *BTB_index;
	}

	//shared
	uint32_t history_p = 0;
	history_p = (table_ptr->rows[*hist_index]).history_reg;
	if(table_ptr->Shared == 1){
		history_p = shift_pc ^ history_p;
	}
	if (table_ptr->Shared == 2){
		uint32_t shift_pc_16 = 0;
		shift_pc_16 = pc >> 16;
		history_p = shift_pc_16 ^ history_p;
	}
	history_p = history_p & *history_mask;
	return history_p;
}


bool BP_predict(uint32_t pc, uint32_t *dst){

	uint32_t BTB_index; int hist_index; int state_index; 
	uint32_t pc_tag; uint32_t history_mask;

	uint32_t history_p =  direct_map(pc, &BTB_index, &hist_index, &state_index, &pc_tag, &history_mask);	
	
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

	uint32_t BTB_index; int hist_index; int state_index; 
	uint32_t pc_tag; uint32_t history_mask;

	uint32_t history_p = direct_map(pc, &BTB_index, &hist_index, &state_index, &pc_tag, &history_mask);

	uint32_t old_hist_reg = history_p;

	(table_ptr->rows[BTB_index]).target = targetPc;
	//if tag not matches to any row, insert a new row
	if ((table_ptr->rows[BTB_index]).tag != pc_tag){
		//insert new row. In global_hist case, we use history_reg of first row to all rows in BTB
		// Namely, hist_index=0. Only tag and target are different in each row(=BTB_index).
		(table_ptr->rows[BTB_index]).tag = pc_tag;

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

		if(!(table_ptr->isGlobalTable)){
			for(unsigned j=0; j< (1U << table_ptr->historySize); j++){
				table_ptr->state_chooser[state_index][j] = table_ptr->fsmState;
			} 
		}
	} else {//we found a match with a tag
		(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg << 1;
		if(taken){
			(table_ptr->rows[hist_index]).history_reg +=1;
		}
		(table_ptr->rows[hist_index]).history_reg = (table_ptr->rows[hist_index]).history_reg & history_mask;
	}

	// update statistics and fsm state
	int old_state;
	int new_state;
	//if(table_ptr->isGlobalTable){
	 	old_state = table_ptr->state_chooser[state_index][old_hist_reg];
	//} 
	//else {
	//	old_state = table_ptr->fsmState;
	//}

	(table_ptr->btb_stats).br_num++;
	int flush = (table_ptr->btb_stats).flush_num;
	int branch = (table_ptr->btb_stats).br_num;

	//flush number
	if( (taken && (targetPc != pred_dst)) || ((!taken) && (pred_dst != pc+4)))
		(table_ptr->btb_stats).flush_num++;
		

	//taken case
	if(taken && ((old_state == SNT) || (old_state == WNT))){
		//(table_ptr->btb_stats).flush_num++;
		new_state = old_state + 1;
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}

	if(taken && ((old_state == ST) || (old_state == WT))){
		if(old_state == ST){//stuck in a loop
			new_state = old_state;
		} else {
			new_state = old_state + 1;
		}
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}

	// Not Taken case:
	if((!taken) && ((old_state == SNT) || (old_state == WNT))){
		if(old_state == SNT){//stuck in a loop
			new_state = old_state;
		}
		else{
			new_state = old_state - 1;
		}
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}

	if((!taken) && ((old_state == ST) || (old_state == WT))){
		//(table_ptr->btb_stats).flush_num++;
		new_state = old_state - 1;
		table_ptr->state_chooser[state_index][old_hist_reg] = new_state;
	}	

	return;
}

//we can do the destructor **inside BP_GetStats
void BP_GetStats(SIM_stats *curStats){
	curStats->flush_num = (table_ptr->btb_stats).flush_num;	
	curStats->br_num = (table_ptr->btb_stats).br_num;	
	unsigned size = 0;
	size = (table_ptr->btbSize)*(TARGET_SIZE + table_ptr->tagSize + 1);
	if(table_ptr->isGlobalHist){
		size += table_ptr->historySize;
	} else { //local history
		size += (table_ptr->historySize)*(table_ptr->btbSize);
	}

	if(table_ptr->isGlobalTable){
		size += pow(2,table_ptr->historySize + 1);
		delete table_ptr->state_chooser[0];
	} else { //local state chooser table
		size += (table_ptr->btbSize)*pow(2,table_ptr->historySize + 1);	
		//free tables 
		for(unsigned i=0; i<(table_ptr->btbSize); i++){
			delete[] (table_ptr->state_chooser)[i];
		}
	}

	curStats->size = size;

	delete[] table_ptr->state_chooser;
	delete[] table_ptr->rows;
	delete[] table_ptr;
	return;
}

