/*
 * @(#) Common\xbra.c
 * @(#) Stefan Eissing, 17. September 1991
 *
 * installiere OS-Vektoren, etc.
 */

#include <stddef.h> 
#include <string.h>
#include <tos.h>

#include "xbra.h"






void InstallXBRA (XBRA *x, long *vector)
{
	long old_stack;
	
	old_stack = Super (NULL);

	x->old_vec = *vector;
	*vector = (long)&x->routine;

	Super ((void *)old_stack);
}


void RemoveXBRA (XBRA *x, long *vector)
{
	long old_stack;
	int done = 0;
	
	old_stack = Super (NULL);

	while (!done)
	{
		/* Wenn vector auf unsere eigene Routine zeigt, dann
		 * setze x->old_vec in vector ein. Damit sind wir
		 * aus der Kette ausgeh„ngt.
		 */
		if (*vector == (long)&x->routine)
		{
			*vector = (long)x->old_vec;
			done = 1;
		}
		else
		{
			XBRA *tmp;
			
			tmp = (XBRA *)((*vector) - offsetof (XBRA, routine));
			
			if (*vector && tmp->magic == 'XBRA')
			{
				/* Es h„ngt eine andere XBRA-Struktur an dem
				 * Vektor. Betrachte tmp->old_vec als neuen
				 * Vektor, aus dem wir uns austragen mssen.
				 */
				vector = (long *)&tmp->old_vec;
			}
			else
			{
				/* Es ist keine XBRA-Struktur mehr vorhanden,
				 * weil sich jemand ohne XBRA in den Vektor
				 * geh„ngt hat. Entferne ihn, indem vector auf
				 * unseren alten Wert (x->old_vec) gesetzt wird.
				 */
				*vector = x->old_vec;
				done = 1;
			}
		}
	}

	Super ((void *)old_stack);
}

