/*
 * @(#) Mupfel\mupfel.h
 * @(#) Stefan Eissing, 04. Juli 1993
 *
 * jr 26.5.1996
 */


#ifndef M_MUPFEL__
#define M_MUPFEL__

#ifndef M_GLOB__


int PutEnv (void *M, const char *name);
void CreateAutoVar (void *M, const char *name, size_t size);
char *GetEnv (void *M, const char *name);

#define PARM_NO		0
#define PARM_YES	1
#define PARM_EGAL	2

int StartGeminiProgram (void *MupfelInfo, const char *full_name, 
	char *command, int show_path, int gem_start, 
	int not_in_window, int wait_key, int overlay, int single_mode,
	int parallel);
	
int DoingOverlay (void *MupfelInfo);


int FormatDrive (void *MupfelInfo, const char *command, int drv,
	int sides, int sectors, int verbose, char *label,
	void (*progress)(int));

int m_init (void *MupfelInfo, int argc, char **argv);
int SetLabel (void *MupfelInfo, char *path, char *newlabel, 
	const char *cmd);

void InstallErrorFunction (void *MupfelInfo, void (*f)(const char *));
void DeinstallErrorFunction (void *MupfelInfo);

void SetRowsAndColumns (void *MupfelInfo, int rows, int cols);

extern SYSHDR *SysBase;
extern unsigned long MiNTVersion;
extern unsigned short GEMDOSVersion, MagiCVersion;
extern unsigned short TOSVersion, TOSDate, TOSCountry;

#endif /* !M_GLOB__ */

#endif /* !M_MUPFEL__ */
