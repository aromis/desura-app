/*
Desura is the leading indie game distribution platform
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)

$LicenseInfo:firstyear=2014&license=lgpl$
Copyright (C) 2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>
or write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
*/


#ifndef DESURA_BRANCHINSTALLINFO_H
#define DESURA_BRANCHINSTALLINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "usercore/ItemInfoI.h"

namespace XML
{
	class gcXMLElement;
}

namespace UnitTest
{
	class BranchInstallInfoFixture;
}

class InsCheck;

namespace sqlite3x
{
	class sqlite3_connection;
}

class WildcardManager;

namespace UserCore
{
	namespace Item
	{

		class BranchItemInfoI;
		class ItemInfo;

		class ExeInfo : public Misc::ExeInfoI
		{
		public:
			ExeInfo(const char* name)
			{
				m_szName = name;
				m_uiRank = -1;
			}

			virtual const char* getName()
			{
				return m_szName.c_str();
			}

			virtual const char* getExe()
			{
				return m_szExe.c_str();
			}

			virtual const char* getExeArgs()
			{
				return m_szExeArgs.c_str();
			}

			virtual const char* getUserArgs()
			{
				return m_szUserArgs.c_str();
			}

			void setExe(const char* exe)
			{
				if (!exe)
					m_szExe = "";
				else
					m_szExe = UTIL::FS::PathWithFile(exe).getFullPath();
			}

			void setUserArgs(const char* args)
			{
				m_szUserArgs = gcString(args);
			}

			gcString m_szExe;		//exe command
			gcString m_szExeArgs;	//command line args
			gcString m_szUserArgs;	//user command line args
			gcString m_szName;
			uint32 m_uiRank;

			gc_IMPLEMENT_REFCOUNTING(ExeInfo);
		};

		class ProcessResult
		{
		public:
			bool found;
			bool useCip;
			bool notFirst;
			gcString insCheck;
		};

		class BranchInstallInfo : public gcRefBase
		{
		public:
			BranchInstallInfo(uint32 biId, gcRefPtr<BranchItemInfoI> itemInfo, UTIL::FS::UtilFSI* pFileSystem);
			~BranchInstallInfo();


			//! Removes this from the db
			//!
			//! @param db Sqlite db connection
			//!
			void deleteFromDb(sqlite3x::sqlite3_connection* db);

			//! Save regular changed vars to db
			//!
			//! @param db Sqlite db connection
			//!
			void saveDb(sqlite3x::sqlite3_connection* db);

			//! Load vars from db
			//!
			//! @param db Sqlite db connection
			//!
			void loadDb(sqlite3x::sqlite3_connection* db);

			//! Load data for this from xml
			//!
			//! @param xmlNode Xml to get data from
			//!
			ProcessResult processSettings(const XML::gcXMLElement &setNode, gcRefPtr<WildcardManager> &pWildCard, bool reset, bool hasBroughtItem, const char* cipPath);


			void setLinkInfo(const char* szPath, const char* szExe, const char* szArgs);

			uint32 getExeCount(bool setActive);
			void setActiveExe(const char* name);
			gcRefPtr<UserCore::Item::Misc::ExeInfoI> getActiveExe();
			void getExeList(std::vector<gcRefPtr<UserCore::Item::Misc::ExeInfoI>> &list);


			const char* getInstallCheck();

			void updated();
			bool setInstalledMcf(MCFBuild build);
			void overideInstalledBuild(MCFBuild build);
			void resetInstalledMcf();

			bool processUpdateXml(const XML::gcXMLElement &branch);

			bool hasInstalledMod();

			DesuraId getInstalledModId();
			const char* getPath();
			const char* getInsPrimary();
			void setInstalledModId(DesuraId id);
			const char* getInstalledVersion();
			uint64 getInstallSize();
			uint64 getDownloadSize();
			MCFBuild getLastInstalledBuild();
			MCFBuild getInstalledBuild();
			MCFBuild getNextUpdateBuild();
			void overideMcfBuild(MCFBuild build);

			uint32 getBiId();


			void setLinkInfo(const char* exe, const char* args);

		protected:
			friend class UnitTest::BranchInstallInfoFixture;

			void launchExeHack();
			void processExes(const XML::gcXMLElement &setNode, gcRefPtr<WildcardManager> &pWildCard, bool useCip);

			virtual bool isValidFile(const gcString &strFile);

			void setPath(const char *path);
			void setInsCheck(const char *path);
			void setInsPrimary(const char* path);

			bool isInstalled();


			void extractInstallChecks(const XML::gcXMLElement &icsNode, gcRefPtr<WildcardManager> &pWildCard, std::vector<InsCheck> &vInsChecks);

			//Used for install items to convert install check to path relative to the dir the item is installed to
			bool updateInstallCheck(gcString &strCheckRes, const gcString &strPath);
			void UpdateInstallCheckList(const std::vector<InsCheck> &vInsChecks, gcRefPtr<WildcardManager> &pWildCard);

		private:
			gcString m_szPath;
			gcString m_szInsCheck;	//install check
			gcString m_szInsPrim;
			gcString m_szInsVersion;

			gcString m_szActiveExe;

			std::vector<gcRefPtr<ExeInfo>> m_vExeList;
			std::vector<gcString> m_vInstallChecks;

			MCFBuild m_NextBuild;	//next build
			MCFBuild m_INBuild;		//installed build
			MCFBuild m_LastBuild;	//last installed build

			uint64 m_uiInstallSize;
			uint64 m_uiDownloadSize;

			DesuraId m_ItemId;
			DesuraId m_iInstalledMod;

			uint32 m_BiId;

			gcRefPtr<BranchItemInfoI> m_pItem;
			UTIL::FS::UtilFSI* m_pFileSystem;

			gc_IMPLEMENT_REFCOUNTING(BranchInstallInfo)
		};


		inline const char* BranchInstallInfo::getInstallCheck()
		{
			return m_szInsCheck.c_str();
		}

		inline bool BranchInstallInfo::hasInstalledMod()
		{
			return m_iInstalledMod.isOk();
		}

		inline DesuraId BranchInstallInfo::getInstalledModId()
		{
			return m_iInstalledMod;
		}

		inline const char* BranchInstallInfo::getPath()
		{
			return m_szPath.c_str();
		}

		inline const char* BranchInstallInfo::getInsPrimary()
		{
			return m_szInsPrim.c_str();
		}

		inline void BranchInstallInfo::setInstalledModId(DesuraId id)
		{
			m_iInstalledMod = id;
		}

		inline const char* BranchInstallInfo::getInstalledVersion()
		{
			return m_szInsVersion.c_str();
		}

		inline uint64 BranchInstallInfo::getInstallSize()
		{
			return m_uiInstallSize;
		}

		inline uint64 BranchInstallInfo::getDownloadSize()
		{
			return m_uiDownloadSize;
		}

		inline MCFBuild BranchInstallInfo::getLastInstalledBuild()
		{
			return m_LastBuild;
		}

		inline MCFBuild BranchInstallInfo::getInstalledBuild()
		{
			return m_INBuild;
		}

		inline MCFBuild BranchInstallInfo::getNextUpdateBuild()
		{
			return m_NextBuild;
		}

		inline void BranchInstallInfo::overideMcfBuild(MCFBuild build)
		{
			m_INBuild = build;
		}

		inline uint32 BranchInstallInfo::getBiId()
		{
			return m_BiId;
		}
	}
}

#endif //DESURA_BRANCHINSTALLINFO_H
