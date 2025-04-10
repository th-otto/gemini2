/*
 * @(#)Mupfel/loctime.c
 * @(#)Julian F. Reschke & Stefan Eissing, 04. Januar 1992
 * 
 * local time functions
*/

#ifndef M_loctime__
#define M_loctime__

int conv_date (MGLOBAL *M, const char *format, char *to);

size_t print_date (MGLOBAL *M, const char *format);

#endif /* !M_loctime__ */