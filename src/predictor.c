//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

uint8_t *gsharetable;
uint64_t gsharebhr;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  gsharetable = calloc(1<<pcIndexBits, sizeof(uint8_t));
  gsharebhr = 0;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE: {
      uint32_t index = (pc & ((1 << pcIndexBits) - 1)) ^ gsharebhr;
      uint8_t predict = gsharetable[index];
      return predict ? TAKEN : NOTTAKEN;
    }
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  // Train based on the bpType
  switch (bpType) {
    case STATIC:
      break;
    case GSHARE: {
      uint32_t index = (pc & ((1 << pcIndexBits) - 1)) ^ gsharebhr;
      transition_predictor(&gsharetable[index], outcome);
      gsharebhr = (gsharebhr << 1) & outcome;
      break;
    }
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }
}

void transition_predictor(uint8_t *state, uint8_t outcome) {
  switch(*state) {
    case SN:
      *state = outcome == TAKEN ? WT : SN;
      break;
    case WN:
      *state = outcome == TAKEN ? ST : SN;
      break;
    case WT:
      *state = outcome == TAKEN ? ST : WN;
      break;
    case ST:
      *state = outcome == TAKEN ? ST : WT;
      break;
    default:
      break;
  }
}