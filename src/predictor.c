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
// const char *studentName = "Abhishek Mysore Harish";
// const char *studentID   = "A59020322";
// const char *email       = "amysoreharish@ucsd.edu";

// const char *studentName = "Adyanth Hosavalike";
// const char *studentID   = "A59017401";
// const char *email       = "ahosavalike@ucsd.edu";

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

// Gshare
uint8_t *gsharetable;   // Global predictor
uint64_t gsharebhr;     // History shift register
// Tournamemt
uint8_t *msharetable;   // Meta predictor
uint32_t *psharetable;  // Local predictor select
uint8_t *lsharetable;  // Local predictor

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case TOURNAMENT: {
      // Start with simple BHT
      int tablesize = 1 << pcIndexBits;
      msharetable = calloc(tablesize, sizeof(uint8_t));
      psharetable = calloc(tablesize, sizeof(uint32_t));
      lsharetable = calloc(1 << lhistoryBits, sizeof(uint8_t));
      // Initialize 
      for (int i = 0; i < tablesize; i++) {
        // Start with Local predictor (weak) (P2 - N)
        msharetable[i] = WN;
        // Initialize each predictor to 0
        psharetable[i] = 0;
      }
      for (int i = 0; i < (1 << lhistoryBits); i++) {
        lsharetable[i] = WN;
      }
      // Fallthrough since we need gselect for tournament
    }
    case GSHARE: {
      int tablesize = 1<<ghistoryBits; 
      gsharetable = calloc(tablesize, sizeof(uint8_t));

      // Initialize each predictor to WN
      for (int i = 0; i < tablesize; i++) {
        gsharetable[i] = WN;
      }
      // All history is not taken
      gsharebhr = 0;
      break;
    }
    default:
      break;
  }
}

uint8_t predict_nbit(uint64_t predict, int n) {
  return (predict >= (1 << (n-1))) ? TAKEN : NOTTAKEN;
}

uint8_t predict_2bit(uint8_t predict) {
  return predict_nbit(predict, 2);
}

void transition_nbit(uint8_t *state, uint8_t outcome, int n) {
  *state += outcome ? 1:-1;
  uint64_t st = ((1 << n)-1);
  if(*state > st) {
    *state = st;
  } else if (*state < SN) {
    *state = SN;
  }
}

void transition_2bit(uint8_t *state, uint8_t outcome) {
  return transition_nbit(state, outcome, 2);
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
    case TOURNAMENT: {
      uint32_t index = pc & ((1 << pcIndexBits) - 1);
      // If we are to use local predictor, prediction is N
      if(!predict_2bit(msharetable[index])) {
        return predict_2bit(lsharetable[psharetable[index]]);
      }
      // Else fallthrough for gshare
    }
    case GSHARE: {
      uint32_t index = (pc & ((1 << ghistoryBits) - 1)) ^ gsharebhr;
      uint8_t predict = gsharetable[index];
      return predict_2bit(predict);
    }
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
    case TOURNAMENT: {
      uint32_t index = pc & ((1 << pcIndexBits) - 1);
      // Transition meta predictor
      uint8_t p1 = predict_2bit(gsharetable[index]);
      uint8_t p2 = predict_2bit(lsharetable[psharetable[index]]);
      if(p1 != p2) {
        transition_2bit(&msharetable[index], p1 > p2);
      }
      // Transition local predictor
      transition_2bit(&lsharetable[psharetable[index]], outcome);
      psharetable[index] = ((psharetable[index] << 1) | outcome) & ((1 << lhistoryBits) - 1);
      // Fall through transition global predictor
    }
    case GSHARE: {
      uint32_t index = (pc & ((1 << ghistoryBits) - 1)) ^ gsharebhr;
      transition_2bit(&gsharetable[index], outcome);
      gsharebhr = ((gsharebhr << 1) | outcome) & ((1 << ghistoryBits) - 1);
      break;
    }
    case CUSTOM:
    default:
      break;
  }
}
