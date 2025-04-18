/*******************************************************************/
/* EXEC_JOB: Interface zur KOBOLD-Kommunikation                    */
/*                                                                 */
/*  Erlaubt auf einfache Weise, Jobs an den KOBOLD zu Åbermitteln. */
/*  Dabei muû der KOBOLD nicht unbedingt als ACC installiert sein, */
/*  sondern kann auch optional als Programm gestartet werden.      */
/*  Diese Library fÅhrt alle nîtigen Operationen durch, damit der  */
/*  KOBOLD auch in Fehler- oder Abbruchsituationen korrekt bedient */
/*  wird.                                                          */
/*                                                                 */
/*  Das Vorgehen ist einfach: Im eigenen Projekt 'EXEC_JOB.LIB'    */
/*  eintragen, im Programm 'EXEC_JOB.H' (diese Datei also) per     */
/*  '#include' einbinden. Wenn ein Job geschickt werden soll, dann */
/*  stellt man diesen zusammen, ruft einmal 'init_kobold' mit den  */
/*  entsprechenden Parametern auf und schickt bei erfolgreicher    */
/*  Initialisierung den Job mittels 'perform_kobold_job' ab.       */
/*  WÑhrend der KOBOLD arbeitet, hat man i.d.R. (auûer bei Pro-    */ 
/*  grammstart unter nicht-Multitasking BS) sofort die Kontrolle   */
/*  zurÅck. In der Haupteventschleife muû man auf den Erhalt der   */
/*  KOBOLD_ANSWER reagieren und bei Erhalt 'kobold_job_done' auf-  */
/*  rufen. Desktops mÅssen *ANSCHLIESSEND* noch die betroffenen    */
/*  Fenster neuzeichnen!                                           */
/*******************************************************************/  

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#if defined (__TURBOC__)
#include <aes.h>
#include <tos.h>
#else
#define __GEMLIB_OLDNAMES /* FIXME later */
#include <gem.h>
#include <osbind.h>
#endif

#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  elif defined(__TURBOC__)
#    define _WORD int
#  else
#    define _WORD short
#  endif
#endif

#define KOBOLD_ANSWER 0x2F12 
/* Antwortnachricht des KOBOLD mit Status in Wort 3. Wird als      */
/* Message-Event erhalten.                                         */


/* _BOOL */ int init_kobold(_WORD own_aes_id, char *path);
/*******************************************************************/
/* Initialisiert die KOBOLD-Schnittstelle fÅr eine bevorstehende   */
/* JobÅbermittlung (Kopieren, Formatieren etc.). Muû vor *jeder*   */
/* gewÅnschten JobausfÅhrung einmal aufgerufen werden.             */
/*                                                                 */
/* PARAMETER:                                                      */
/*   int own_aes_id: Applikation-ID des eigenen Programmes         */
/*   char *path:     Zeiger auf einen Pfad (ohne Programmnamen),   */
/*                   aus dem der KOBOLD 2 alternativ gestartet     */
/*                   wird, wenn er als ACC (oder Applikation in    */
/*                   einer Multitaskingumgebung) nicht verfÅgbar   */  
/*                   ist. Wird dies nicht gewÅnscht, kann man hier */
/*                   auch einen Nullzeiger Åbergeben.              */
/* SEITENEFFEKT:                                                   */
/*   Interne Variablen werden initialisiert.                       */
/* WERT:                                                           */
/*   0: KOBOLD ist weder aktiv noch als startbares Programm ver-   */
/*      fÅgbar                                                     */
/* <>0: KOBOLD ist verfÅgbar, Job kann geschickt werden.           */
/*******************************************************************/


/* _BOOL */ int perform_kobold_job(char *job);
/*******************************************************************/
/* Sendet den Job an den KOBOLD. Der Job wird zuvor in einen neuen */
/* Speicherblock umkopiert, kann also auch als Stackvariable Åber- */
/* geben werden.                                                   */  
/*                                                                 */
/* PARAMETER:                                                      */
/*   char *job: Zeiger auf den auszufÅhrenden JOB. Dieser braucht  */
/*              weder QUIT noch Laufwerksdeselektionskommandos     */
/*              enthalten.                                         */
/* SEITENEFFEKT:                                                   */
/*   Der KOBOLD beginnt im Erfolgsfall mit seiner Arbeit. Auûerdem */
/*   wurde dann ein Speicherbereich (unter MTOS global) alloziert, */
/*   der erst bei Aufruf von 'kobold_job_done' wieder freigegeben  */
/*   wird.                                                         */
/* WERT:                                                           */
/*   0: Job konnte nicht gesendet werden (Speichermangel)          */
/* <>0: Job wurde verschickt                                       */
/*******************************************************************/


void kobold_job_done(int status);
/*******************************************************************/
/* Diese Funktion ist als Reaktion auf den Erhalt einer            */
/* KOBOLD_ANSWER-Message aufzurufen.                               */
/*                                                                 */
/* PARAMETER:                                                      */
/*   int status: Wort 3 (Status) des Nachrichtenpuffers, mit dem   */
/*               die KOBOLD_ANSWER-Message ankam.                  */
/* SEITENEFFEKT:                                                   */
/*   Ein ggf. unter 'perform_kobold' allozierter Puffer wird       */
/*   wieder freigegeben. Der KOBOLD erhÑlt weiterhin evtl. erfor-  */
/*   derliche Abschluûnachrichten.                                 */
/* WERT:                                                           */
/*        ./.                                                      */
/*******************************************************************/
