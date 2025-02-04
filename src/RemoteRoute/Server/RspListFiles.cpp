#include "ServerSideManager.h"

#include "../../Shared/thirdparty/zlib/zlib.h"
#include "Src/Utility/DDRMacro.h"
#include "Src/MTLib/GeneralLockGuard.hpp"

namespace DDRCloudService {

std::shared_ptr<google::protobuf::Message> ServerSideRouteManager::_rspListFiles(google::protobuf::Message *pMsg)
{
	if (pMsg->GetTypeName() != "DDRCommProto.reqListFiles_uploader" &&
		pMsg->GetTypeName() != "DDRCommProto.reqListFiles_downloader") {
		return std::shared_ptr<google::protobuf::Message>();
	}
	auto ret = std::make_shared<RemoteRouteProto::rspListFiles>();
	ret->set_ret(RemoteRouteProto::eReqVoid);
	u64 reqID;
	if (pMsg->GetTypeName() == "DDRCommProto.reqListFiles_uploader") {
		reqID = ((RemoteRouteProto::reqListFiles_uploader*)pMsg)->reqguid();
	} else {
		reqID = ((RemoteRouteProto::reqListFiles_downloader*)pMsg)->reqguid();
	}
	ret->set_reqguid(reqID);

	DDRMTLib::_lock_guard lg(true, m_gLoc, 50);
	if (!lg) {
		ret->set_ret(RemoteRouteProto::eTooBusy);
		return ret;
	}

	int ind = -1;
	if (pMsg->GetTypeName() == "DDRCommProto.reqListFiles_uploader") {
		auto pp = m_UIDMapper.find(((RemoteRouteProto::reqListFiles_uploader*)pMsg)->uploadid());
		if (m_UIDMapper.end() != pp) {
			ind = (int)pp->second;
			LevelLog(DDRFramework::Log::Level::INFO, "reqListFiles(uid=%llu)",
				     ((RemoteRouteProto::reqListFiles_uploader*)pMsg)->uploadid());
		}
	} else {
		auto pp = m_DIDMapper.find(((RemoteRouteProto::reqListFiles_downloader*)pMsg)->downloadid());
		if (m_DIDMapper.end() != pp) {
			ind = (int)pp->second;
			LevelLog(DDRFramework::Log::Level::INFO, "reqListFiles(did=%llu)",
				     ((RemoteRouteProto::reqListFiles_downloader*)pMsg)->downloadid());
		}
	}
	if (-1 == ind) {
		ret->set_ret(RemoteRouteProto::eIDInvalid);
		return ret;
	}

	auto &ele = m_routes[ind];
	if (!lg.lock(true, ele->loc, 500)) {
		ret->set_ret(RemoteRouteProto::eTooBusy);
		return ret;
	}

	if (ele->files.empty()) {
		ret->set_filerecords_sz(0);
		ret->set_ret(RemoteRouteProto::eOkay);
		return ret;
	}

	RemoteRouteProto::MultipleFileInfo mfi;
	for (auto &x : ele->files) {
		auto pNewFile = mfi.add_records();
		pNewFile->set_name(x.fn);
		pNewFile->set_sz(x.sz);
		pNewFile->set_curpos(x.pos);
		pNewFile->set_mtime(x.mtime);
	}
	std::vector<char> tmpBuf(mfi.ByteSize());
	if (!mfi.SerializeToArray(&tmpBuf[0], (int)tmpBuf.size())) {
		ret->set_ret(RemoteRouteProto::eInternalErr);
		return ret;
	}
	ret->set_filerecords_sz(tmpBuf.size());

	bool bDataOrg = false;
	ret->mutable_filerecords()->resize(tmpBuf.size());
	uLongf _afterCompSz = (uLongf)tmpBuf.size();
	if (Z_OK == compress((Bytef*)&((*ret->mutable_filerecords())[0]), &_afterCompSz,
			             (const Bytef*)&tmpBuf[0], (uLong)tmpBuf.size())) {
		ret->mutable_filerecords()->resize(_afterCompSz);
		ret->set_ziptype(RemoteRouteProto::eZLib);
		bDataOrg = true;
	}
	if (!bDataOrg) {
		ret->set_filerecords(&tmpBuf[0], tmpBuf.size());
		ret->set_ziptype(RemoteRouteProto::eNoZip);
	}
	ret->set_ret(RemoteRouteProto::eOkay);
	return ret;
}

}
