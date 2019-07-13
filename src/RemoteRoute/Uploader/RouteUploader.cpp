#include "RouteUploader.h"

#include <fstream>
#include <chrono>

#include "Src/MTLib/BkgThread.h"
#include "Src/Network/TcpClientBase.h"
#include "RobotSideManager.h"
#include "RUDispatcher.h"
#include "UploaderClient.h"

namespace DDRCloudService {

extern int g_CurrentRouteVersion;
int g_MaxDisRetrials = 5;

struct RouteUploaderARG
{
	std::string rID;
	std::string routeName;
	std::string serAddr;
	std::string serPort;
	bool bForceUpdate;
	RouteUploaderARG(const char *_rid, const char *_rname,
		             const char *_addr, const char *_port,
		             bool _bForce) :
		rID(_rid), routeName(_rname), serAddr(_addr), serPort(_port), bForceUpdate(_bForce)
	{}
};

static void _thrFunc(void *ptr, bool *pbQuit)
{
	std::shared_ptr<RouteUploaderARG> pArg((RouteUploaderARG*)ptr);
	auto pClt = std::make_shared<RUTcpClient>(pArg->rID, pArg->routeName,
		                                      pArg->serAddr, pArg->serPort,
		                                      pArg->bForceUpdate);
	if (pClt->IsRSRMWrong()) {
		return;
	}
	pClt->Start(1);
	pClt->Connect(pArg->serAddr, pArg->serPort);

	while (!pbQuit || !(*pbQuit)) {
		if (pClt->IsRSRMWrong() || pClt->GetSecondsSinceLastRcv() > 15) {
			pClt->Try2reconnect();
			break;
		}
		if (pClt->IsStopped()) {
			break;
		}
		if (pClt->IsUploadingFinished()) {
			LevelLog(DDRFramework::Log::INFO, "SUCCESSFULLY uploaded the route (%s, %s)!\n", pArg->rID, pArg->routeName);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	pClt->Stop();
}

unsigned int StartUploadingRoute(const char *robotID,
	                             const char *routeName,
	                             const char *serverAddr,
	                             const char *serverPort,
	                             bool bForceUpdate)
{
	auto arg = new RouteUploaderARG(robotID, routeName, serverAddr, serverPort, bForceUpdate);
	unsigned int id;
	if (DDRMTLib::CreateBkgThread(_thrFunc, arg, &id)) {
		return id;
	} else {
		return -1;
	}
}

}