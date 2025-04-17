/*
 * @(#) Gemini\CpKobold.h
 * @(#) Stefan Eissing, 22. Januar 1993
 *
 * description: functions to copy/move/delete files via Kobold
 *
 */

#ifndef G_CPKOBOLD__
#define G_CPKOBOLD__

typedef enum
{
	kb_format,
	kb_copy,
	kb_move,
	kb_delete
} KoboldCopyCommand;


int KoboldPresent (void);

int CopyWithKobold (ARGVINFO *A, const char *dest_dir, 
	KoboldCopyCommand cmd, int confirm, int overwrite, int rename);

#define KOBOLD_ANSWER 0x2F12       /* Antwort vom KOBOLD mit Status in Wort 3  */
                                   /* und Zeile in Wort 4                      */

void GotKoboldAnswer (word messbuff[8]);

#endif