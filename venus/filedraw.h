/*
 * @(#) Gemini\filedraw.h
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: functions to redraw text in file windows
 */

#ifndef G_filedraw__
#define G_filedraw__

#include <flydial\flydial.h>
#include "..\common\memfile.h"


extern FONTWORK filework;

typedef struct
{
	word id;
	word size;
	word char_width;
	word char_height;
	
	word filled_for_id;
	word filled_for_size;
	word wchar;
	word hchar;
	word num_wchar;
	char is_proportional;

	word kursiv_offset_links;
	word kursiv_offset_rechts;

	struct
	{
		unsigned size : 1;
		unsigned date : 1;
		unsigned time : 1;
	} show;
} FILEFONTINFO;

extern FILEFONTINFO TextFont, SIconFont;
extern FILEFONTINFO SystemNormFont, SystemIconFont;

word cdecl drawFileText (PARMBLK *pb);

void FileWindowDrawRect (FileWindow_t *wp, const GRECT *rect);
void FileWindowRedraw (word wind_handle, FileWindow_t *wp, 
	const GRECT *rect);

int FileWindowDarstellung (WindInfo *wp, int only_asking);

void initFileFonts (void);
void exitFileFonts (void);

word WriteFontInfo (MEMFILEINFO *mp, char *buffer);
void execFontInfo (char *line);

void GetFileFont (word *id, word *points);

word SetFont (FONTWORK *fw, word *id, word *points, word *width, 
	word *height);

void SetIconFont (void);
void SetFileFont (FILEFONTINFO *ffi);
void SetSystemFont (void);


void VstEffects (word effect);


#endif /* !G_filedraw__ */