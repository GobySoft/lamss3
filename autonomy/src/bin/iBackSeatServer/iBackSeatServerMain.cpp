#include "MOOSLIB/MOOSLib.h"
#include "MOOSGenLib/MOOSGenLib.h"
#include "iBackSeatServer.h"

int main(int argc, char *argv[])
{
	const char *sMissionFile = "iBackSeatServer.moos";
	
	if(argc > 1) {
		sMissionFile = argv[1];
	}
	
	CiBackSeatServer iBackSeatServer;
	
	iBackSeatServer.Run("iBackSeatServer", sMissionFile);

	return 0;
}

