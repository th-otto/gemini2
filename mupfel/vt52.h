/*
 * @(#) Mupfel\vt52.h 
 * @(#) Gereon Steffens, 29. April 1991
 *
 * vt52 escape sequence definitions
 */

#ifndef __vt52__
#define __vt52__
 
#define cursor_on(M)	vt52(M, 'e')
#define cursor_off(M)	vt52(M, 'f')

#define saveCursor(M)	vt52(M, 'j')
#define restoreCursor(M)	vt52(M, 'k')

#define clearscreen(M)	vt52(M, 'E')
#define cleareol(M)		vt52(M, 'K')
#define cleareos(M)		vt52(M, 'J')

#define cursorhome(M)	vt52(M, 'H')

#define cursor_up(M)	vt52(M, 'A')
#define cursor_down(M)	vt52(M, 'B')
#define cursor_left(M)	vt52(M, 'D')
#define cursor_right(M)	vt52(M, 'C')

#define wrapon(M)		vt52(M, 'v')
#define wrapoff(M)		vt52(M, 'w')

#define reverseon(M)	vt52(M, 'p')
#define reverseoff(M)	vt52(M, 'q')

#define delete_line(M)	vt52(M, 'M')
#define insert_line(M)	vt52(M, 'L')

#define erase_to_line_start(M)	vt52(M, 'o')
#define erase_line(M)	vt52(M, 'l')
#define clear_to_end_of_line(M)	vt52(M, 'K')

#endif /* __vt52__ */