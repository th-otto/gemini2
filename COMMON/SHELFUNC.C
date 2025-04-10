#include <aes.h>

static AESPB mypb =
{
    &(_GemParBlk.contrl[0]),
    &(_GemParBlk.global[0]),
    &(_GemParBlk.intin[0]),
    &(_GemParBlk.intout[0]),
    &(((int *)_GemParBlk.addrin)[0]),
    &(((int *)_GemParBlk.addrout)[0]),
};


int shel_wdef (char *cmd, char *dir)
{
	_GemParBlk.addrin[0] = (int *)cmd;
	_GemParBlk.addrin[1] = (int *)dir;
	mypb.contrl[0] = 127;
	mypb.contrl[1] = 0;
	mypb.contrl[2] = 0;
	mypb.contrl[3] = 2;
	mypb.contrl[4] = 0;
	
	_crystal (&mypb);
	
	return mypb.intout[1];
}
