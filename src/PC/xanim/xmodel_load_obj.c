#include "common_types.h"
#include "imports.h"

int XModelGetStaticBounds(const XModel *model, vec3_t *axis, vec_t *mins, vec_t *maxs);
XModel *XModelLoad(const char *name, Alloc_t Alloc, Alloc_t AllocColl);

extern void MatrixTransformVector(const float *in1, const float (*in2)[3], float *out);
extern void QuatMultiply(const float *in1, const float *in2, float *out);
extern XModelParts *XModelPartsFindData(const char *name);
extern void XModelPartsSetData(const char *name, XModelParts *modelParts, Alloc_t Alloc);
extern XModelSurfs *XModelSurfsFindData(const char *name);
extern void XModelSurfsSetData(const char *name, XModelSurfs *modelSurfs, Alloc_t Alloc);
extern XSurface *XModelReadSurface(XModel *model, int *partBits, const byte **pos, Alloc_t Alloc);
extern void XModelFree(XModel *model);
extern trXSkin_t *R_LoadXSkins(XModel *model);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern void Com_Printf(const char *fmt, ...);
extern unsigned int SL_GetString_(const char *str, unsigned int user, int type);
extern unsigned int SL_GetStringOfLen(const char *str, unsigned int user, unsigned int len, int type);

extern float floorf(float x);
extern float sqrtf(float x);
extern refexport_t re;

int XModelGetStaticBounds(const XModel *model, vec3_t *axis, vec_t *mins, vec_t *maxs)
{
    int i, k, j;
    XModelCollSurf *csurf;
    vec3_t corner;
    vec3_t rotated;

    if (!model->numCollSurfs) {
        return 0;
    }

    mins[0] = 3.4028234663852886e+38f;
    mins[1] = 3.4028234663852886e+38f;
    mins[2] = 3.4028234663852886e+38f;
    maxs[0] = -3.4028234663852886e+38f;
    maxs[1] = -3.4028234663852886e+38f;
    maxs[2] = -3.4028234663852886e+38f;

    if (model->numCollSurfs <= 0) {
        return 1;
    }

    for (i = 0; i < model->numCollSurfs; i++) {
        csurf = &model->collSurfs[i];

        for (k = 0; k < 8; k++) {

            if (k & 1) {
                corner[0] = csurf->mins[0];
            } else {
                corner[0] = csurf->maxs[0];
            }

            if (k & 2) {
                corner[1] = csurf->mins[1];
            } else {
                corner[1] = csurf->maxs[1];
            }

            if (k & 4) {
                corner[2] = csurf->mins[2];
            } else {
                corner[2] = csurf->maxs[2];
            }

            MatrixTransformVector(corner, axis, rotated);

            for (j = 1; j < 4; j++) {
                if (mins[j - 1] > rotated[j - 1]) {
                    mins[j - 1] = rotated[j - 1];
                }
                if (rotated[j - 1] > maxs[j - 1]) {
                    maxs[j - 1] = rotated[j - 1];
                }
            }
        }
    }

    return 1;
}

XModel *XModelLoad(const char *name, Alloc_t Alloc, Alloc_t AllocColl)
{
    XModelConfig config;
    int nameLens[4];
    char filename[64];
    char partsFilename[64];
    void *buf;
    void *partsBuf;
    int i, j;
    XModel *model;
    const byte *pos;
    const byte *bufEnd = NULL;
    int size;
    XModelParts *modelParts;
    char *lodFilename;
    int lodIndex;
    const char *modelName;
    short modelNumSurfs;

    union {
        short s;
        int i;
        float f;
    } u;

    if (Com_sprintf(filename, 64, "xmodel/%s", name) < 0) {
        Com_Printf("^1ERROR: filename '%s' too long\n", filename);
        model = NULL;
        goto done;
    }

    i = FS_ReadFile(filename, &buf);
    if (i < 0) {
        Com_Printf("^1ERROR: xmodel '%s' not found\n", name);
        model = NULL;
        goto done;
    }

    if (i == 0) {
        Com_Printf("^1ERROR: xmodel '%s' has 0 length\n", name);
        goto version_error;
    }

    pos = (const byte *)buf;
    bufEnd = pos + i;
    u.s = *(short *)pos;
    if (u.s != 0x14) {
        Com_Printf("^1ERROR: xmodel '%s' out of date (version %d, expecting %d).", name, (int)(signed short)u.s, 0x14);
    version_error:
        FS_FreeFile(buf);
        model = NULL;
        goto done;
    }

    config.flags = *(unsigned char *)(pos + 2);

    memcpy(&config.mins[0], pos + 3, 4);
    memcpy(&config.mins[1], pos + 7, 4);
    memcpy(&config.mins[2], pos + 11, 4);
    memcpy(&config.maxs[0], pos + 15, 4);
    memcpy(&config.maxs[1], pos + 19, 4);
    memcpy(&config.maxs[2], pos + 23, 4);

    {
        const byte *cur = pos + 27;

        for (i = 0; i < 4; i++) {
            memcpy(&config.entries[i].dist, cur, 4);
            cur += 4;
            strcpy(config.entries[i].filename, (const char *)cur);
            cur += strlen((const char *)cur) + 1;
        }

        memcpy(&config.collLod, cur, 4);
        cur += 4;

        size = 0;
        for (j = 0; j < 4; j++) {
            nameLens[j] = strlen(config.entries[j].filename) + 1;
            size += nameLens[j];
        }

        size += sizeof(XModel);
        model = (XModel *)Alloc(size);
        model->memUsage = size;

        {
            int numCS;
            memcpy(&numCS, cur, 4);
            cur += 4;

            if (numCS < 0 ||
                (size_t)numCS > (size_t)0x7fffffff / sizeof(XModelCollSurf) ||
                (cur + (long long)numCS * (long long)sizeof(XModelCollSurf)) > bufEnd) {
                Com_Printf("^1ERROR: xmodel '%s' has invalid numCollSurfs %d\n", name, numCS);
                FS_FreeFile(buf);
                model = NULL;
                goto done;
            }
            model->numCollSurfs = numCS;

            if (numCS != 0) {

                model->collSurfs = (XModelCollSurf *)AllocColl(numCS * sizeof(XModelCollSurf));

                if (model->numCollSurfs > 0) {
                    float epsilon = 0.0010000000474974513f;
                    for (i = 0; i < model->numCollSurfs; i++) {
                        XModelCollSurf *surf = &model->collSurfs[i];
                        int numTris;

                        memcpy(&numTris, cur, 4);
                        cur += 4;

                        if (numTris < 0 ||
                            (size_t)numTris > (size_t)0x7fffffff / sizeof(XModelCollTri) ||
                            (cur + (long long)numTris * (long long)sizeof(XModelCollTri)) > bufEnd) {
                            Com_Printf("^1ERROR: xmodel '%s' has invalid numCollTris %d\n", name, numTris);
                            FS_FreeFile(buf);
                            model = NULL;
                            goto done;
                        }
                        surf->numCollTris = numTris;

                        surf->collTris = (XModelCollTri *)AllocColl(numTris * sizeof(XModelCollTri));

                        if (surf->numCollTris > 0) {
                            for (j = 0; j < surf->numCollTris; j++) {
                                XModelCollTri *tri = &surf->collTris[j];

                                memcpy(&tri->plane[0], cur, 4);
                                cur += 4;
                                memcpy(&tri->plane[1], cur, 4);
                                cur += 4;
                                memcpy(&tri->plane[2], cur, 4);
                                cur += 4;
                                memcpy(&tri->plane[3], cur, 4);
                                cur += 4;
                                memcpy(&tri->svec[0], cur, 4);
                                cur += 4;
                                memcpy(&tri->svec[1], cur, 4);
                                cur += 4;
                                memcpy(&tri->svec[2], cur, 4);
                                cur += 4;
                                memcpy(&tri->svec[3], cur, 4);
                                cur += 4;
                                memcpy(&tri->tvec[0], cur, 4);
                                cur += 4;
                                memcpy(&tri->tvec[1], cur, 4);
                                cur += 4;
                                memcpy(&tri->tvec[2], cur, 4);
                                cur += 4;
                                memcpy(&tri->tvec[3], cur, 4);
                                cur += 4;
                            }
                        }

                        {
                            float tmp;
                            memcpy(&tmp, cur, 4);
                            surf->mins[0] = tmp - epsilon;
                            cur += 4;
                            memcpy(&tmp, cur, 4);
                            surf->mins[1] = tmp - epsilon;
                            cur += 4;
                            memcpy(&tmp, cur, 4);
                            surf->mins[2] = tmp - epsilon;
                            cur += 4;
                            memcpy(&tmp, cur, 4);
                            surf->maxs[0] = tmp + epsilon;
                            cur += 4;
                            memcpy(&tmp, cur, 4);
                            surf->maxs[1] = tmp + epsilon;
                            cur += 4;
                            memcpy(&tmp, cur, 4);
                            surf->maxs[2] = tmp + epsilon;
                            cur += 4;
                        }

                        memcpy(&surf->boneIdx, cur, 4);
                        cur += 4;

                        {
                            int contents;
                            memcpy(&contents, cur, 4);
                            cur += 4;
                            surf->contents = contents & 0xdffffffb;
                        }

                        memcpy(&surf->surfFlags, cur, 4);
                        cur += 4;

                        model->contents |= surf->contents;
                    }
                }
            }
        }

        lodFilename = (char *)model + sizeof(XModel);
        model->numLods = 0;

        for (i = 0; i < 4; i++) {
            strcpy(lodFilename, config.entries[i].filename);
            model->lodInfo[i].filename = lodFilename;

            if (*lodFilename == '\0') {
                model->lodInfo[i].surfNames = NULL;
                goto next_lod;
            }

            model->numLods += 1;

            {
                short ns;
                memcpy(&ns, cur, 2);
                cur += 2;

                model->lodInfo[i].numsurfs = ns;

                model->lodInfo[i].surfNames = (short unsigned int *)Alloc(ns * 2);
                model->memUsage += ns * 2;

                if (model->lodInfo[i].numsurfs > 0) {
                    for (j = 0; j < model->lodInfo[i].numsurfs; j++) {
                        int len;
                        const char *str = (const char *)cur;
                        len = strlen(str) + 1;
                        cur += len;
                        model->lodInfo[i].surfNames[j] = (unsigned short)SL_GetString_(str, 0, 8);
                    }
                }
            }

        next_lod:
            model->lodInfo[i].dist = config.entries[i].dist;
            lodFilename += nameLens[i];
        }
#ifdef __EMSCRIPTEN__
        if (model->numLods > 0 &&
            memcmp(name, "weapon_", 7) != 0 &&
            memcmp(name, "projectile_", 11) != 0) {
            int lastLod = model->numLods - 1;
            float maxDist = model->lodInfo[lastLod].dist;
            if (maxDist >= 800.0f && maxDist < 3600.0f) {
                float factor = 3600.0f / maxDist;
                if (factor > 3.5f)
                    factor = 3.5f;
                for (i = 0; i < model->numLods; i++)
                    model->lodInfo[i].dist *= factor;
            } else if (maxDist == 0.0f && model->numLods > 1) {
                for (i = 0; i < lastLod; i++) {
                    if (model->lodInfo[i].dist >= 250.0f && model->lodInfo[i].dist < 2400.0f)
                        model->lodInfo[i].dist *= 1.6f;
                }
            }
        }
#endif

        {
            const char *partsName = model->lodInfo[0].filename;

            modelParts = XModelPartsFindData(partsName);
            if (!modelParts) {

                if (Com_sprintf(partsFilename, 64, "xmodelparts/%s", partsName) < 0) {
                    Com_Printf("^1ERROR: filename '%s' too long\n", partsFilename);
                    goto cannot_find_parts;
                }

                i = FS_ReadFile(partsFilename, &partsBuf);
                if (i < 0) {
                    Com_Printf("^1ERROR: xmodelparts '%s' not found\n", partsName);
                    goto cannot_find_parts;
                }

                if (i == 0) {
                    Com_Printf("^1ERROR: xmodelparts '%s' has 0 length\n", partsName);
                    FS_FreeFile(partsBuf);
                cannot_find_parts:
                    Com_Printf("^1ERROR: Cannot find xmodelparts '%s'.\n", partsName);
                    modelParts = NULL;
                    goto have_parts;
                }

                {
                    const byte *partsPos = (const byte *)partsBuf;
                    short ver;
                    memcpy(&ver, partsPos, 2);
                    if (ver != 0x14) {
                        FS_FreeFile(partsBuf);
                        Com_Printf("^1ERROR: xmodelparts '%s' out of date (version %d, expecting %d).", partsName, (int)ver, 0x14);
                        goto cannot_find_parts;
                    }

                    {
                        short numRootBones;
                        short numNonRootBones;
                        short totalBones;
                        int numNonRoot;
                        short unsigned int *boneNames;
                        XBoneHierarchy *hierarchy;
                        const byte *filePos;

                        memcpy(&numRootBones, partsPos + 2, 2);
                        memcpy(&numNonRootBones, partsPos + 4, 2);
                        numNonRoot = (int)(signed short)numNonRootBones;
                        filePos = partsPos + 6;

                        totalBones = numRootBones + numNonRootBones;

                        if ((int)(signed short)totalBones > 127) {
                            FS_FreeFile(partsBuf);
                            Com_Printf("^1ERROR: xmodel '%s' has more than %d bones\n", partsName, 127);
                            goto cannot_find_parts;
                        }

                        boneNames = (short unsigned int *)Alloc((int)(signed short)totalBones * 2);
                        model->memUsage += (int)(signed short)totalBones * 2;

                        {
                            int hierSize = (int)(signed short)numRootBones + 7;
                            hierarchy = (XBoneHierarchy *)Alloc(hierSize);
                            model->memUsage += hierSize;
                        }

                        hierarchy->names = boneNames;

                        {
                            int partsSize = (int)(signed short)totalBones * 32 + 0x44;
                            modelParts = (XModelParts *)Alloc(partsSize);
                            model->memUsage += partsSize;
                        }

                        modelParts->hierarchy = hierarchy;

                        if (numRootBones != 0) {
                            modelParts->quats = (short int *)Alloc((int)(signed short)numRootBones * 8);
                            model->memUsage += (int)(signed short)numRootBones * 8;
                            modelParts->trans = (float *)Alloc((int)(signed short)numRootBones * 16);
                            model->memUsage += (int)(signed short)numRootBones * 16;
                        } else {
                            modelParts->quats = NULL;
                            modelParts->trans = NULL;
                        }

                        modelParts->partClassification = (unsigned char *)Alloc((int)(signed short)totalBones);
                        model->memUsage += (int)(signed short)totalBones;

                        modelParts->numBones = totalBones;
                        modelParts->numRootBones = numNonRootBones;

                        {
                            int numParented = (int)(signed short)totalBones - numNonRoot;
                            short *quats = modelParts->quats;
                            float *transArr = modelParts->trans;
                            unsigned char *parentList = hierarchy->parentList;

                            for (i = 0; i < numParented; i++) {
                                short qx, qy, qz;
                                int rem;

                                parentList[i] = (unsigned char)((i + numNonRoot) - (int)filePos[0]);

                                memcpy(&transArr[i * 3 + 0], filePos + 1, 4);
                                memcpy(&transArr[i * 3 + 1], filePos + 5, 4);
                                memcpy(&transArr[i * 3 + 2], filePos + 9, 4);

                                memcpy(&qx, filePos + 13, 2);
                                memcpy(&qy, filePos + 15, 2);
                                memcpy(&qz, filePos + 17, 2);
                                filePos += 19;

                                quats[i * 4 + 0] = qx;
                                quats[i * 4 + 1] = qy;
                                quats[i * 4 + 2] = qz;

                                rem = 0x3fff0001 - (int)qx * (int)qx - (int)qy * (int)qy - (int)qz * (int)qz;
                                if (rem > 0) {
                                    quats[i * 4 + 3] = (short)(int)floorf(sqrtf((float)rem) + 0.5f);
                                } else {
                                    quats[i * 4 + 3] = 0;
                                }
                            }
                        }

                        {
                            int totalB = (int)(signed short)totalBones;
                            for (i = 0; i < totalB; i++) {
                                int slen = strlen((const char *)filePos) + 1;
                                boneNames[i] = (unsigned short)SL_GetStringOfLen((const char *)filePos, 0, slen, 10);
                                filePos += slen;
                            }
                        }

                        memcpy(modelParts->partClassification, filePos, (int)(signed short)totalBones);

                        {
                            DObjAnimMat *matBase = (DObjAnimMat *)&((XModelParts *)modelParts)->skel.mat[0].quat[0];
                            int numFree = (int)(signed short)numNonRootBones;
                            int numParented = (int)(signed short)totalBones - (int)(signed short)numNonRootBones;
                            short *quatData = modelParts->quats;
                            float *transArr = modelParts->trans;
                            unsigned char *parentList = hierarchy->parentList;
                            float scale = 3.0518509447574615e-05f;
                            float fone = 1.0f;
                            float two = 2.0f;
                            int bi;

                            for (bi = 0; bi < numFree; bi++) {
                                matBase[bi].quat[0] = 0.0f;
                                matBase[bi].quat[1] = 0.0f;
                                matBase[bi].quat[2] = 0.0f;
                                matBase[bi].quat[3] = 1.0f;
                                matBase[bi].trans[0] = 0.0f;
                                matBase[bi].trans[1] = 0.0f;
                                matBase[bi].trans[2] = 0.0f;
                                matBase[bi].transWeight = 2.0f;
                            }

                            for (bi = 0; bi < numParented; bi++) {
                                DObjAnimMat *mat = &matBase[numFree + bi];
                                DObjAnimMat *parentMat = (DObjAnimMat *)((byte *)mat - ((int)parentList[bi] << 5));
                                float q[4];

                                q[0] = (float)(short)quatData[bi * 4 + 0] * scale;
                                q[1] = (float)(short)quatData[bi * 4 + 1] * scale;
                                q[2] = (float)(short)quatData[bi * 4 + 2] * scale;
                                q[3] = (float)(short)quatData[bi * 4 + 3] * scale;

                                QuatMultiply(q, (float *)parentMat, (float *)mat);

                                {
                                    float len2 = mat->quat[0] * mat->quat[0] + mat->quat[1] * mat->quat[1] + mat->quat[2] * mat->quat[2] + mat->quat[3] * mat->quat[3];
                                    if (len2 == 0.0f) {
                                        mat->quat[3] = fone;
                                        mat->transWeight = two;
                                    } else {
                                        mat->transWeight = two / len2;
                                    }
                                }

                                {
                                    float scale2 = parentMat->transWeight;
                                    float sx = scale2 * parentMat->quat[0];
                                    float sy = scale2 * parentMat->quat[1];
                                    float sz = scale2 * parentMat->quat[2];

                                    float xx = sx * parentMat->quat[0];
                                    float xy = sx * parentMat->quat[1];
                                    float xz = sx * parentMat->quat[2];
                                    float xw = sx * parentMat->quat[3];

                                    float yy = sy * parentMat->quat[1];
                                    float yz = sy * parentMat->quat[2];
                                    float yw = sy * parentMat->quat[3];

                                    float zz = sz * parentMat->quat[2];
                                    float zw = sz * parentMat->quat[3];

                                    float lt0 = transArr[bi * 3 + 0];
                                    float lt1 = transArr[bi * 3 + 1];
                                    float lt2 = transArr[bi * 3 + 2];

                                    mat->trans[0] = (fone - (yy + zz)) * lt0 + (xy - zw) * lt1 + (xz + yw) * lt2 + parentMat->trans[0];
                                    mat->trans[1] = (xy + zw) * lt0 + (fone - (xx + zz)) * lt1 + (yz - xw) * lt2 + parentMat->trans[1];
                                    mat->trans[2] = (xz - yw) * lt0 + (yz + xw) * lt1 + (fone - (xx + yy)) * lt2 + parentMat->trans[2];
                                }
                            }
                        }

                        {
                            int *bits1 = (int *)&((XModelParts *)modelParts)->skel.animPartBits[0];
                            int *bits2 = (int *)&((XModelParts *)modelParts)->skel.skelPartBits[0];
                            bits1[0] = -1;
                            bits1[1] = -1;
                            bits1[2] = -1;
                            bits1[3] = -1;
                            bits2[0] = -1;
                            bits2[1] = -1;
                            bits2[2] = -1;
                            bits2[3] = -1;
                        }

                        XModelPartsSetData(partsName, modelParts, Alloc);
                    }
                }
            }
        }

    have_parts:
        model->parts = (void (*)())modelParts;

        if (!modelParts) {

            FS_FreeFile(buf);
            XModelFree(model);
            model = NULL;
            goto done;
        }

        {
            int numBones = (int)(signed short)modelParts->numBones;
            int boneInfoSize = numBones * 40;
            XBoneInfo *boneInfo;

            boneInfo = (XBoneInfo *)Alloc(boneInfoSize);
            model->memUsage += boneInfoSize;

            if (numBones > 0) {
                float half = 0.5f;
                for (i = 0; i < numBones; i++) {

                    memcpy(&boneInfo[i].bounds[0][0], cur, 4);
                    cur += 4;
                    memcpy(&boneInfo[i].bounds[0][1], cur, 4);
                    cur += 4;
                    memcpy(&boneInfo[i].bounds[0][2], cur, 4);
                    cur += 4;
                    memcpy(&boneInfo[i].bounds[1][0], cur, 4);
                    cur += 4;
                    memcpy(&boneInfo[i].bounds[1][1], cur, 4);
                    cur += 4;
                    memcpy(&boneInfo[i].bounds[1][2], cur, 4);
                    cur += 4;

                    boneInfo[i].offset[0] = (boneInfo[i].bounds[0][0] + boneInfo[i].bounds[1][0]) * half;
                    boneInfo[i].offset[1] = (boneInfo[i].bounds[0][1] + boneInfo[i].bounds[1][1]) * half;
                    boneInfo[i].offset[2] = (boneInfo[i].bounds[0][2] + boneInfo[i].bounds[1][2]) * half;

                    {
                        float dx = boneInfo[i].bounds[1][0] - boneInfo[i].offset[0];
                        float dy = boneInfo[i].bounds[1][1] - boneInfo[i].offset[1];
                        float dz = boneInfo[i].bounds[1][2] - boneInfo[i].offset[2];
                        boneInfo[i].radiusSquared = dx * dx + dy * dy + dz * dz;
                    }
                }
            }

            model->boneInfo = boneInfo;
        }

        FS_FreeFile(buf);

        model->mins[0] = config.mins[0];
        model->mins[1] = config.mins[1];
        model->mins[2] = config.mins[2];
        model->maxs[0] = config.maxs[0];
        model->maxs[1] = config.maxs[1];
        model->maxs[2] = config.maxs[2];

        model->collLod = (short)config.collLod;
        model->flags = config.flags;

        if (!re.XModelAllowReadSurface) {
            goto done;
        }

        for (lodIndex = 0; lodIndex < 4; lodIndex++) {
            const char *lodName = model->lodInfo[lodIndex].filename;
            if (*lodName == '\0') {
                break;
            }

            modelName = model->name;
            modelNumSurfs = model->lodInfo[lodIndex].numsurfs;

            {
                XModelSurfs *modelSurfs = XModelSurfsFindData(lodName);

                if (!modelSurfs) {

                    if (Com_sprintf(filename, 64, "xmodelsurfs/%s", lodName) < 0) {
                        Com_Printf("^1ERROR: filename '%s' too long\n", filename);
                        goto cannot_find_surfs;
                    }

                    i = FS_ReadFile(filename, &buf);
                    if (i < 0) {
                        Com_Printf("^1ERROR: xmodelsurf '%s' not found\n", lodName);
                        goto cannot_find_surfs;
                    }

                    if (i == 0) {
                        Com_Printf("^1ERROR: xmodelsurf '%s' has 0 length\n", lodName);
                        FS_FreeFile(buf);
                    cannot_find_surfs:
                        Com_Printf("^1ERROR: Cannot find 'xmodelsurfs '%s'.\n", lodName);
                        modelSurfs = NULL;
                        goto have_surfs;
                    }

                    {
                        const byte *surfsPos = (const byte *)buf;
                        short surfsVer;
                        memcpy(&surfsVer, surfsPos, 2);
                        surfsPos += 2;

                        if (surfsVer != 0x14) {
                            FS_FreeFile(buf);
                            Com_Printf("^1ERROR: xmodelsurfs '%s' out of date (version %d, expecting %d).", lodName, (int)(signed short)surfsVer, 0x14);
                            goto cannot_find_surfs;
                        }

                        {
                            short fileNumSurfs;
                            memcpy(&fileNumSurfs, surfsPos, 2);
                            surfsPos += 2;

                            if (modelNumSurfs != fileNumSurfs) {
                                FS_FreeFile(buf);
                                Com_Printf("^1ERROR: File conflict (between non-iwd and iwd file) on xmo", lodName, modelName);
                                goto cannot_find_surfs;
                            }

                            {
                                int surfsAllocSize = 0x14 + (int)(signed short)modelNumSurfs * 4;
                                XSurface **surfPtrs;

                                modelSurfs = (XModelSurfs *)Alloc(surfsAllocSize);
                                model->memUsage += surfsAllocSize;

                                surfPtrs = (XSurface **)((byte *)modelSurfs + 0x14);
                                modelSurfs->surfs = surfPtrs;

                                if ((int)(signed short)modelNumSurfs > 0) {
                                    int *partBits = (int *)((byte *)modelSurfs + 4);
                                    for (j = 0; j < (int)(signed short)modelNumSurfs; j++) {
                                        surfPtrs[j] = XModelReadSurface(model, partBits, &surfsPos, Alloc);
                                    }
                                }

                                FS_FreeFile(buf);
                                XModelSurfsSetData(lodName, modelSurfs, Alloc);
                            }
                        }
                    }
                }

            have_surfs:
                model->lodInfo[lodIndex].surfs = modelSurfs;
                if (!modelSurfs) {

                    XModelFree(model);
                    model = NULL;
                    goto done;
                }
            }
        }

        model->xskins = R_LoadXSkins(model);
    }

done:
    return model;
}
