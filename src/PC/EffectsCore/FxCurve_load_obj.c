#include "common_types.h"
#include "imports.h"

extern void *Hunk_AllocAlignInternal(int size, int align);

const FxCurve *FxCurve_AllocAndCreateWithKeys(const float *keyArray, int dimensionCount, int keyCount)
{
    int keySize = dimensionCount + 1;
    int needFirstKey = (keyArray[0] != 0.0f) ? 1 : 0;
    int needLastKey = (keyArray[(keyCount - 1) * keySize] != 1.0f) ? 1 : 0;
    int totalKeys = keyCount + needFirstKey + needLastKey;

    FxCurve *newCurve = (FxCurve *)Hunk_AllocAlignInternal(8 + totalKeys * keySize * 4, 4);
    newCurve->dimensionCount = dimensionCount;

    float *curveKeys = (float *)((char *)newCurve + 8);
    int keyIndex;

    if (needFirstKey) {

        curveKeys[0] = 0.0f;
        int d;
        for (d = 0; d < dimensionCount; d++) {
            curveKeys[1 + d] = keyArray[1 + d];
        }
        keyIndex = 1;
    } else {
        keyIndex = 0;
    }

    memcpy(&curveKeys[keyIndex * keySize], keyArray, keyCount * keySize * 4);
    keyIndex += keyCount;

    if (needLastKey) {

        curveKeys[keyIndex * keySize] = 1.0f;
        int d;
        for (d = 0; d < dimensionCount; d++) {
            curveKeys[keyIndex * keySize + 1 + d] = keyArray[(keyCount - 1) * keySize + 1 + d];
        }
    } else {

    }

    newCurve->keyCount = totalKeys;

    return newCurve;
}
