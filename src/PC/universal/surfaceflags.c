#include "common_types.h"
#include "imports.h"


extern infoParm_t infoParms[54];

extern int stricmp(const char *, const char *);

int Com_SurfaceTypeFromName(const char *name)
{
    int i;

    if (stricmp(name, "default") == 0)
        return 0;

    for (i = 0; i < 22; i++) {
        if (stricmp(name, infoParms[i].name) == 0) {
            return (int)(((unsigned int)infoParms[i].surfaceFlags & 0x1f00000) >> 20);
        }
    }

    return -1;
}

const char *Com_SurfaceTypeToName(int iTypeIndex)
{
    if (iTypeIndex < 1 || iTypeIndex > 22)
        return "default";

    return infoParms[iTypeIndex - 1].name;
}

infoParm_t infoParms[54] = {
    { (char *)&str_0022009c, 0x0, 0x100000, 0x0, 0x0 },
    { (char *)&str_002200a4, 0x0, 0x200000, 0x0, 0x0 },
    { (char *)&str_002200ac, 0x0, 0x300000, 0x0, 0x0 },
    { (char *)&str_002200b4, 0x0, 0x400000, 0x0, 0x0 },
    { (char *)&str_002200bc, 0x0, 0x500000, 0x0, 0x0 },
    { (char *)&str_002200c8, 0x0, 0x600000, 0x0, 0x0 },
    { (char *)&str_002200d0, 0x0, 0x700000, 0x0, 0x0 },
    { (char *)&str_002200d8, 0x1, 0x800000, 0x2, 0x0 },
    { (char *)&str_002200e0, 0x1, 0x900000, 0x10, 0x0 },
    { (char *)&str_002200e8, 0x0, 0xa00000, 0x0, 0x0 },
    { (char *)&str_002200f0, 0x0, 0xb00000, 0x0, 0x0 },
    { (char *)&str_002200f8, 0x0, 0xc00000, 0x0, 0x0 },
    { (char *)&str_002200fc, 0x0, 0xd00000, 0x0, 0x0 },
    { (char *)&str_00220104, 0x0, 0xe00000, 0x0, 0x0 },
    { (char *)&str_00220108, 0x0, 0xf00000, 0x0, 0x0 },
    { (char *)&str_00220110, 0x0, 0x1000000, 0x0, 0x0 },
    { (char *)&str_00220118, 0x0, 0x1100000, 0x0, 0x0 },
    { (char *)&str_00220120, 0x0, 0x1200000, 0x0, 0x0 },
    { (char *)&str_00220128, 0x0, 0x1300000, 0x0, 0x0 },
    { (char *)&str_00220130, 0x1, 0x1400000, 0x20, 0x0 },
    { (char *)&str_00220138, 0x0, 0x1500000, 0x0, 0x0 },
    { (char *)&str_00220140, 0x0, 0x1600000, 0x0, 0x0 },
    { (char *)&str_00220148, 0x0, 0x1600000, 0x0, 0x0 },
    { (char *)&str_00220154, 0x1, 0x0, 0x80, 0x0 },
    { (char *)&str_00220160, 0x1, 0x0, 0x1000, 0x0 },
    { (char *)&str_0022016c, 0x1, 0x0, 0x2000, 0x0 },
    { (char *)&str_00220178, 0x1, 0x0, 0x10000, 0x0 },
    { (char *)&str_00220184, 0x1, 0x0, 0x20000, 0x0 },
    { (char *)&str_00220190, 0x1, 0x0, 0x200, 0x0 },
    { (char *)&str_0022019c, 0x1, 0x0, 0x400, 0x0 },
    { (char *)&str_002201a8, 0x1, 0x0, 0x80000000, 0x0 },
    { (char *)&str_002201b0, 0x1, 0x4000, 0x0, 0x0 },
    { (char *)&str_002201bc, 0x0, 0x0, 0x8000000, 0x0 },
    { (char *)&str_002201c4, 0x0, 0x0, 0x10000000, 0x0 },
    { (char *)&str_002201d0, 0x1, 0x80000000, 0x0, 0x0 },
    { (char *)&str_002201d8, 0x0, 0x0, 0x40, 0x0 },
    { (char *)&str_0021a570, 0x1, 0x0, 0x0, 0x4 },
    { (char *)&str_002201e8, 0x0, 0x4, 0x800, 0x0 },
    { (char *)&str_002201ec, 0x0, 0x10000, 0x0, 0x0 },
    { (char *)&str_002201f8, 0x0, 0x40000, 0x0, 0x0 },
    { (char *)&str_00220208, 0x0, 0x2, 0x0, 0x0 },
    { (char *)&str_00220210, 0x0, 0x10, 0x0, 0x0 },
    { (char *)&str_0022021c, 0x0, 0x20, 0x0, 0x0 },
    { (char *)&str_00220224, 0x0, 0x8, 0x0, 0x0 },
    { (char *)&str_0022022c, 0x0, 0x1, 0x0, 0x0 },
    { (char *)&str_00220238, 0x0, 0x2000000, 0x1000000, 0x0 },
    { (char *)&str_00220244, 0x0, 0x4000000, 0x1000000, 0x0 },
    { (char *)&str_00220250, 0x0, 0x2000, 0x0, 0x0 },
    { (char *)&str_00220258, 0x0, 0x80, 0x0, 0x0 },
    { (char *)&str_00220260, 0x0, 0x800, 0x0, 0x0 },
    { (char *)&str_0022026c, 0x0, 0x400, 0x0, 0x0 },
    { (char *)&str_00220278, 0x0, 0x20000, 0x0, 0x0 },
    { 0, 0x0, 0x0, 0x0, 0x0 },
    { 0, 0x0, 0x0, 0x0, 0x0 }
};

const char str_0021a570[] = "origin";
const char str_0022009c[] = "bark";
const char str_002200a4[] = "brick";
const char str_002200ac[] = "carpet";
const char str_002200b4[] = "cloth";
const char str_002200bc[] = "concrete";
const char str_002200c8[] = "dirt";
const char str_002200d0[] = "flesh";
const char str_002200d8[] = "foliage";
const char str_002200e0[] = "glass";
const char str_002200e8[] = "grass";
const char str_002200f0[] = "gravel";
const char str_002200f8[] = "ice";
const char str_002200fc[] = "metal";
const char str_00220104[] = "mud";
const char str_00220108[] = "paper";
const char str_00220110[] = "plaster";
const char str_00220118[] = "rock";
const char str_00220120[] = "sand";
const char str_00220128[] = "snow";
const char str_00220130[] = "water";
const char str_00220138[] = "wood";
const char str_00220140[] = "asphalt";
const char str_00220148[] = "opaqueglass";
const char str_00220154[] = "clipmissile";
const char str_00220160[] = "ai_nosight";
const char str_0022016c[] = "clipshot";
const char str_00220178[] = "playerclip";
const char str_00220184[] = "monsterclip";
const char str_00220190[] = "vehicleclip";
const char str_0022019c[] = "itemclip";
const char str_002201a8[] = "nodrop";
const char str_002201b0[] = "nonsolid";
const char str_002201bc[] = "detail";
const char str_002201c4[] = "structural";
const char str_002201d0[] = "portal";
const char str_002201d8[] = "canshootclip";
const char str_002201e8[] = "sky";
const char str_002201ec[] = "alphashadow";
const char str_002201f8[] = "nocastshadow";
const char str_00220208[] = "slick";
const char str_00220210[] = "noimpact";
const char str_0022021c[] = "nomarks";
const char str_00220224[] = "ladder";
const char str_0022022c[] = "nodamage";
const char str_00220238[] = "mantleOn";
const char str_00220244[] = "mantleOver";
const char str_00220250[] = "nosteps";
const char str_00220258[] = "nodraw";
const char str_00220260[] = "pointlight";
const char str_0022026c[] = "nolightmap";
const char str_00220278[] = "nodlight";
