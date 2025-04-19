/*
 * @(#)Language/nlstest.c
 * @(#)Stefan Eissing, 29. Dezember 1990
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "nls.h"

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

#define NlsLocalSection "Gemini.Dialog"
enum NlsLocalText{

T_SHAREWARE1,	/*%s ist ein Shareware-Programm. Shareware 
bedeutet, daû Sie dieses Programm fÅr nicht-kommerzielle 
Zwecke frei kopieren und testen dÅrfen. Wenn Sie allerdings 
GEMINI/MUPFEL/VENUS regelmÑûig benutzen, so haben Sie sich an das Konzept von 
Shareware zu halten, indem Sie den Autoren einen Obolus von 
50,- DM entrichten. Dies ist KEIN Public-Domain- oder Freeware-
Programm!*/

T_SHAREWARE2, /*Wenn Sie keine Raubkopie benutzen wollen oder 
an der Weiterentwicklung diese Programms interessiert sind, 
kînnen Sie das Geld auf eins unserer Konten 
Åberweisen:|
|Gereon Steffens,
|%sKto. 1032583765, SSK Kîln, BLZ 370 501 98
|Stefan Eissing,
|%sKto. 0441939400, VB Baumberge eG, BLZ 400 694 08|
|Danke.*/

T_FOLDER,	/*Der Pfad dieses Fensters ist zu lang, um neue 
Ordner anzulegen!*/

T_NONAME,	/*Sie sollten einen Namen angeben!*/

T_EXISTS,	/*Ein Objekt dieses Namens existiert schon!*/

T_INVALID,	/*Dies ist kein gÅltiger Name fÅr eine Datei 
oder einen Ordner! Bitte wÑhlen Sie einen anderen Namen.*/

T_NOSPACE,	/*Die Auflîsung ist zu gering, um diesen Dialog 
darzustellen!*/

T_ESCTEST,  /* Dies ist Kommentaranfang \[ und -ende \]! und 
ein normaler Backslash-escape mit einem Kommentar \\\[ dran 
mit einer Hexzahl \x40 und einer Oktalen \7*/
};

int main(void)
{
	if (NlsInit("nlstest.nls", NULL, Malloc, Mfree))
	{
		puts("NlsInit erfolgreich!");
		puts(NlsStr(T_SHAREWARE1));
		puts(NlsStr(T_SHAREWARE2));
		puts(NlsStr(T_ESCTEST));
		NlsExit();
		return 0;
	}
	return 1;
}
