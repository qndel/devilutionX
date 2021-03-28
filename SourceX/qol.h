/**
 * @file qol.h
 *
 * Quality of life features
 */
#ifndef __QOL_H__
#define __QOL_H__

#include "engine.h"

DEVILUTION_BEGIN_NAMESPACE

extern int drawMinX;
extern int drawMaxX;
extern bool altPressed;
extern bool isGeneratingLabels;
extern bool isLabelHighlighted;

void FreeQol();
void InitQol();
void DrawMonsterHealthBar(CelOutputBuffer out);
void DrawXPBar(CelOutputBuffer out);
void AutoGoldPickup(int pnum);
void UpdateLabels(BYTE *dst, int width);
void GenerateLabelOffsets();
void AddItemToDrawQueue(int x, int y, int id);
void HighlightItemsNameOnMap();
void RepeatClicks();
void AutoPickGold(int pnum);
void RepeatClicks();

DEVILUTION_END_NAMESPACE

#endif /* __QOL_H__ */
