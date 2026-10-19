#include "common_types.h"
#include "headers/PC/bgame/bg_attachment_rules.h"
#include "headers/PC/cgame_mp/cg_attachmodels.h"
#include <stdio.h>
#include <string.h>

extern struct XModel *CL_RegisterModel(const char *name);
extern int XModelBad(void *model);
extern int FS_ReadFile(const char *qpath, void **buffer);

/* What each visible attachment's model rides, as browser_attachment_models.py builds them:
 * xmodel/<gun model>__<attachment>, and for the Extended Mag the gun without its magazine,
 * xmodel/<gun model>__nomag; a rifle loaded through the top has instead a box magazine on its
 * receiver, xmodel/<gun model>__extbox. */
static const struct {
    int attachment;
    const char *bone;
} s_cgAttachRides[] = {
    { ATT_SUPPRESSOR, "tag_flash" },
    { ATT_REFLEX, "j_gun" },
    { ATT_LENS, "j_gun" },
    { ATT_OPTIC4X, "j_gun" },
    { ATT_EXTMAG, "j_clip" },
};
#define CG_ATTACH_RIDES ((int)(sizeof(s_cgAttachRides) / sizeof(s_cgAttachRides[0])))
#define CG_ATTACH_NOMAG CG_ATTACH_RIDES
#define CG_ATTACH_EXTBOX (CG_ATTACH_RIDES + 1)
#define CG_ATTACH_WEAPONS 128

/* Each weapon's models, looked up once: a model missing from the archives loads as a fresh
 * default each time it is registered. */
static struct XModel *s_cgAttachModels[CG_ATTACH_WEAPONS][CG_ATTACH_RIDES + 2];
static unsigned char s_cgAttachLooked[CG_ATTACH_WEAPONS][CG_ATTACH_RIDES + 2];

static struct XModel *CG_AttachModel(int weapon, int ride, const char *gunModel, const char *suffix)
{
    char name[96];

    if (weapon <= 0 || weapon >= CG_ATTACH_WEAPONS || !gunModel || !gunModel[0])
        return NULL;
    if (!s_cgAttachLooked[weapon][ride]) {
        s_cgAttachLooked[weapon][ride] = 1;
        snprintf(name, sizeof(name), "xmodel/%s__%s", gunModel, suffix);
        if (FS_ReadFile(name, NULL) > 0) {
            struct XModel *model = CL_RegisterModel(name);

            if (model && !XModelBad(model))
                s_cgAttachModels[weapon][ride] = model;
        }
    }
    return s_cgAttachModels[weapon][ride];
}

unsigned CG_VisibleAttachments(unsigned mask)
{
    unsigned visible = 0;
    int i;

    for (i = 0; i < CG_ATTACH_RIDES; i++)
        visible |= ATT_BIT(s_cgAttachRides[i].attachment);
    return mask & visible;
}

int CG_ViewModelAttachmentModels(int weapon, const char *gunModel, unsigned mask, DObjModel_s *models, int count)
{
    int i;

    if (mask & ATT_BIT(ATT_EXTMAG)) {
        int ride;
        struct XModel *nomag = CG_AttachModel(weapon, CG_ATTACH_NOMAG, gunModel, "nomag");

        for (ride = 0; s_cgAttachRides[ride].attachment != ATT_EXTMAG; ride++)
            ;
        if (nomag && CG_AttachModel(weapon, ride, gunModel, g_attachments[ATT_EXTMAG].name)) {
            models[1].model = nomag;
        } else {
            struct XModel *box = CG_AttachModel(weapon, CG_ATTACH_EXTBOX, gunModel, "extbox");

            if (box && count < CG_VIEW_DOBJ_MODELS) {
                models[count].model = box;
                models[count].boneName = "j_gun";
                models[count].ignoreCollision = 1;
                count++;
            }
            mask &= ~ATT_BIT(ATT_EXTMAG);
        }
    }
    for (i = 0; i < CG_ATTACH_RIDES && count < CG_VIEW_DOBJ_MODELS; i++) {
        int attachment = s_cgAttachRides[i].attachment;
        struct XModel *model;

        if (!(mask & ATT_BIT(attachment)))
            continue;
        model = CG_AttachModel(weapon, i, gunModel, g_attachments[attachment].name);
        if (!model)
            continue;
        models[count].model = model;
        models[count].boneName = s_cgAttachRides[i].bone;
        models[count].ignoreCollision = 1;
        count++;
    }
    return count;
}

void CG_ViewModelAttachmentsForget(int weapon)
{
    if (weapon <= 0 || weapon >= CG_ATTACH_WEAPONS)
        return;
    memset(s_cgAttachModels[weapon], 0, sizeof(s_cgAttachModels[weapon]));
    memset(s_cgAttachLooked[weapon], 0, sizeof(s_cgAttachLooked[weapon]));
}
