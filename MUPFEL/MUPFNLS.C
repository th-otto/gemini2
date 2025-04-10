/* mupfel.msg translation into C code
 * done by D:\pure-c\gmni\nls\nls2c.ttp
 * at Fri May 02 13:56:20 1997
 */

#include <stddef.h>

#define NLS_DEFAULT
#include <nls\nls.h>

typedef struct sec
{
	char *SectionTitel;		/* Offset des Section-Strings im File */
	char *SectionStrings;	/* Offset des ersten Strings dieser
							 * Section im File die weiteren Strings
							 * liegen direkt dahinter */
	long StringCount;		/* Anzahl der Strings */
	struct sec *NextSection;/* Offset der nÑchsten Section im File
							 * 0L ist gleich Ende */
} BINSECTION;

static BINSECTION internalSections[] =
{
    { "M.mupfel", 
        "%s: kann Variablen nicht setzen\x0A\x00"
        "[3][|Das AES scheint mich nicht zu mîgen!][Abbruch]\x00"
        "%s: Fehler beim Suchen nach %s\x0A\x00"
        "1.a2·\x00",
    4, internalSections+1 },
    { "M.alias", 
        "[Name[=Kommando]]...\x00"
        "Namen mit Kommando versehen\x00"
        "-a | Name...\x00"
        "Namen lîschen\x00",
    4, internalSections+2 },
    { "M.backup", 
        "[-icv] dateien\x00"
        "Dateien kopieren, Neue Extension ist .bak\x00"
        "%s: %s ist kein gÅltiger Name.\x0A\x00"
        "%s: nicht genÅgend Speicher\x0A\x00"
        "%s: kann Verzeichnis %s nicht sichern\x0A\x00",
    5, internalSections+3 },
    { "M.chario", 
        "%s%sI/O-Fehler %ld%s%s\x0A\x00"
        " beim Zugriff auf \x00",
    2, internalSections+4 },
    { "M.chmod", 
        "[-cv][[+-][ahsw]] dateien\x00"
        "Attribute setzen/lîschen\x00"
        "%s ist ein Verzeichnis\x00"
        "chmod: kann Attribute von %s nicht lesen\x00"
        "chmod: kann Attribute von %s nicht setzen\x0A\x00"
        "%s: ungÅltige Option %c\x0A\x00"
        "%s: ungÅltige Option %s\x0A\x00",
    7, internalSections+5 },
    { "M.cookie", 
        "[-flsv]\x00"
        "Cookie-Jar anzeigen\x00"
        "Cookie  Wert\x0A\x00"
        "Cookie  Hex       Wert\x0A\x00"
        "%s: Cookie Jar nicht vorhanden\x0A\x00"
        "DIP-Schalter\x00"
        "Diablo-Emulator\x00"
        "GEMDOS File Locking verfÅgbar\x00"
        "Netzwerk\x00"
        "Version\x00"
        "Es ist Platz fÅr insgesamt %ld EintrÑge (%ld belegt)\x0A\x00"
        "Landessprache: %s, Tastatur: %s\x00"
        "kein Atari\x00",
    13, internalSections+6 },
    { "M.cpmv", 
        "[-adfinprRsv] f1 f2  oder  cp f1..fn dir\x00"
        "Dateien kopieren\x00"
        "[-fiv] f1 f2 oder mv f1..fn dir oder mv dir1 dir2\x00"
        "dateien umbenennen/verschieben\x00"
        "%s: ACHTUNG: -a funktioniert erst ab TOS 1.04 (GEMDOS 0.15) richtig\x0A\x00"
        "%s: `%s\' ist kein Verzeichnis\x0A\x00"
        "%s: kann `%s\' nicht in `%s\' umbenennen (%s)\x0A\x00"
        "%s: Benutzen Sie -r um Verzeichnisse zu kopieren\x0A\x00"
        "%s: `%s\' und `%s\' sind dieselbe Datei\x0A\x00"
        "%s: kann `%s\' nicht anlegen (%s)\x0A\x00"
        "fertig.\x0A\x00"
        "%s: `%s\' ist kein gÅltiger Name.\x0A\x00"
        "%s: Datei `%s\' existiert nicht\x0A\x00"
        "%s: `%s\' ist schreibgeschÅtzt\x0A\x00"
        "%s: `%s\' als Quelle und `%s\' als Ziel fÅhrt zu einer Endlosschleife\x0A\x00"
        "%s: `%s\' enthÑlt Wildcards\x0A\x00"
        "benenne `%s\' in `%s\' um...\x00",
    17, internalSections+7 },
    { "M.cpmvutil", 
        "%s: %s existiert. öberschreiben (j/n/a/q)\? \x00"
        "JNAQ\x00"
        "%s: %s nach %s kopieren (j/n/a/q)\? \x00"
        "%s: %s nach %s verschieben (j/n/a/q)\? \x00"
        "%s %s nach %s... \x00"
        "kopiere\x00"
        "verschiebe\x00"
        "%s: kann %s nicht Åberschreiben\x0A\x00"
        " fertig.\x0A%s wird in %s umbenannt... \x00"
        "%s: Ziel-Laufwerk ist voll\x0A\x00"
        "%s: Schreibfehler in `%s\'\x0A\x00"
        "%s: Dateidatum fÅr `%s\' nicht kopiert\x0A\x00",
    12, internalSections+8 },
    { "M.date", 
        "UngÅltige Datumsangabe\x0A\x00"
        "Neues Datum: %s\x00"
        "UngÅltige Datumsangabe\x00"
        "%s ist ein Verzeichnis\x00"
        "kann %s nicht anlegen\x00"
        "%s existiert nicht\x00"
        "kann Datum fÅr %s nicht setzen\x00"
        "%s ist kein gÅltiger Name\x00"
        "[hhmm|mmddhhmm[yy]]\x00"
        "Systemdatum anzeigen/Ñndern\x00"
        "[-c][-d hhmm|mmddhhmm[yy]] dateien\x00"
        "Modifikationsdatum Ñndern\x00",
    12, internalSections+9 },
    { "M.exec", 
        "Zu viele Argumente, xArg/ARGV wird benutzt\x0A\x00"
        "Kann nicht ins Verzeichnis `%s\' wechseln\x0A\x00"
        "Oh!\x00"
        "DrÅcken Sie RETURN...\x00"
        "`%s\' ist abgestÅrzt! Weitermachen kann gefÑhrlich sein.\x00"
        "Weiter\x00"
        "Neustart\x00"
        "Kann `runner2.app\' nicht finden\x0A\x00"
        "`%s\' ist nicht vorhanden. Bitte kopieren Sie `runner2.app\' dorthin.\x0A\x00"
        "Konnte keine temporÑre Datei fÅr die ParameterÅbergabe anlegen.\x0A\x00"
        "Kann Programm nicht im Single-Modus starten!\x0A\x00"
        "GEM-Programme kînnen nicht gestartet werden!\x0A\x00",
    12, internalSections+10 },
    { "M.exectree", 
        "Nicht genug freier Speicher vorhanden\x0A\x00"
        "Fehler beim Suchen von %s\x0A\x00"
        "%s nicht gefunden\x0A\x00"
        "Fehler beim Setzen der Parameter\x0A\x00"
        "%s ist keine Programmdatei\x0A\x00"
        "Konnte Variablen nicht setzen\x0A\x00"
        "Aufruf zu tief verschachtelt\x0A\x00",
    7, internalSections+11 },
    { "M.fkey", 
        "[taste [text]]\x00"
        "Funktionstasten setzen/lîschen/anzeigen\x00"
        "UngÅltige Funktionstaste\x00",
    3, internalSections+12 },
    { "M.format", 
        "[-vy][-s seiten][-c sektoren][-l label] drv:\x00"
        "Diskette formatieren\x00"
        "%s: nicht genug Speicher vorhanden\x0A\x00"
        "Formatiere Spur %02d Seite %d\x00"
        "%s: Fehler beim Formatieren von Spur %d (%s)\x0A\x00"
        "%s: abgebrochen\x0A\x00"
        "%s: nur 9 bis 20 Sektoren erlaubt\x0A\x00"
        "%s: kann Laufwerk `%s\' nicht formatieren\x0A\x00"
        "%s: `%s\' ist kein Laufwerksbezeichner\x0A\x00"
        "Diskette in Laufwerk %c: wirklich formatieren (j/n)\? \x00"
        "JN\x00"
        "%s: Laufwerk %c: ist in Gebrauch\x0A\x00",
    12, internalSections+13 },
    { "M.free", 
        "[-l]\x00"
        "freien Speicher anzeigen\x00"
        "%s: zu viele Speicherblîcke\x0A\x00"
        "Block     Adresse      Grîûe\x0A\x00"
        "%lu Bytes (%luK) in %d Blîcken frei, grîûter Block: %lu Bytes (%luK)\x0A\x00",
    5, internalSections+14 },
    { "M.getopt", 
        "%s: Option \'%s\' ist mehrdeutig\x0A\x00"
        "%s: Option \'%c%s\' darf kein Argument haben\x0A\x00"
        "%s: Option \'%s\' benîtigt ein Argument\x0A\x00"
        "%s: Option \'-%c\' benîtigt ein Argument\x0A\x00"
        "%s: Unbekannte Option \'%c%s\'\x0A\x00"
        "%s: Unbekannte Option, code=0%o\x0A\x00"
        "%s: Unbekannte Option \'-%c\'\x0A\x00"
        "%s: Nicht genug Speicher\x00",
    8, internalSections+15 },
    { "M.hash", 
        "[ -r ] [ name ... ]\x00"
        "Hashtabelle lîschen, anzeigen, name eintragen\x00"
        "Treffer Aufwand  Kommando       Voller Pfad\x0A\x00"
        "%7d %7d  %-15s%s\x0A\x00"
        "%s: Fehler beim Suchen von %s\x0A\x00"
        "%s: %s nicht gefunden\x0A\x00"
        "%s: %s ist eine Funktion\x0A\x00"
        "%s: %s ist ein internes Kommando\x0A\x00"
        "%s: %s ist nicht ausfÅhrbar\x0A\x00"
        "%s: %s kann nicht gehasht werden\x0A\x00",
    10, internalSections+16 },
    { "M.history", 
        "[-s nummer] [-a|-n] [[-r|-w] [datei]]\x00"
        "History anzeigen, lesen, schreiben\x00"
        "TBD\x00"
        "Kommandoliste bearbeiten\x00"
        "%s: kann Grîûe %d nicht setzen\x0A\x00",
    5, internalSections+17 },
    { "M.init", 
        "[-y][-s seiten][-c sektoren][-l label] drv:\x00"
        "Lîschen einer Diskette\x00"
        "%s: kann Boot-Sektor auf Laufwerk %c: nicht schreiben\x0A\x00"
        "%s: Sektorzahl nicht erlaubt\x0A\x00"
        "%s: ungÅltiger Laufwerksname `%s\'\x0A\x00"
        "%s: %s ist kein Laufwerksbezeichner\x0A\x00"
        "Diskette in Laufwerk %c: wirklich initialisieren (j/n)\? \x00"
        "JN\x00"
        "%s: kann Boot-Sektor von Laufwerk %c: nicht lesen (%s)\x0A\x00"
        "%s: Unbekannter Diskettentyp, bitte Sektoren und Seiten angeben.\x0A\x00"
        "%s: Laufwerk `%c:\' ist in Gebrauch (%s)\x0A\"\x00"
        "%s: Laufwerk `%c:\' wird von `%s\' (Prozess %d) benutzt\x0A\"\x00",
    12, internalSections+18 },
    { "M.internal", 
        "[cmd]\x00"
        "Liste aller Kommandos oder Hilfe zu cmd\x00"
        "cmd\x00"
        "internes Kommando cmd ausfÅhren\x00"
        "[-vV] cmd [ arg... ]\x00"
        "Kommando (nicht Funktion) cmd ausfÅhren bzw. Info Åber cmd anzeigen\x0A\x00"
        "Maske\x00"
        "Zugriffsrechte bei erzeugten Dateien festlegen\x00"
        "[ cmd ... ]\x00"
        "interne Kommandos cmd einschalten/anzeigen\x00"
        "interne Kommandos cmd abschalten/anzeigen\x00"
        "%s: %s ist kein eingebautes Mupfel-Kommando\x0A\x00"
        "Liste der internen Kommandos (mehr mit: help Kommando)\x0A\x00"
        "%s: nicht genug Speicher vorhanden\x0A\x00",
    14, internalSections+19 },
    { "M.label", 
        "[drive [label]]\x00"
        "Label anzeigen/Ñndern\x00"
        "kein Label\x00"
        "%s: `%s\' existiert bereits\x0A\x00",
    4, internalSections+20 },
    { "M.lineedit", 
        "Wollen Sie alle %d Mîglichkeiten sehen\? (j/n) \x00"
        "jJ\x00",
    2, internalSections+21 },
    { "M.locate", 
        "Kommandos mit Pfadangabe sind nicht erlaubt\x0A\x00",
    1, internalSections+22 },
    { "M.ls", 
        "[-acdiklmpqrstuxABCDFHLMRSUX1][-w zeichen][-I muster]\x00"
        "Dateien anzeigen\x00"
        "%s: `%s\': %s\x0A\x00"
        "falsche Breitenangabe\x00"
        "%s: nicht genug Speicher fÅr %s\x0A\x00",
    5, internalSections+23 },
    { "M.mcd", 
        "[dir]\x00"
        "akt. Verzeichnis in dir oder $HOME wechseln\x00"
        "Directory-Stack anzeigen\x00"
        "[dir]\x00"
        "dir bzw . auf Directory-Stack speichern\x00"
        "cd ins letzte gemerkte Verzeichnis\x00"
        "aktuelles Verzeichnis ausgeben\x00"
        "%s: kein Argument ist ein Verzeichnis\x0A\x00"
        "%s ist nicht erlaubt\x0A\x00"
        "%s: Argumente sind nicht erlaubt\x0A\x00"
        "%s: kein HOME-Verzeichnis gesetzt\x0A\x00"
        "%s: leeres Argument nicht erlaubt\x0A\x00"
        "%s: Fehler beim Setzen von %s\x0A\x00"
        "%s: nicht genug freier Speicher vorhanden\x0A\x00"
        "%s: Verzeichnis %s nicht gefunden\x0A\x00"
        "%c: ist kein gÅltiges Laufwerk\x0A\x00"
        "%s: %s ist kein Verzeichnis\x0A\x00"
        "%s: es ist kein Verzeichnis gepusht\x0A\x00"
        "%s: nicht genug freier Speicher\x0A\x00",
    19, internalSections+24 },
    { "M.mlex", 
        "Fehler beim Schreiben von %s\x0A\x00",
    1, internalSections+25 },
    { "M.mglobal", 
        "Gebrauch: mupfel [[+-][acefhiklnrstuvxSCG]] [parameter]\x0A\x00",
    1, internalSections+26 },
    { "M.mkdir", 
        "mkdir: %s existiert bereits\x0A\x00"
        "mkdir: %s ist kein gÅltiger Name\x0A\x00"
        "[-p] dirs\x00"
        "Directories anlegen\x00",
    4, internalSections+27 },
    { "M.mparse", 
        "Nicht genug freier Speicher\x0A\x00"
        "Unerwartetes Ende der Datei\x00"
        ", ich suchte nach `%c\'\x0A\x00"
        "Syntax Fehler: \x00"
        "Syntax Fehler in Zeile %ld: \x00"
        "das geht hier so nicht\x0A\x00"
        "ich hatte eigentlich `)\' oder `|\' erwartet\x0A\x00"
        "ich hatte ein (reserviertes) Kommando erwartet\x0A\x00"
        "ich hatte `;\' oder eine neue Zeile erwartet\x0A\x00"
        "hier muû ein `)\' hin\x0A\x00"
        "hier fehlt ein Kommando\x0A\x00"
        "Fehler in der Kommandosubstitution\x0A\x00",
    12, internalSections+28 },
    { "M.mset", 
        "[[+-][aefhklnrtuvxCS%\\^]] wert\x00"
        "Interne Variablen und Shell-Flags setzen/lîschen\x00"
        "%s: ungÅltige Option %s\x0A\x00"
        "%s: Option o ohne Parameter\x0A\x00"
        "%s: -c ohne Parameter\x0A\x00"
        "%s: -o ohne gÅltigen Optionsnamen\x0A\x00",
    6, internalSections+29 },
    { "M.names", 
        "nicht genug Speicher, um %s zu setzen\x0A\x00"
        "Variable %s ist schreibgeschÅtzt\x0A\x00"
        "UngÅltiger Variablenname %s\x0A\x00",
    3, internalSections+30 },
    { "M.nameutil", 
        "[ -p | name[=value] ... ]\x00"
        "Variable fÅr Export auf value setzen\x00"
        "[ name [value] ]\x00"
        "Variable fÅr Export auf value setzen\x00"
        "[ -p | name[=value] ... ]\x00"
        "Variable mit Schreibschutz versehen\x00"
        "[ name ... ]\x00"
        "Variable/Funktionen entfernen\x00",
    8, internalSections+31 },
    { "M.read", 
        "[ name ... ]\x00"
        "Werte fÅr name einlesen\x00",
    2, internalSections+32 },
    { "M.redirect", 
        "Konnte `%s\' nicht îffnen (%s)\x0A\x00"
        "Handle %s kann nicht umgeleitet werden\x0A\x00"
        "Dies ist eine eingeschrÑnkte Shell\x0A\x00"
        "Umlenkung von %d auf %d fehlgeschlagen (%s)\x0A\x00"
        "%s existiert bereits\x0A\x00",
    5, internalSections+33 },
    { "M.rename", 
        "[-icv] ext dateien\x00"
        "dateien mit Endung ext versehen\x00"
        "%s: die Endung `%s\' enthÑlt nicht erlaubte Zeichen\x0A\x00"
        "%s: `%s\' ist kein gÅltiger Name.\x0A\x00"
        "%s: kann Verzeichnis `%s\' nicht umbenennen\x0A\x00"
        "%s: Datei `%s\' existiert nicht\x0A\x00"
        "%s: `%s\' existiert bereits\x0A\x00",
    7, internalSections+34 },
    { "M.rm", 
        "[-bfirRv] datei...\x00"
        "Dateien lîschen\x00"
        "%s: `%s\' ist kein gÅltiger Name\x0A\x00"
        " fertig.\x0A\x00"
        "JNAQ\x00"
        "Lîsche `%s\'...\x00"
        "rm: `%s\' ist ein Verzeichnis\x0A\x00"
        "`%s\' wirklich lîschen (j/n/a/q)\? \x00"
        "`%s\' rekursiv durchgehen (j/n/a/q)\? \x00",
    9, internalSections+35 },
    { "M.rmdir", 
        "dir ...\x00"
        "Verzeichnis dir lîschen\x00"
        "Lîsche Verzeichnis `%s\'...\x00"
        "%s: `%s\' ist kein Verzeichnis\x0A\x00"
        "%s: `%s\' enthÑlt noch Dateien\x0A\x00"
        " fertig.\x0A\x00",
    6, internalSections+36 },
    { "M.shelfork", 
        "kann keinen neuen Prozeû erzeugen\x0A\x00"
        "Nicht genug freier Speicher vorhanden\x0A\x00",
    2, internalSections+37 },
    { "M.shrink", 
        "[-b val][-t val][-f][-i]\x00"
        "Speicher verkleinern/freigeben\x00"
        "%s: Fehler bei Mfree()\x0A\x00"
        "%s: kann %lu Bytes nicht allozieren\x0A\x00"
        "%s: kann nicht um mehr verkleinern als frei ist!\x0A\x00"
        "%s: kaufen Sie eine RAM-Erweiterung!\x0A\x00"
        "Speicher ist nicht verkleinert\x00"
        "Speicher ist um %lu Bytes (%luK) verkleinert\x00"
        ", grîûter freier Block: %lu Bytes (%luK)\x0A\x00"
        "%s: unerlaubte Grîûenangabe\x0A\x00"
        "%s: Argument nicht numerisch oder Null\x0A\x00"
        "%s: nur eins von -b, -t, -f oder -i erlaubt\x0A\x00",
    12, internalSections+38 },
    { "M.smallcmd", 
        "%s: Nicht genug Speicher\x0A\x00"
        "zeichenkette [suffix]\x00"
        "Dateiname aus vollem Pfad extrahieren\x00"
        "[pfadname]\x00"
        "Ordnername aus vollem Pfad extrahieren\x00"
        "zeichenkette\x00"
        "Zeichenkette als Befehl ausfÅhren\x00"
        "name\x00"
        "Kommandos von Datei name lesen\x00"
        "%s: kann %s nicht îffnen\x0A\x00"
        "%s: %s ist kein Shell-Script\x0A\x00"
        "Exit-Status auf falsch(1) setzen\x00"
        "legale-Optionen [Optionen]\x00"
        "Optionen testen und auflîsen\x00"
        "Exit-Status auf wahr(0) setzen\x00"
        "leeres Kommando\x00"
        "[ n ]\x00"
        "Beende n for/while Schleifen\x00"
        "[ n ]\x00"
        "Setze die n-te for/while Schleife fort\x00"
        "[ n ]\x00"
        "Verlasse Funktion mit Returnwert n\x00"
        "return ist nur in einer Funktion erlaubt!\x0A\x00"
        "[ n ]\x00"
        "Shell mit Returnwert n verlassen\x00"
        "[zahl]\x00"
        "interne Variablen um <zahl> oder 1 verschieben\x00"
        "[args]\x00"
        "Argumente ausgeben\x00",
    29, internalSections+39 },
    { "M.test", 
        "ausdruck\x00"
        "ausdruck ]\x00"
        "Bedingung testen\x00"
        "Argument fehlt\x0A\x00"
        "Argument erwartet, %s gefunden\x0A\x00"
        "%s `%s\' wird ein ganzzahliger Ausdruck erwartet\x0A\x00"
        "vor\x00"
        "nach\x00"
        "Zeichen `%s\' fehlt\x0A\x00"
        "%s: zu viele Argumente\x0A\x00",
    10, internalSections+40 },
    { "M.times", 
        "abgelaufene Zeit anzeigen\x00"
        "Abgelaufene Zeit: %02ld:%02ld:%02ld.%ld%ld\x0A\x00"
        "       User-Zeit: %02ld:%02ld:%02ld.%ld%ld\x0A\x00"
        "     System-Zeit: %02ld:%02ld:%02ld.%ld%ld\x0A\x00",
    4, internalSections+41 },
    { "M.trap", 
        "[ Befehle ] Name ...\x00"
        "Signal Name mit Befehl belegen/ignorieren\x00"
        "%s: nicht genug Speicher\x0A\x00"
        "%s: ungÅltiges Signal %s\x0A\x00",
    4, internalSections+42 },
    { "M.type", 
        "name\x00"
        "Art des Kommandos name bestimmen\x00"
        "%s ist ein Alias auf %s\x0A\x00"
        "%s: Fehler beim Suchen nach %s\x0A\x00"
        "%s ist als Kommando nicht bekannt\x0A\x00"
        "%s ist eine Funktion\x0A\x00"
        "%s ist ein internes Kommando\x0A\x00"
        "%s ist eine Datei (%s)\x0A\x00"
        "%s ist ein %sScript (%s) fÅr %s\x0A\x00"
        "%s ist ein %sScript (%s)\x0A\x00"
        "%s ist ein %sProgramm (%s)\x0A\x00"
        "gehashtes \x00",
    12, internalSections+43 },
    { "M.version", 
        "[-amgdth]\x00"
        "Versionsnummern anzeigen\x00"
        "Version\x00"
        "Festplattentreiber kompatibel zu AHDI-Version %d.%02d\x0A\x00"
        "Kein AHDI-kompatibler Festplattentreiber installiert.\x0A\x00"
        "%s: diese Mupfel benutzt kein GEM\x0A\x00"
        "Keine TOS-Version\x0A\x00",
    7, internalSections+44 },
    { "", "\x00\x00", 0, NULL }
};

BINSECTION *NlsDefaultSection = internalSections;

