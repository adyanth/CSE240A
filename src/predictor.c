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
uint32_t *psharetable;   // Local history table
uint8_t *lsharetable;   // Local predictor
// Custom
int32_t **perceptrontable; // Perceptron table 
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
      int lsharetablesize = 1 << lhistoryBits;
      msharetable = calloc(tablesize, sizeof(uint8_t));
      psharetable = calloc(tablesize, sizeof(uint32_t));
      lsharetable = calloc(lsharetablesize, sizeof(uint8_t));
      // Initialize 
      for (int i = 0; i < tablesize; i++) {
        // Start with Local predictor (weak) (P2 - N)
        msharetable[i] = WN;
        // Initialize local history with NOT TAKEN
        psharetable[i] = NOTTAKEN;
      }
      for (int i = 0; i < lsharetablesize; i++) {
        // Initialize each predictor to WN
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
    case CUSTOM: {
      int tablesize = 1<<ghistoryBits;
      int perceptroncount = ghistoryBits + 1;
      perceptrontable = (int32_t **)calloc(tablesize, sizeof(int32_t *));
      for (int i = 0; i < tablesize; i++) {
        perceptrontable[i] = (int32_t *)calloc(perceptroncount, sizeof(int32_t));
        perceptrontable[i][0] = 1; // Bias weight
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

uint8_t predict_lbit(uint64_t predict) {
  return predict_nbit(predict, lhistoryBits);
}

void transition_nbit(uint8_t *state, uint8_t outcome, int n) {
  int16_t newSt = *state;
  newSt += outcome ? 1:-1;
  uint64_t st = ((1 << n)-1);
  if(newSt > (int16_t)st) {
    *state = st;
  } else if (newSt < SN) {
    *state = SN;
  } else {
    *state = newSt;
  }
}

void transition_2bit(uint8_t *state, uint8_t outcome) {
  return transition_nbit(state, outcome, 2);
}

void transition_lbit(uint8_t *state, uint8_t outcome) {
  return transition_nbit(state, outcome, lhistoryBits);
}

int predict_perceptron(int32_t *perceptron) {
  int prediction = perceptron[0];
  uint64_t globalhistory = gsharebhr;

  for (int i = 1; i <= ghistoryBits; i++) {
    prediction += perceptron[i]*((gsharebhr & 1)? 1: -1);
    globalhistory = globalhistory >> 1;
  }

  return prediction;
}

void transition_perceptron(int32_t *perceptron, uint8_t outcome) {
  uint64_t globalhistory = gsharebhr;
  for (int i = 1; i <= ghistoryBits; i++) {
    perceptron[i] = perceptron[i] + ((outcome == NOTTAKEN)? -1 : 1)*((globalhistory & TAKEN) ? 1 : -1);
    globalhistory = globalhistory >> 1;
  }
  return;
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
        int lsharetableindex = psharetable[index];
        return predict_2bit(lsharetable[lsharetableindex]);
      }
      // Else fallthrough for gshare
    }
    case GSHARE: {
      uint32_t index = (pc & ((1 << ghistoryBits) - 1)) ^ gsharebhr;
      uint8_t predict = gsharetable[index];
      return predict_2bit(predict);
    }
    case CUSTOM: {
      uint32_t index = (pc & ((1 << ghistoryBits) - 1));
      int pred = predict_perceptron(perceptrontable[index]);
      return (pred > 0)? TAKEN: NOTTAKEN;
    }
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
      uint32_t lsharetableindex = psharetable[index];
      // Transition meta predictor
      uint8_t p1 = predict_2bit(gsharetable[index]);
      uint8_t p2 = predict_2bit(lsharetable[lsharetableindex]);
      if(p1 != p2) {
        transition_2bit(&msharetable[index], p1 > p2);
      }
      // Transition local predictor
      transition_2bit(&lsharetable[lsharetableindex], outcome);
      // Transition locah history table
      psharetable[index] = ((psharetable[index] << 1) | outcome) & ((1<<lhistoryBits)-1);
      // Fall through transition global predictor
    }
    case GSHARE: {
      uint32_t index = (pc & ((1 << ghistoryBits) - 1)) ^ gsharebhr;
      transition_2bit(&gsharetable[index], outcome);
      gsharebhr = ((gsharebhr << 1) | outcome) & ((1 << ghistoryBits) - 1);
      break;
    }
    case CUSTOM:{
      uint32_t index = pc & ((1 << ghistoryBits) - 1);
      transition_perceptron(perceptrontable[index], outcome);
      gsharebhr = ((gsharebhr << 1) | outcome) & ((1 << ghistoryBits) - 1);
      break;
    }
    default:
      break;
  }
}
