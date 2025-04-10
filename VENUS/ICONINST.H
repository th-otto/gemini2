/*
 * @(#) Gemini\iconinst.h
 * @(#) Stefan Eissing, 17. Juli 1994
 *
 * description: Header File for iconinst.c
 */


#ifndef G_ICONINFO__
#define G_ICONINFO__

#include "..\common\memfile.h"

#include "userdefs.h"

/* Icontypes (fÅr Desktophintergrund)
 */
#define DI_FILESYSTEM	0x0001
#define DI_TRASHCAN		0x0002
#define DI_SHREDDER		0x0003
#define DI_SCRAPDIR		0x0004
#define DI_FOLDER		0x0005
#define DI_PROGRAM		0x0006

#define ICON_MAGIC	'IcoN'



typedef struct
{
	long magic;						/* Iconmagic */
	word defnumber;					/* Nummer des default Icons */
	word altnumber;					/* Nummer eines altern. Icons */
	word shortcut;					/* shortcut fÅr das Icon */
	char type;						/* Typ des Icons (s.o.) */
	char truecolor;					/* gewÅnschte Iconfarbe */
	USERBLK user;
	ICON_OBJECT icon;				/* Iconinformationen des Icons */
	
	char iconname[MAX_FILENAME_LEN];/* String im Icon */
	char label[MAX_FILENAME_LEN];	/* Label of drive it came from */
	char *fullname;					/* default Path */
} IconInfo;

void InitIconInfo (IconInfo *pi, OBJECT *po, 
	const char *name, char true_color);

IconInfo *GetIconInfo (OBJECT *po);
void FreeIconInfo (IconInfo *pii);

void CopyNewDesk (OBJECT *pnewdesk);
void RemoveDeskIcon (word objnr);

void FillIconObject (ICON_OBJECT *icon, OBJECT *po);
int IconsAreEqual (const OBJECT *po1, const ICONBLK *icon2,
	char color);
void ChangeIconObject (ICON_OBJECT *icon, OBJECT *new_icon,
	char color);


void InstShredderIcon (word obx, word oby, const char *name, char truecolor);
void InstTrashIcon (word obx, word oby, const char *name,
					word shortcut, char truecolor);
void InstScrapIcon (word obx, word oby, const char *name,
					word shortcut, char truecolor);
void InstDriveIcon (word obx, word oby, word todraw, word iconNr,
					char drive, const char *name, word shortcut,
					char truecolor);
void InstPrgIcon (word obx, word oby, word todraw, word normicon,
				word isfolder, const char *path, const char *name,
				char *label, word shortcut);

void AddDefIcons (void);

void DoInstDialog (void);

void InstDraggedIcons (WindInfo *fwp, word fromx, word fromy, 
	word tox, word toy);
void InstArgvOnDesktop (word mx, word my, ARGVINFO *A);

int DoShredderDialog (IconInfo *pii, word objnr);
void RehashDeskIcon (void);
word SortDeskTree (void);

void DeskIconNewLabel (char drive, const char *oldlabel, 
						const char *newlabel);

void AddDeskIcon (word obx, word oby, word todraw,
					OBJECT *po, USERBLK *user);
					
word WriteDeskIcons (MEMFILEINFO *mp, char *buffer);


#endif /* !G_ICONINFO__ */