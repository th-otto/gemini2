/*
	@(#)FlyDial/fdlight.c
	Julian F. Reschke, 6. Mai 1995
	
	'Light'-Version fÅr MagiC 3
*/

#include <aes.h>
#include <tos.h>

typedef struct
{
	OBJECT	*tree;
	void	*buffer;
	int		x, y, w, h;
} DIALINFO;

int appl_xgetinfo (int type, int *out1, int *out2, int *out3, int *out4);
int DialStart (OBJECT *TheTree, DIALINFO *D);
void DialDraw (DIALINFO *D);
void DialEnd (DIALINFO *D);
int DialDo (DIALINFO *dip, int *startob);
