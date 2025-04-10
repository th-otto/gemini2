/*
 * @(#) Common\strerror.c
 * @(#) Julian Reschke, 22. April 1995
 *
 */

typedef struct
{
	int errno;
	char *string;
} ERR;

#if 1
static ERR Errs[] =
{
	0, "Fehlernummer %ld",
	-1, "allgemeiner Fehler",
	-2,	"GerÑt nicht bereit",
	-3, "unbekanntes Kommando",
	-4, "PrÅfsummenfehler",
	-5, "Kommando kann nicht ausgefÅhrt werden",
	-6, "Spur kann nicht angesteuert werden",
	-7, "Medientyp nicht bekannt",
	-8, "Sektor nicht gefunden",
	-9, "kein Papier mehr",
	-10, "Fehler beim Schreiben",
	-11, "Fehler beim Lesen",
	-12, "allgemeiner Fehler",
	-13, "Medium ist schreibgeschÅtzt",
	-14, "Medium wurde gewechselt",
	-15, "GerÑt nicht bekannt",
	-16, "defekte Sektoren beim Formatieren",
	-17, "anderes Medium einlegen",
	-18, "Medium einlegen",
	-19, "GerÑt reagiert nicht",
	-32, "unbekannte Funktionsnummer",
	-33, "Datei nicht gefunden",
	-34, "Verzeichnis nicht gefunden",
	-35, "zuviele Dateien geîffnet",
	-36, "Zugriff verweigert",
	-37, "falsche Dateinummer",
	-39, "zuwenig Speicher",
	-40, "falsche Speicherblockadresse",
	-46, "Laufwerkskennung ungÅltig",
	-48, "nicht auf dem gleichen Dateisystem",
	-49, "keine weiteren Dateien",
	-58, "Bereich ist blockiert",
	-59, "keine Blockierung vorhanden",
	-64, "Parameter auûerhalb des mîglichen Bereichs",
	-65, "interner Fehler im GEMDOS",
	-66, "keine ausfÅhrbare Datei",
	-67, "Speicherblock kann nicht vergrîûert werden",
	
	/* Mag!x */

	-68, "Programm abgebrochen",
	-69, "Programmabsturz",
	-70, "zu langer Verzeichnisname",
	
	/* MiNT */
	
	-80, "zu viele symbolische Verweise",
	-81, "Pipeline abgebrochen",
};
#endif

#if 0
static ERR EnglishErrs[] =
{
	0,	"error code %ld",
	-1, "error",
	-2,	"drive not ready",
	-3, "unknown command",
	-4, "CRC error",
	-5, "bad request",
	-6, "seek error",
	-7, "unknown media",
	-8, "sector not found",
	-9, "out of paper",
	-10, "write fault",
	-11, "read fault",
	-12, "general error",
	-13, "write on write-protected media",
	-14, "media change detected",
	-15, "unknown device",
	-16, "bad sectors on format",
	-17, "insert other disk",
	-18, "insert disk",
	-19, "device not responding",
	-32, "invalid function number",
	-33, "file not found",
	-34, "path not found",
	-35, "handle pool exhausted",
	-36, "access denied",
	-37, "invalid handle",
	-39, "insufficient memory",
	-40, "invalid memory block address",
	-46, "invalid drive specification",
	-48, "not the same drive",
	-49, "no more files",
	-58, "record is locked",
	-59, "no such lock",
	-64, "range error",
	-65, "GEMDOS internal error",
	-66, "invalid executable format",
	-67, "memory block growth failure",

	/* Mag!x */

	-68, "program aborted",
	-69, "program crashed",
	-70, "path name too long",

	/* MiNT */

	-80, "too many symbolic links",
	-81, "broken pipe",
};
#endif

const char *
StrError (long error)
{
	int i;

	for (i = 1; i < sizeof (Errs) / sizeof (ERR); i++)
		if (error == Errs[i].errno)
			return Errs[i].string;

#if 1
	return "unbekannter Fehler";
#endif
#if 0
	return	"unknown error";
#endif
}

