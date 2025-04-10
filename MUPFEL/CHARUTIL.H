/*
 * @(#) MParse\CharUtil.h
 * @(#) Stefan Eissing, 13. September 1991
 *
 */


#ifndef __CharUtil__
#define __CharUtil__
 
int IsLetter (int c);
int IsValidDollarChar (int c);
int IsEnvChar (int c);

char *StrToken (char *string, const char *white, char **save);

char *StrDup (ALLOCINFO *A, const char *str);
char lastchr (const char *str);
void chrcat (char *str, char c);

void ConvertSlashes (char *string, const char *quoted);

int ContainsWildcard (const char *string, const char *quoted);
int ContainsGEMDOSWildcard (const char *string);

void RemoveDoubleBackslash (char *str);
void RemoveAppendingBackslash (char *str);

#endif /* __CharUtil__ */