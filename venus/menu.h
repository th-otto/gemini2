/*
 * @(#) Gemini\menu.h
 * @(#) Stefan Eissing, 18. September 1991
 *
 * description: Header File for menu.c
 */


#ifndef G_MENU__
#define G_MENU__

void SetShowInfo (void);

void SetMenuBar (word install);

#define InstallMenuBar()		(SetMenuBar (TRUE))
#define RemoveMenuBar()		(SetMenuBar (FALSE))

int DoMenu (word mtitle, word mentry, word kstate);

void ManageMenu (void);

#endif /* !G_MENU__ */