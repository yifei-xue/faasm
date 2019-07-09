#include "faasm/faasm.h"
#include "faasm/counter.h"
#include "faasm/matrix.h"
#include "faasm/sgd.h"
#include "faasm/func.h"

using namespace::faasm;

FAASM_MAIN_FUNC() {
    bool fullAsync = getEnvFullAsync();

    long inputSize = faasmGetInputSize();
    auto inputBuffer = new uint8_t[inputSize];
    faasmGetInput(inputBuffer, inputSize);

    const char *inputStr = reinterpret_cast<const char *>(inputBuffer);
    int *intArgs = faasm::splitStringIntoInts(inputStr, 4);

    int batchNumber = intArgs[0];
    int startIdx = intArgs[1];
    int endIdx = intArgs[2];
    int epoch = intArgs[3];

    printf("SGD step: %i %i %i %i\n", batchNumber, startIdx, endIdx, epoch);

    // Load params
    SgdParams sgdParams = readParamsFromState(PARAMS_KEY, fullAsync);

    // Perform updates
    if (sgdParams.lossType == HINGE) {
        printf("SGD hinge weight update\n");
        hingeLossWeightUpdate(sgdParams, epoch, batchNumber, startIdx, endIdx);
    } else {
        printf("SGD least squares weight update\n");
        leastSquaresWeightUpdate(sgdParams, batchNumber, startIdx, endIdx);
    }

    // Flag that this worker has finished
    printf("Writing finished flag\n");
    writeFinishedFlag(sgdParams, batchNumber);

    // If this is the last, dispatch the barrier (will have finished by now or will do soon)
    if (batchNumber == sgdParams.nBatches - 1) {
        faasmChainFunction("sgd_barrier");
    }

    delete[] intArgs;

    return 0;
}