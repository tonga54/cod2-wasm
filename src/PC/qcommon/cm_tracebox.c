#include "common_types.h"
#include "imports.h"

void CM_CalcTraceEntents(TraceExtents *extents)
{
    int i;
    float delta;

    for (i = 0; i < 3; i++) {
        delta = extents->start[i] - extents->end[i];
        if (delta == 0.0f)
            extents->invDelta[i] = 0.0f;
        else
            extents->invDelta[i] = 1.0f / delta;
    }
}

qboolean CM_TraceBox(const TraceExtents *extents, const vec_t *mins, const vec_t *maxs, float fraction)
{
    float enter = 0.0f;
    float leave = fraction;
    int i;

    /* Return true only if the segment misses the box before its current hit. */
    for (i = 0; i < 3; i++) {
        float nearTime, farTime;
        if (extents->start[i] == extents->end[i]) {
            if (extents->start[i] < mins[i] || extents->start[i] > maxs[i])
                return 1;
            continue;
        }
        /* invDelta is 1 / (start - end), as computed above. */
        nearTime = (extents->start[i] - mins[i]) * extents->invDelta[i];
        farTime = (extents->start[i] - maxs[i]) * extents->invDelta[i];
        if (nearTime > farTime) {
            float swap = nearTime;
            nearTime = farTime;
            farTime = swap;
        }
        if (nearTime > enter) enter = nearTime;
        if (farTime < leave) leave = farTime;
        if (enter > leave)
            return 1;
    }
    return 0;
}
