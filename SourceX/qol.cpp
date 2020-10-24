/**
 * @file qol.cpp
 *
 * Quality of life features
 */
#include "all.h"
#include "options.h"
#include "DiabloUI/art_draw.h"

DEVILUTION_BEGIN_NAMESPACE
namespace {

Art ArtHealthBox;
Art ArtResistance;
Art ArtHealth;

int drawMinX;
int drawMaxX;
/**
 * 0 = disabled
 * 1 = highlight when alt pressed
 * 2 = hide when alt pressed
 * 3 = always highlight
 */
int highlightItemsMode = 0;
bool altPressed = false;
bool generatedLabels = false;
bool isGeneratingLabels = false;
bool isLabelHighlighted = false;

BYTE *qolbuff;
int labelCenterOffsets[ITEMTYPES];

int GetTextWidth(const char *s)
{
	int l = 0;
	while (*s) {
		l += fontkern[fontframe[gbFontTransTbl[(BYTE)*s++]]] + 1;
	}
	return l;
}

void FastDrawHorizLine(CelOutputBuffer out, int x, int y, int width, BYTE col)
{
	memset(out.at(x, y), col, width);
}

void FastDrawVertLine(CelOutputBuffer out, int x, int y, int height, BYTE col)
{
	BYTE *p = out.at(x, y);
	for (int j = 0; j < height; j++) {
		*p = col;
		p += out.pitch();
	}
}

void FillRect(CelOutputBuffer out, int x, int y, int width, int height, BYTE col)
{
	for (int j = 0; j < height; j++) {
		FastDrawHorizLine(out, x, y + j, width, col);
	}
}

} // namespace

void FreeQol()
{
	if (sgOptions.Gameplay.bEnemyHealthBar) {
		ArtHealthBox.Unload();
		ArtHealth.Unload();
		ArtResistance.Unload();
	}
}

void InitQol()
{
	if (sgOptions.Gameplay.bEnemyHealthBar) {
		LoadMaskedArt("data\\healthbox.pcx", &ArtHealthBox, 1, 1);
		LoadArt("data\\health.pcx", &ArtHealth);
		LoadMaskedArt("data\\resistance.pcx", &ArtResistance, 6, 1);
	}
}

void DrawMonsterHealthBar(CelOutputBuffer out)
{
	if (!sgOptions.Gameplay.bEnemyHealthBar)
		return;
	if (currlevel == 0)
		return;
	if (pcursmonst == -1)
		return;

	MonsterStruct *mon = &monster[pcursmonst];

	Sint32 width = ArtHealthBox.w();
	Sint32 height = ArtHealthBox.h();
	Sint32 xPos = (gnScreenWidth - width) / 2;
	Sint32 yPos = 18;
	Sint32 border = 3;

	Sint32 maxLife = mon->_mmaxhp;
	if (mon->_mhitpoints > maxLife)
		maxLife = mon->_mhitpoints;

	DrawArt(out, xPos, yPos, &ArtHealthBox);
	DrawHalfTransparentRectTo(out, xPos + border, yPos + border, width - (border * 2), height - (border * 2));
	DrawArt(out, xPos + border + 1, yPos + border + 1, &ArtHealth, 0, (width * mon->_mhitpoints) / maxLife, height - (border * 2) - 2);

	if (sgOptions.Gameplay.bShowMonsterType) {
		Uint8 borderColors[] = { 248 /*undead*/, 232 /*demon*/, 172 /*beast*/ };
		Uint8 borderColor = borderColors[mon->MData->mMonstClass];
		Sint32 borderWidth = width - (border * 2);
		FastDrawHorizLine(out, xPos + border, yPos + border, borderWidth, borderColor);
		FastDrawHorizLine(out, xPos + border, yPos + height - border - 1, borderWidth, borderColor);
		Sint32 borderHeight = height - (border * 2) - 2;
		FastDrawVertLine(out, xPos + border, yPos + border + 1, borderHeight, borderColor);
		FastDrawVertLine(out, xPos + width - border - 1, yPos + border + 1, borderHeight, borderColor);
	}

	Sint32 barLableX = xPos + width / 2 - GetTextWidth(mon->mName) / 2;
	Sint32 barLableY = yPos + 10 + (height - 11) / 2;
	PrintGameStr(out, barLableX - 1, barLableY + 1, mon->mName, COL_BLACK);
	text_color color = COL_WHITE;
	if (mon->_uniqtype != 0)
		color = COL_GOLD;
	else if (mon->leader != 0)
		color = COL_BLUE;
	PrintGameStr(out, barLableX, barLableY, mon->mName, color);

	if (mon->_uniqtype != 0 || monstkills[mon->MType->mtype] >= 15) {
		monster_resistance immunes[] = { IMMUNE_MAGIC, IMMUNE_FIRE, IMMUNE_LIGHTNING };
		monster_resistance resists[] = { RESIST_MAGIC, RESIST_FIRE, RESIST_LIGHTNING };

		Sint32 resOffset = 5;
		for (Sint32 i = 0; i < 3; i++) {
			if (mon->mMagicRes & immunes[i]) {
				DrawArt(out, xPos + resOffset, yPos + height - 6, &ArtResistance, i * 2 + 1);
				resOffset += ArtResistance.w() + 2;
			} else if (mon->mMagicRes & resists[i]) {
				DrawArt(out, xPos + resOffset, yPos + height - 6, &ArtResistance, i * 2);
				resOffset += ArtResistance.w() + 2;
			}
		}
	}
}

void DrawXPBar(CelOutputBuffer out)
{
	if (!sgOptions.Gameplay.bExperienceBar)
		return;

	int barWidth = 306;
	int barHeight = 5;
	int yPos = gnScreenHeight - 9;                 // y position of xp bar
	int xPos = (gnScreenWidth - barWidth) / 2 + 5; // x position of xp bar
	int dividerHeight = 3;
	int numDividers = 10;
	int barColor = 198;
	int emptyBarColor = 0;
	int frameColor = 196;
	bool space = true; // add 1 pixel separator on top/bottom of the bar

	PrintGameStr(out, xPos - 22, yPos + 6, "XP", COL_WHITE);
	int charLevel = plr[myplr]._pLevel;
	if (charLevel == MAXCHARLEVEL - 1)
		return;

	int prevXp = ExpLvlsTbl[charLevel - 1];
	if (plr[myplr]._pExperience < prevXp)
		return;

	Uint64 prevXpDelta_1 = plr[myplr]._pExperience - prevXp;
	int prevXpDelta = ExpLvlsTbl[charLevel] - prevXp;
	int visibleBar = barWidth * prevXpDelta_1 / prevXpDelta;

	FillRect(out, xPos, yPos, barWidth, barHeight, emptyBarColor);
	FastDrawHorizLine(out, xPos - 1, yPos - 1, barWidth + 2, frameColor);
	FastDrawHorizLine(out, xPos - 1, yPos + barHeight, barWidth + 2, frameColor);
	FastDrawVertLine(out, xPos - 1, yPos - 1, barHeight + 2, frameColor);
	FastDrawVertLine(out, xPos + barWidth, yPos - 1, barHeight + 2, frameColor);
	for (int i = 1; i < numDividers; i++)
		FastDrawVertLine(out, xPos - 1 + (barWidth * i / numDividers), yPos - dividerHeight + 3, barHeight, 245);

	FillRect(out, xPos, yPos + (space ? 1 : 0), visibleBar, barHeight - (space ? 2 : 0), barColor);
}

bool HasRoomForGold()
{
	for (int i = 0; i < NUM_INV_GRID_ELEM; i++) {
		int idx = plr[myplr].InvGrid[i];
		if (idx == 0 || (idx > 0 && plr[myplr].InvList[idx]._itype == ITYPE_GOLD && plr[myplr].InvList[idx]._ivalue < MaxGold)) {
			return true;
		}
	}

	return false;
}

void AutoGoldPickup(int pnum)
{
	if (!sgOptions.Gameplay.bAutoGoldPickup)
		return;
	if (pnum != myplr)
		return;
	if (leveltype == DTYPE_TOWN)
		return;
	if (!HasRoomForGold())
		return;

	for (int dir = 0; dir < 8; dir++) {
		int x = plr[pnum]._px + pathxdir[dir];
		int y = plr[pnum]._py + pathydir[dir];
		if (dItem[x][y] != 0) {
			int itemIndex = dItem[x][y] - 1;
			if (item[itemIndex]._itype == ITYPE_GOLD) {
				NetSendCmdGItem(TRUE, CMD_REQUESTAGITEM, pnum, pnum, itemIndex);
				item[itemIndex]._iRequest = TRUE;
				PlaySFX(IS_IGRAB);
			}
		}
	}
}

void diablo_parse_config()
{
	//highlightItemsMode = GetConfigIntValue("highlight items", 0);
}

class drawingQueue {
public:
	int ItemID;
	int Row;
	int Col;
	int x;
	int y;
	int width;
	int height;
	int color;
	char text[64];
	drawingQueue(int x2, int y2, int width2, int height2, int Row2, int Col2, int ItemID2, int q2, char *text2)
	{
		x = x2;
		y = y2;
		Row = Row2;
		Col = Col2;
		ItemID = ItemID2;
		width = width2;
		height = height2;
		color = q2;
		strcpy(text, text2);
	}
};

std::vector<drawingQueue> drawQ;

void UpdateLabels(BYTE *dst, int width)
{
	int xval = (dst - &gpBuffer[0]) % BUFFER_WIDTH;
	if (xval < drawMinX)
		drawMinX = xval;
	xval += width;
	if (xval > drawMaxX)
		drawMaxX = xval;
}

void GenerateLabelOffsets()
{
	if (generatedLabels)
		return;
	isGeneratingLabels = true;
	int itemTypes = gbIsHellfire ? ITEMTYPES : 35;
	for (int i = 0; i < itemTypes; i++) {
		drawMinX = BUFFER_WIDTH;
		drawMaxX = 0;
		CelClippedDraw(BUFFER_WIDTH / 2 - 16, 351, itemanims[i], ItemAnimLs[i], 96);
		labelCenterOffsets[i] = drawMinX - BUFFER_WIDTH / 2 + (drawMaxX - drawMinX) / 2;
	}
	isGeneratingLabels = false;
	generatedLabels = true;
}

void AddItemToDrawQueue(int x, int y, int id)
{
	if (highlightItemsMode == 0 || (highlightItemsMode == 1 && !altPressed) || (highlightItemsMode == 2 && altPressed))
		return;
	ItemStruct *it = &item[id];

	char textOnGround[64];
	if (it->_itype == ITYPE_GOLD) {
		sprintf(textOnGround, "%i gold", it->_ivalue);
	} else {
		sprintf(textOnGround, "%s", it->_iIdentified ? it->_iIName : it->_iName);
	}

	int nameWidth = GetTextWidth((char *)textOnGround);
	x += labelCenterOffsets[ItemCAnimTbl[it->_iCurs]];
	x -= SCREEN_X;
	y -= SCREEN_Y;
	y -= TILE_HEIGHT;
	if (!zoomflag) {
		x <<= 1;
		y <<= 1;
	}
	x -= nameWidth / 2;
	char clr = COL_WHITE;
	if (it->_iMagical == ITEM_QUALITY_MAGIC)
		clr = COL_BLUE;
	if (it->_iMagical == ITEM_QUALITY_UNIQUE)
		clr = COL_GOLD;
	drawQ.push_back(drawingQueue(x, y, nameWidth, 13, it->_ix, it->_iy, id, clr, textOnGround));
}

void HighlightItemsNameOnMap()
{
	isLabelHighlighted = false;
	if (highlightItemsMode == 0 || (highlightItemsMode == 1 && !altPressed) || (highlightItemsMode == 2 && altPressed))
		return;
	const int borderX = 5;
	for (unsigned int i = 0; i < drawQ.size(); ++i) {
		std::map<int, bool> backtrace;

		bool canShow;
		do {
			canShow = true;
			for (unsigned int j = 0; j < i; ++j) {
				if (abs(drawQ[j].y - drawQ[i].y) < drawQ[i].height + 2) {
					int newpos = drawQ[j].x;
					if (drawQ[j].x >= drawQ[i].x && drawQ[j].x - drawQ[i].x < drawQ[i].width + borderX) {
						newpos -= drawQ[i].width + borderX;
						if (backtrace.find(newpos) != backtrace.end())
							newpos = drawQ[j].x + drawQ[j].width + borderX;
					} else if (drawQ[j].x < drawQ[i].x && drawQ[i].x - drawQ[j].x < drawQ[j].width + borderX) {
						newpos += drawQ[j].width + borderX;
						if (backtrace.find(newpos) != backtrace.end())
							newpos = drawQ[j].x - drawQ[i].width - borderX;
					} else
						continue;
					canShow = false;
					drawQ[i].x = newpos;
					backtrace[newpos] = true;
				}
			}
		} while (!canShow);
	}

	for (unsigned int i = 0; i < drawQ.size(); ++i) {
		drawingQueue t = drawQ[i];

		if (t.x < 0 || t.x >= gnScreenWidth || t.y < 0 || t.y >= gnScreenHeight) {
			continue;
		}

		if (MouseX >= t.x && MouseX <= t.x + t.width && MouseY >= t.y - t.height && MouseY <= t.y) {
			if ((invflag || sbookflag) && MouseX > RIGHT_PANEL && MouseY <= SPANEL_HEIGHT) {
			} else if ((chrflag || questlog) && MouseX < SPANEL_WIDTH && MouseY <= SPANEL_HEIGHT) {
			} else if (MouseY >= PANEL_TOP && MouseX >= PANEL_LEFT && MouseX <= PANEL_LEFT + PANEL_WIDTH) {
			} else if (gmenu_is_active() || PauseMode != 0 || deathflag) {
			} else {
				isLabelHighlighted = true;
				cursmx = t.Row;
				cursmy = t.Col;
				pcursitem = t.ItemID;
			}
		}
		int bgcolor = 0;
		if (pcursitem == t.ItemID)
			bgcolor = 134;
		FillRect(t.x, t.y - t.height, t.width + 1, t.height, bgcolor);
		CelOutputBuffer out = GlobalBackBuffer();
		PrintGameStr(out, t.x, t.y - 1, t.text, t.color);
	}
	drawQ.clear();
}

DEVILUTION_END_NAMESPACE
