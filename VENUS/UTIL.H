/*
 * @(#) Gemini\util.h
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 */

#ifndef __util__
#define __util__

typedef enum {M_CP, M_MV, M_RM} 
MFunction;

word GotBlitter (void);
word SetBlitter (word mode);

word getScrapIcon (void);
word getTrashIcon (void);

int UpdateSpecialIcons (void);

void PathUpdate (const char *path);
word BuffPathUpdate (const char *path);
void FlushPathUpdate (void);

long scale123 (register long p1,register long p2,register long p3);
word checkPromille (word pm, word def);
void deskAlign (OBJECT *deskobj);
void fulldraw (OBJECT *tree,word objnr);

word escapeKeyPressed (void);
word killEvents (word eventtypes, word maxtimes);
void WaitKeyButton (void);
word ButtonPressed (word *mx, word *my, word *bstate, word *kstate);

/* mx und my sind inout-Parameter, sollten also die aktuelle
 * Mausposition beinhalten!
 */
int ButtonReleased (word *mx, word *my, word *bstate, word *kstate);

void setSelected (OBJECT *tree, word objnr, word flag);
word isSelected (OBJECT *tree, word objnr);

void setHideTree (OBJECT *tree, word objnr, word flag);
word isHidden (OBJECT *tree, word objnr);

void setDisabled (OBJECT *tree, word objnr, word flag);
word isDisabled (OBJECT *tree, word objnr);

word countObjects (OBJECT *tree,word startobj);
void doFullGrowBox (OBJECT *tree,word objnr);
void makeFullName (char *name);
void makeEditName (char *name);

int ObjectWasMoved (const char *oldname,const char *newname, int folder);
#define FileWasMoved(a,b)	(ObjectWasMoved ((a), (b), FALSE))
#define FolderWasMoved(a,b)	(ObjectWasMoved ((a), (b), TRUE))

word getBootPath (char *path,const char *fname);

void setBigClip (word handle);
void remchr (char *str, char c);
word GotGEM14 (void);

unsigned long GetHz200 (void);


int PointInGRECT (word px, word py, const GRECT *rect);

int GRECTIntersect (const GRECT *g1, GRECT *g2);
char VDIRectIntersect (word r1[4], word r2[4]);

/* nur bei Gemini */
int CallMupfelFunction (MFunction f, const char *fpath,
						const char *tpath, word *memfailed);

int SetFileSizeField (TEDINFO *ted, unsigned long size);
long AdjustTextObjectWidth (OBJECT *object, long maxlength);
void SelectFrom2EditFields (OBJECT *tree, int one, int two, int which);

#endif