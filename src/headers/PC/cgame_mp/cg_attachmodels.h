#ifndef CG_ATTACHMODELS_H
#define CG_ATTACHMODELS_H

/* A first-person gun's models: the hands, the gun and its visible attachments. */
#define CG_VIEW_DOBJ_MODELS 8

/* The attachments among mask that change the gun's model. */
unsigned CG_VisibleAttachments(unsigned mask);
/* After the hands and the gun in models[0] and [1], the models of the visible attachments in
 * mask that the archives built for gunModel, each on the bone it rides; an Extended Mag also
 * puts the gun without its own magazine in models[1]. The new model count. */
int CG_ViewModelAttachmentModels(int weapon, const char *gunModel, unsigned mask, DObjModel_s *models, int count);
/* Forget a weapon's looked-up models, as its registration starts again. */
void CG_ViewModelAttachmentsForget(int weapon);

#endif
