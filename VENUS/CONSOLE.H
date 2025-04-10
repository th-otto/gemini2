/*
 * @(#) Gemini\console.h
 * @(#) Stefan Eissing, 17. September 1991
 *
 */


#ifndef G_console__
#define G_console__

extern int ConsoleIsOpen;

void ConsoleDispCanChar (char ch);
void ConsoleDispRawChar (char ch);

void ConsoleDispString (char *str);

#endif	/* !G_console__ */