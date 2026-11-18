#include "LUTs.h"

#define NUM_INPUTS  16
#define NUM_MODULES 4
#define NUM_OUTPUTS 5   /* [next state, is mistake, module defused, accelerometer value, load cell value] */

enum { OUT_NEXT_STATE, OUT_MISTAKE, OUT_DEFUSED, OUT_ACCEL, OUT_LOAD };

struct intArr_dict {
    int *key;
    size_t key_len;
    int *intArr;
    UT_hash_handle hh;
};

//map is passed by address because HASH_ADD changes the head pointer
void add_intArr(struct intArr_dict **map, const int *key, size_t key_len, const int *intArr) {
    size_t key_bytes = key_len * sizeof(int);

    struct intArr_dict *entry = malloc(sizeof(*entry));
    if (entry == NULL) return;
    entry->key = malloc(key_bytes);
    entry->intArr = malloc(NUM_OUTPUTS * sizeof(int));
    if (entry->key == NULL || entry->intArr == NULL) {
        free(entry->key);
        free(entry->intArr);
        free(entry);
        return;
    }

    memcpy(entry->key, key, key_bytes);
    memcpy(entry->intArr, intArr, NUM_OUTPUTS * sizeof(int));
    entry->key_len = key_len;

    HASH_ADD_KEYPTR(hh, *map, entry->key, key_bytes, entry);
}

int *find_intArr(struct intArr_dict *map, const int *key, size_t key_len) {
    struct intArr_dict *entry = NULL;
    HASH_FIND(hh, map, key, key_len * sizeof(int), entry);
    return entry ? entry->intArr : NULL;
}


volatile int numMistakes = 0;     //placeholder
volatile int accelerometerValue;  //placeholder
volatile int loadcellWeight;      //placeholder


#define NUM_STATES_MODULE_1 4
#define NUM_STATES_MODULE_2 4
#define NUM_STATES_MODULE_3 4
#define NUM_STATES_MODULE_4 4

//You could use an dict of intArr_dict instead of arrays, but at the scale we are using, there is really no advantage
struct intArr_dict *module1States[NUM_STATES_MODULE_1];
struct intArr_dict *module2States[NUM_STATES_MODULE_2];
struct intArr_dict *module3States[NUM_STATES_MODULE_3];
struct intArr_dict *module4States[NUM_STATES_MODULE_4];
struct intArr_dict **moduleStates[NUM_MODULES] = {module1States, module2States, module3States, module4States};
const int numStatesPerModule[NUM_MODULES] = {NUM_STATES_MODULE_1, NUM_STATES_MODULE_2,
                                             NUM_STATES_MODULE_3, NUM_STATES_MODULE_4};

volatile struct intArr_dict *modules[NUM_MODULES]; //current state of each module

volatile int defusedModules[NUM_MODULES] = {0, 0, 0, 0}; //modules that have been defused, starts at 0

volatile int curInputs[NUM_INPUTS] = {0}; //keeps track of the states of inputs, all start at 0


const int inputsPerModule[NUM_MODULES] = {4, 0, 0, 0};

const int module1Inputs[] = {1, 2, 3, 4};
// const int module2Inputs[] = {0, 0, 0, 0};
// const int module3Inputs[] = {0, 0, 0, 0};
// const int module4Inputs[] = {0, 0, 0, 0};

int inputs[NUM_INPUTS] = {0}; //which module (1-based) each input belongs to


void init_module1(void);

void init_moduleLogic(void) {
    init_module1();
    //init_module2();
    //init_module3();
    //init_module4();
}

void checkInput(int inputNum, int inputVal) {
    if (inputNum < 1 || inputNum > NUM_INPUTS) return;
    curInputs[inputNum - 1] = inputVal; //first, update inputs with the new input

    int curModule = inputs[inputNum - 1]; //then, find what module was used
    if (defusedModules[curModule - 1]) return; //if the module has been defused, skip
    size_t numInputs = inputsPerModule[curModule - 1]; //then, find how many inputs that module has

    //then, collect the inputs of that module (in ascending input-number order)
    int moduleInputs[NUM_INPUTS];
    size_t currentIndex = 0;
    for (int i = 0; i < NUM_INPUTS && currentIndex < numInputs; i++) {
        if (inputs[i] == curModule) {
            moduleInputs[currentIndex] = curInputs[i];
            currentIndex++;
        }
    }

    struct intArr_dict *curModuleState = modules[curModule - 1]; //then, find the current state of the module
    int *results = find_intArr(curModuleState, moduleInputs, numInputs); //give that state the inputs and see the outputs
    if (results == NULL) { //if no defined state exists, use the default state
        results = find_intArr(curModuleState, (int[]){-1}, 1);
    }
    if (results == NULL) { //no default defined either: treat as a mistake
        ++numMistakes;
        return;
    }

    modules[curModule - 1] = moduleStates[curModule - 1][results[OUT_NEXT_STATE] - 1];
    if (results[OUT_MISTAKE]) ++numMistakes;
    if (results[OUT_DEFUSED]) defusedModules[curModule - 1] = 1;
    if (results[OUT_ACCEL] != -1) accelerometerValue = results[OUT_ACCEL];
    if (results[OUT_LOAD] != -1) loadcellWeight = results[OUT_LOAD];
}


void init_module1(void) {
    for (int i = 0; i < inputsPerModule[0]; i++) {
        inputs[module1Inputs[i] - 1] = 1;
    }

    //Module state outputs: [next state, is mistake, module defused, accelerometer value (-1 to skip), load cell value (-1 to skip)]
    //An input that uses a module's input but does not match its states map is automatically a mistake
    //The default (base case) key is {-1} with a length of 1
    struct intArr_dict *module1State1 = NULL; //starting state
    struct intArr_dict *module1State2 = NULL; //state2
    struct intArr_dict *module1State3 = NULL; //state3
    struct intArr_dict *module1State4 = NULL; //state4

    add_intArr(&module1State1, (int[]){1, 0, 0, 0}, inputsPerModule[0], (int[]){2, 0, 0, -1, -1}); //press button 1 to advance
    add_intArr(&module1State2, (int[]){0, 1, 0, 0}, inputsPerModule[0], (int[]){3, 0, 0, -1, -1}); //press button 2 to advance
    add_intArr(&module1State3, (int[]){0, 0, 1, 0}, inputsPerModule[0], (int[]){4, 0, 0, -1, -1}); //press button 3 to advance
    add_intArr(&module1State4, (int[]){0, 0, 0, 1}, inputsPerModule[0], (int[]){4, 0, 1, -1, -1}); //press button 4 to defuse this module

    add_intArr(&module1State1, (int[]){0, 0, 0, 0}, inputsPerModule[0], (int[]){1, 0, 0, -1, -1}); //release: stay, no mistake
    add_intArr(&module1State2, (int[]){0, 0, 0, 0}, inputsPerModule[0], (int[]){2, 0, 0, -1, -1}); //release: stay, no mistake
    add_intArr(&module1State3, (int[]){0, 0, 0, 0}, inputsPerModule[0], (int[]){3, 0, 0, -1, -1}); //release: stay, no mistake
    add_intArr(&module1State4, (int[]){0, 0, 0, 0}, inputsPerModule[0], (int[]){4, 0, 0, -1, -1}); //release: stay, no mistake

    add_intArr(&module1State1, (int[]){-1},         1,                  (int[]){1, 1, 0, -1, -1}); //base case at this state: is a mistake
    add_intArr(&module1State2, (int[]){-1},         1,                  (int[]){1, 1, 0, -1, -1}); //base case at this state: is a mistake, and resets
    add_intArr(&module1State3, (int[]){-1},         1,                  (int[]){1, 1, 0, -1, -1}); //base case at this state: is a mistake, and resets
    add_intArr(&module1State4, (int[]){-1},         1,                  (int[]){1, 1, 0, -1, -1}); //base case at this state: is a mistake, and resets



    module1States[0] = module1State1;
    module1States[1] = module1State2;
    module1States[2] = module1State3;
    module1States[3] = module1State4;

    modules[0] = module1State1; //add state 1 as the first state
}