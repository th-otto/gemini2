/*
 * @(#) Gemini\userdefs.h
 * @(#) Stefan Eissing, 28. Februar 1993
 *
 * description: Header File for iconinst.c
 */


#ifndef G_USERDEFS__
#define G_USERDEFS__


#ifndef G_CICON
#define G_CICON         33      /* Farbicon-Objekttyp */

typedef struct cicon_data
{
	word num_planes;				/* number of planes in the following data          */
	word *col_data;					/* pointer to color bitmap in standard form        */
	word *col_mask;					/* pointer to single plane mask of col_data        */
	word *sel_data;					/* pointer to color bitmap of selected icon        */
	word *sel_mask;					/* pointer to single plane mask of selected icon   */
	struct cicon_data *next_res;	/* pointer to next icon for a different resolution */
}	CICON;

typedef struct cicon_blk
{
	ICONBLK monoblk;              /* default monochrome icon                         */
	CICON *mainlist;              /* list of color icons for different resolutions   */
}	CICONBLK;
#endif



typedef struct
{
	unsigned is_folder : 1;
	unsigned is_icon : 1;
	unsigned noObject : 1;		/* ob das Object gefÅllt ist */
	unsigned is_symlink : 1;
} FileFlags;

typedef struct
{
	FileFlags flags;
	ICONBLK icon;
	CICON *mainlist;
	word x;
	word y;
	char *string;				/* String fÅr Textanzeige */
	uword freelength;			/* jr: LÑnge des Namens oder Null, falls 8+3 */
} ICON_OBJECT;


#endif /* !G_USERDEFS__ */