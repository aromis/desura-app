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

#include "Common.h"
#include "InternalLink.h"

#include "CreateForm.h"
#include "UploadForm.h"
#include "ChangeLogForm.h"
#include "ModWizard.h"
#include "NewsForm.h"
#include "gcUpdateForm.h"

#include "LaunchPrompt.h"
#include "ItemUpdateForm.h"
#include "EulaForm.h"
#include "CDKeyForm.h"
#include "ExeSelectForm.h"
#include "GameDiscForm.h"

#include "usercore/ItemHandleI.h"

template <class T>
T* findForm(const std::function<bool(T*)> &callback, std::vector<wxFrame*>& vSubForms)
{
	for (size_t x = 0; x<vSubForms.size(); x++)
	{
		T * temp = dynamic_cast< T *>(vSubForms[x]);

		if (!temp)
			continue;

		if (callback(temp))
		{
			return temp;
		}
	}

	return nullptr;
}

template <class T>
T* findForm(DesuraId id, std::vector<wxFrame*>& vSubForms)
{
	return findForm<T>([id](T* pForm){
		return pForm->getItemId() == id;
	}, vSubForms);
}



#define FINDFORM( id, type )									\
	{															\
	type *temp = findForm< type >( (id) , m_vSubForms);			\
	if (temp){temp->Show(true);temp->Raise();return;}			\
	}


InternalLink::InternalLink(wxWindow *parent)
{
	m_iNewsFormId = 0;
	m_iGiftFormId = 0;
	m_pParent = parent;
}

InternalLink::~InternalLink()
{
}

void InternalLink::regForm(DesuraId id, gcFrame *form)
{
	m_mFormMap[id.toInt64()] = form;
	m_vSubForms.push_back(form);
}

bool InternalLink::checkForm(DesuraId id)
{
	std::map<uint64,gcFrame*>::iterator it = m_mFormMap.find(id.toInt64());

	if (it != m_mFormMap.end() && it->second)
	{
		UI::Forms::ItemForm* itemForm = dynamic_cast<UI::Forms::ItemForm*>(it->second);

		if (itemForm && !itemForm->isInit())
			return false;

		it->second->Show();
		it->second->Raise();
		return true;
	}

	return false;
}

void InternalLink::closeAll()
{
	size_t size = m_vSubForms.size();
	//we need to do m_pParent as the forms get removed on close so thus the size keeps changing.
	while (size > 0)
	{
		UploadMCFForm* t1 = dynamic_cast< UploadMCFForm *>(m_vSubForms[0]);

		if (t1)
			t1->setTrueClose();
		
		m_vSubForms[0]->Show(false);
		m_vSubForms[0]->Close(true);

		size = m_vSubForms.size();
	}

	m_vSubForms.clear();
	m_mFormMap.clear();
}

void InternalLink::handleInternalLink(const char* link)
{
	std::vector<gcString> list;
	bool badLink = false;

	g_pMainApp->showMainWindow(true);
	
	if (strncmp("desura://", link, 9) == 0)
	{
		char* str = nullptr;
		Safe::strcpy(&str, link+9, 255);

		char* context = nullptr;

		char* token = Safe::strtok(str, "/", &context);

		while (token)
		{
			list.push_back(gcString(token));
			token = Safe::strtok(nullptr, "/", &context);
		}

		safe_delete(str);
	}
	else
	{	
		badLink = true;
	}

	if (list.size() >= 1)
	{
		if (list[0] == "switchtab" || list[0] == "tab")
		{
			if (!switchTab(badLink, list, link))
				return;
		}
		else if (list[0] == "settings")
		{
			gcString url(GetGCThemeManager()->getWebPage("settings"));

			if (list.size() >=2 )
				url += "#" + list[1];
			else
				url += "#general";

			g_pMainApp->loadUrl(url.c_str(), SUPPORT);
		}
		else if (list[0] == "cip")
		{
			handleInternalLink(0, ACTION_SHOWSETTINGS, FormatArgs("tab=cip"));
		}
		else if (list.size() < 3)
		{
#ifdef WIN32
			if (list[0] == "installwizard")
			{
				handleInternalLink(0, ACTION_INSTALLEDW);
			}
			else 
#endif	
			if (list[0] == "refresh")
			{
				GetUserCore()->forceUpdatePoll();

				if ((list.size() >= 2 && list[1] == "background") == false)
					g_pMainApp->showPlay();
			}
			else
			{
				badLink = true;
			}
		}
		else
		{
			if (!processItemLink(badLink, list, link))
				return;
		}
	}

	if (badLink)
	{
		Warning("{0} [{1}]\n", Managers::GetString("#MF_BADLINK"), link);

		gcWString errMsg(L"{0}: {1}.", Managers::GetString("#MF_BADLINK"), link);
		gcMessageBox(g_pMainApp->getMainWindow(), errMsg, Managers::GetString(L"#MF_ERRTITLE"));
	}
}


bool InternalLink::switchTab(bool &badLink, std::vector<gcString> &list, const char* link)
{
	if (list.size() < 2)
	{
		Warning("{0} [{1}]\n", Managers::GetString("#MF_BADLINK"), link);
		return false;
	}

	PAGE page = ITEMS;

	if (list[1] == "play")
	{
		page = ITEMS;
	}
	else if (list[1] == "games")
	{
		page = GAMES;
	}
#ifndef UI_HIDE_MODS
	else if (list[1] == "mods")
	{
		page = MODS;
	}
#endif
	else if (list[1] == "community")
	{
		page = COMMUNITY;
	}
	else if (list[1] == "development")
	{
		page = DEVELOPMENT;
	}
	else
	{
		Warning("{0} [{1}]\n", Managers::GetString("#MF_BADLINK"), link);
		return false;
	}

	g_pMainApp->showPage(page);

	if (page != ITEMS && list.size() >= 3 && list[2].size() > 0)
	{
		auto out = UTIL::STRING::base64_decode(list[2]);
		auto outLen = out.size();

		std::string url(out, outLen);

		if (!url.empty())
		{
			std::transform(begin(url), end(url), begin(url), ::tolower);
			
			if (url.find("http://") == 0 || url.find("https://") == 0)
				g_pMainApp->loadUrl(url.c_str(), page);
		}
	}

	return true;
}

bool InternalLink::processItemLink(bool &badLink, std::vector<gcString> &list, const char* link)
{
	if (list.size() < 3)
	{
		badLink = true;
		return true;
	}

	std::string key = list[1] + list[2];

	std::map<gcString, gcFrame*>::iterator it = m_mWaitingItemFromMap.find(key);

	if (it != m_mWaitingItemFromMap.end())
	{
		it->second->Raise();
		return false;
	}

	UI::Forms::ItemForm *form = new UI::Forms::ItemForm(m_pParent, list[0].c_str(), list[2].c_str());
	m_mWaitingItemFromMap[key] = form;

	form->Show();

	DesuraId id;

	try
	{
		id = GetWebCore()->nameToId(list[2].c_str(), list[1].c_str());
	}
	catch (gcException &e)
	{
		DesuraId idAsNum(list[2].c_str(), list[1].c_str());

		if (idAsNum.getItem() == 0)
		{
			Warning("Failed to resolve item name {0} for link {2}: {1}\n", list[2], e, link);
			gcErrorBox(form, Managers::GetString("#MF_ERRTITLE"), Managers::GetString("#MF_NAMERESOLVE"), e);

			m_mWaitingItemFromMap.erase(m_mWaitingItemFromMap.find(key));

			form->Show(false);
			form->Destroy();
			return false;
		}
		else
		{
			id = idAsNum;
		}
	}

	m_mWaitingItemFromMap.erase(m_mWaitingItemFromMap.find(key));

	bool destroyForm = false;
	std::vector<std::string> argList;

	if (id.isOk())
	{
		UI::Forms::ItemForm *formFind = findForm<UI::Forms::ItemForm>(id, m_vSubForms);

		if (!formFind)
		{
			form->setItemId(id);
			regForm(id, form);
		}
		else
		{
			formFind->SetPosition(form->GetPosition());
			form->Show(false);
			destroyForm = true;
		}

		if (list[0] == "install" || list[0] == "launch")
		{
			std::string branch;

			if (list.size() >= 4)
				branch = list[3];

			if (branch.size() > 0)
			{
				argList.push_back(std::string("global=") + branch);
				handleInternalLink(id, ACTION_INSTALL, argList);	
			}
			else
			{
				bool isInstall = (list[0] == "install");
				handleInternalLink(id, isInstall?ACTION_INSTALL:ACTION_LAUNCH, argList);	
			}
		}
		else if (list[0] == "uninstall" || list[0] == "remove")
		{
			handleInternalLink(id, ACTION_UNINSTALL);
		}
		else if (list[0] == "verify")
		{
			handleInternalLink(id, ACTION_VERIFY);
		}
		else if (list[0] == "upload")
		{
			destroyForm = true;
			handleInternalLink(id,  ACTION_UPLOAD);
		}
		else if (list[0] == "resumeupload" && list.size() >= 4)
		{
			destroyForm = true;
			argList.push_back(std::string("key=") + list[3]);
			handleInternalLink(id, ACTION_RESUPLOAD, argList);	//
		}
		else if (list[0] == "makemcf")
		{
			destroyForm = true;
			handleInternalLink(id, ACTION_CREATE);
		}
		else if (list[0] == "test" && list.size() >= 5)
		{
			argList.push_back(std::string("branch=") + list[3]);
			argList.push_back(std::string("build=") + list[4]);
			handleInternalLink(id, ACTION_TEST, argList);
		}
		else
		{
			badLink = true;
		}
	}
	else
	{
		badLink = true;
	}

	if (badLink || destroyForm)
	{
		closeForm(form->GetId());
		form->Show(false);
		form->Destroy();
	}

	return true;
}




void InternalLink::handleInternalLink(DesuraId id, uint8 action, const LinkArgs &a)
{
	if (g_pMainApp->isOffline() && action != ACTION_LAUNCH)
		return;

	LinkArgs args(a);

	bool handled = true;

	switch (action)
	{
	case ACTION_UPLOAD		: uploadMCF( id );						break;
	case ACTION_CREATE		: createMCF( id );						break;
	case ACTION_RESUPLOAD	: resumeUploadMCF( id, args );			break;
#ifdef WIN32
	case ACTION_INSTALLEDW	: installedWizard();					break;
#endif
	case ACTION_SHOWSETTINGS: showSettings(args);					break;
	case ACTION_APPUPDATELOG: showUpdateLogApp( id.getItem() );		break;
	case ACTION_PAUSE		: setPauseItem( id , true );			break;
	case ACTION_UNPAUSE		: setPauseItem( id , false );			break;
	case ACTION_UNINSTALL	: uninstallMCF( id );					break;
	case ACTION_PROMPT		: showPrompt(id, args);					break;
	case ACTION_UPDATELOG	: showUpdateLog(id);					break;
	case ACTION_DISPCDKEY	: showCDKey(id, args);					break;

	default: 
		handled = false;
		break;
	}

	if (handled || checkForm(id))
		return;

	switch (action)
	{
	case ACTION_INSTALL		: installItem(id, args);				break;
	case ACTION_LAUNCH		: launchItem(id, args);					break;
	case ACTION_VERIFY		: verifyItem(id, args);					break;
	case ACTION_UPDATE		: updateItem(id, args);					break;
	case ACTION_TEST		: installTestMCF(id, args);				break;
	case ACTION_INSCHECK	: installCheck(id);						break;
	case ACTION_SHOWUPDATE	: showUpdateForm(id, args);				break;
	case ACTION_SWITCHBRANCH: switchBranch(id, args);				break;
	case ACTION_CLEANCOMPLEXMOD: cleanComplexMod(id);				break;

	default: 
		Warning("Unknown internal link {0} for item {1}\n.", (uint32)action, id.toInt64());	
		break;
	}
}


void InternalLink::closeForm(int32 wxId)
{
	if (wxId == m_iNewsFormId)
		m_iNewsFormId = 0;

	if (wxId == m_iGiftFormId)
		m_iGiftFormId = 0;

	for (auto f : m_mFormMap)
	{
		if (f.second && f.second->GetId() == wxId)
		{
			m_mFormMap[f.first] = nullptr;
			break;
		}
	}

	auto it = std::find_if(begin(m_vSubForms), end(m_vSubForms), [wxId](wxFrame* pFrame)
	{
		return pFrame->GetId() == wxId;
	});

	if (it == end(m_vSubForms))
		return;

	(*it)->Destroy();
	m_vSubForms.erase(it);
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PreloadButtonHelper : public HelperButtonsI
{
public:
	PreloadButtonHelper(DesuraId id)
	{
		m_Id = id;

		gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo(id);

		if (!item)
			return;

		for (size_t x=0; x<item->getBranchCount(); x++)
		{
			uint32 flags = item->getBranch(x)->getFlags();
			
			if (HasAllFlags(flags, UserCore::Item::BranchInfoI::BF_NORELEASES))
				continue;

			auto branch = item->getBranch(x);

			if (branch->isPreOrder() || branch == item->getCurrentBranch())
				continue;

			bool isDemo = HasAnyFlags(flags, UserCore::Item::BranchInfoI::BF_DEMO|UserCore::Item::BranchInfoI::BF_TEST);
			bool onAccount = HasAnyFlags(flags, UserCore::Item::BranchInfoI::BF_FREE|UserCore::Item::BranchInfoI::BF_ONACCOUNT);
			bool isLocked = HasAnyFlags(flags, UserCore::Item::BranchInfoI::BF_REGIONLOCK|UserCore::Item::BranchInfoI::BF_MEMBERLOCK);

			if (isDemo)
				m_DemoBranch = item->getBranch(x)->getBranchId();
			else if (onAccount && !isLocked)
				m_bOtherBranches = true;
		}
	}

	virtual uint32 getCount()
	{
		uint32 count = 1;

		if (m_DemoBranch != 0)
			count++;

		if (m_bOtherBranches)
			count++;

		return count;
	}

	virtual const wchar_t* getLabel(uint32 index)
	{
		index = mapIndex(index);

		if (index == 0)
			return Managers::GetString(L"#PM_VIEWPROFILE");
		else if (index == 1)
			return Managers::GetString(L"#IF_PRELOADLAUNCH_INSTALLDEMO");
		else if (index == 2)
			return Managers::GetString(L"#IF_PRELOADLAUNCH_INSTALLOTHER");

		return nullptr;
	}

	virtual const wchar_t* getToolTip(uint32 index)
	{
		return nullptr;
	}

	virtual void performAction(uint32 index)
	{
		index = mapIndex(index);

		if (index == 0)
		{
			g_pMainApp->handleInternalLink(m_Id, ACTION_PROFILE);
		}
		else if (index == 1)
		{
			std::vector<std::string> argsList;
			argsList.push_back(gcString("branch={0}", (uint32)m_DemoBranch));

			g_pMainApp->handleInternalLink(m_Id, ACTION_INSTALL, argsList);
		}
		else if (index == 2)
		{
			std::vector<std::string> argsList;
			argsList.push_back("skippreorder");
			g_pMainApp->handleInternalLink(m_Id, ACTION_INSTALL, argsList);
		}
	}

	uint32 mapIndex(uint32 index)
	{
		if (m_DemoBranch == 0 && m_bOtherBranches && index == 1)
			index = 2;

		return index;
	}

	bool m_bOtherBranches = false;
	MCFBranch m_DemoBranch;
	DesuraId m_Id;
};

void InternalLink::showPrompt(DesuraId id, LinkArgs args)
{
	std::string prompt = args.getArgValue("prompt");
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (prompt == "update")
	{
		args.push_back("reminder=true");
		showUpdateForm(id, args);
	}
	else if (prompt == "launch")
	{
		LaunchItemDialog* form = new LaunchItemDialog(m_pParent);
		regForm(id, form);

		form->setInfo(item);

		form->Show(true);
		form->Raise();
	}
	else if (prompt == "eula")
	{
		showEULA(id);
	}
	else if (prompt == "preload")
	{
		showPreorderPrompt(id, true);
	}
	else if (prompt == "needtorunfirst")
	{
		std::string parentId = args.getArgValue("parentid");

		DesuraId pid(parentId.c_str(), "games");
		showNeedToRun(id, pid);
	}
	else
	{
		gcAssert(false);
	}
}

void InternalLink::showPreorderPrompt(DesuraId id, bool isPreload)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo(id);

	if (!item)
		return;

	auto bi = item->getCurrentBranch();

	if (!bi)
	{
		for (size_t x = 0; x < item->getBranchCount(); x++)
		{
			auto temp = item->getBranch(x);

			if (temp->isPreOrder())
			{
				bi = temp;
				break;
			}
		}
	}

	if (!bi)
		return;

	const char* str = bi->getPreOrderExpDate();

	uint32 days;
	uint32 hours;
	std::string time_available = UTIL::MISC::dateTimeToDisplay(str);

	UTIL::MISC::getTimeDiffFromNow(str, days, hours);

	gcString title(Managers::GetString("#IF_PRELOADLAUNCH_TITLE"), item->getName());
	gcString msg(Managers::GetString("#IF_PRELOADLAUNCH"), item->getName(),
		days,
		Managers::GetString(isPreload ? "#IF_PRELOADLAUNCH_PRELOADED" : "#IF_PRELOADLAUNCH_PREORDERED"),
		time_available,
		Managers::GetString(days == 1 ? "#IF_PRELOADLAUNCH_WORD_DAY" : "#IF_PRELOADLAUNCH_WORD_DAYS"));

	PreloadButtonHelper pobh(id);

	if (pobh.m_bOtherBranches)
		msg += gcString(Managers::GetString("#IF_PRELOADLAUNCH_INSTALLOTHER_INFO"), item->getName());

	auto form = findForm<UI::Forms::ItemForm>(id, m_vSubForms);

	if (form && form->hasMessageBox())
		return;

	if (form)
	{
		gcMessageBox(form, msg, title, wxICON_EXCLAMATION | wxCLOSE, &pobh);
		form->Close();
	}
	else
	{
		gcMessageBox(g_pMainApp->getMainWindow(), msg, title, wxICON_EXCLAMATION | wxCLOSE, &pobh);
	}	
}

UI::Forms::ItemForm* InternalLink::showItemForm(DesuraId id, UI::Forms::INSTALL_ACTION action, bool showForm, LinkArgs args)
{
	return showItemForm(id, action, MCFBranch(), MCFBuild(), showForm, args);
}

UI::Forms::ItemForm* InternalLink::showItemForm(DesuraId id, UI::Forms::INSTALL_ACTION action, MCFBranch branch, MCFBuild build, bool showForm, LinkArgs args)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!item && action != UI::Forms::INSTALL_ACTION::IA_INSTALL && action != UI::Forms::INSTALL_ACTION::IA_INSTALL_TESTMCF)
		return nullptr;

	UI::Forms::ItemForm *form = findForm<UI::Forms::ItemForm>(id, m_vSubForms);
	
	if (!form)
	{
		form = new UI::Forms::ItemForm(m_pParent);
		form->setItemId(id);
		regForm(id, form);
	}

	form->pushArgs(args);
	form->newAction(action, branch, build, showForm);
	form->popArgs();

	return form;
}


void InternalLink::installCheck(DesuraId id)
{
	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_INSTALL_CHECK);

	if (!form)
		Warning("Cant find item (or item not ready) for install check [{0}].\n", id.toInt64());
}

void InternalLink::verifyItem(DesuraId id, LinkArgs args)
{
	bool showForm = args.getArgValue("show") != "false";

	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_VERIFY, showForm);

	if (!form)
		Warning("Cant find item (or item not ready) for verify [{0}].\n", id.toInt64());
	else if (showForm == false)
		form->Show(false);
}

void InternalLink::cleanComplexMod(DesuraId id)
{
	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_CLEANCOMPLEX, true);

	if (!form)
		Warning("Cant find item (or item not ready) for clean complex mod [{0}].\n", id.toInt64());
}

bool InternalLink::checkForPreorder(DesuraId id)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!item)
		return false;

	if (item->getCurrentBranch())
		return item->getCurrentBranch()->isPreOrderAndNotPreload();


	for (size_t x=0; x<item->getBranchCount(); x++)
	{
		if (item->getBranch(x)->isPreOrderAndNotPreload())
			return true;
	}		

	return false;
}

void InternalLink::installItem(DesuraId id, LinkArgs args)
{
	if (checkForPreorder(id))
	{
		showPreorderPrompt(id, false);
		return;
	}

	std::string branch = args.getArgValue("branch");
	std::string global = args.getArgValue("global");

	MCFBranch iBranch;

	if (branch == "shortcut" || global == "shortcut")
	{
		iBranch = MCFBranch::BranchFromInt(-1);
	}
	else if (global.size() > 0)
	{
		iBranch = MCFBranch::BranchFromInt(Safe::atoi(global.c_str()), true);
	}
	else
	{
		iBranch = MCFBranch::BranchFromInt(Safe::atoi(branch.c_str()));
	}

	g_pMainApp->showPlay();

	if (iBranch == 0 && !args.containsArg("skippreorder") && checkForPreorder(id))
		return;
	
	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_INSTALL, iBranch);

	if (!form)
		Warning("Cant find item (or item not ready) for install [{0}].\n", id.toInt64());	
}

DesuraId g_GameDiskList[] = 
{
	DesuraId(49, DesuraId::TYPE_GAME),
	DesuraId(184, DesuraId::TYPE_GAME),
	DesuraId()
};

void InternalLink::launchItem(DesuraId id, LinkArgs args)
{
	bool cdKeyArg = args.containsArg("cdkey");
	bool noUpdateArg = args.containsArg("noupdate");
	bool exeNumArg = args.containsArg("exe");
	std::string exe = args.getArgValue("exe");

	if (exe == "")
		exeNumArg = false;

	g_pMainApp->showPlay();

	if (checkForPreorder(id))
	{
		showPreorderPrompt(id, false);
		return;
	}

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo(id);

	if (!item || !item->isLaunchable())
	{
		installItem(id, LinkArgs());
		return;
	}

	if (!item->hasAcceptedEula())
	{
		showPrompt(id, FormatArgs("prompt=eula"));
		return;
	}

	if (!exeNumArg && item->getExeCount(true) > 1)
	{
		showExeSelect(id, cdKeyArg);
		return;
	}
	else if (exe.size() > 0)
	{
		item->setActiveExe(exe.c_str());
	}

	bool hasCDKey = item->getCurrentBranch() && item->getCurrentBranch()->hasCDKey();
	bool hasDLCDKey = item->getCurrentBranch() && item->getCurrentBranch()->isCDKeyValid();

	if (!cdKeyArg && ((item->isFirstLaunch() && hasCDKey) || (hasCDKey && !hasDLCDKey)))
	{
		showCDKey(id, FormatArgs("launch=true", std::string("exe=") + exe));
		return;
	}

	bool shouldShowGameDisk = false;

	if (args.containsArg("gamedisk") == false && HasAnyFlags(item->getOptions(), UserCore::Item::ItemInfoI::OPTION_DONTPROMPTGAMEDISK) == false)
	{
		size_t x=0; 
		while (g_GameDiskList[x].isOk())
		{
			if (item->getId() == g_GameDiskList[x] || item->getParentId() == g_GameDiskList[x])
			{
				shouldShowGameDisk = true;
				break;
			}

			x++;
		}
	}

	if (shouldShowGameDisk)
	{
		showGameDisk(id, exe.c_str(), cdKeyArg);
		return;
	}

	if (noUpdateArg)
		item->addOFlag(UserCore::Item::ItemInfoI::OPTION_NOTREMINDUPDATE_ONETIME);

	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_LAUNCH, true, args);

	if (!form)
		Warning("Cant find item (or item not ready) for launch [{0}].\n", id.toInt64());
}

void InternalLink::updateItem(DesuraId id, LinkArgs args)
{
	bool show = args.containsArg("show") && args.getArgValue("show") == "true";

	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_UPDATE, show);

	if (!form)
		Warning("Cant find item (or item not ready) for update [{0}].\n", id.toInt64());
}


void InternalLink::installTestMCF(DesuraId id, LinkArgs args)
{
	std::string branch = args.getArgValue("branch");
	std::string build = args.getArgValue("build");

	MCFBranch iBranch;
	MCFBuild iBuild ;

	if (branch.size() > 0)
		iBranch = MCFBranch::BranchFromInt(Safe::atoi(branch.c_str()));

	if (build.size() > 0)
		iBuild = MCFBuild::BuildFromInt(Safe::atoi(build.c_str()));

	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_INSTALL_TESTMCF, iBranch, iBuild);

	if (!form)
		Warning("Cant find item (or item not ready) for install test mcf [{0}].\n", id.toInt64());
}


void InternalLink::uninstallMCF(DesuraId id)
{	
	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_UNINSTALL);

	if (form)
		form->Raise();
	else
		Warning("Cant find item (or item not ready) for uninstall [{0}].\n", id.toInt64());
}


void InternalLink::switchBranch(DesuraId id, LinkArgs args)
{
	std::string branch = args.getArgValue("branch");

	if (branch.size() == 0)
		return;

	MCFBranch iBranch = MCFBranch::BranchFromInt(Safe::atoi(branch.c_str()));

	if (iBranch == 0)
		return;

	UI::Forms::ItemForm* form = showItemForm(id, UI::Forms::INSTALL_ACTION::IA_SWITCH_BRANCH, iBranch);

	if (!form)
		Warning("Cant find item (or item not ready) for uninstall [{0}].\n", id.toInt64());
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void InternalLink::showEULA(DesuraId id)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );
	if (!item)
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(),  Managers::GetString(L"#MF_NAMERESOLVE"), Managers::GetString(L"#MF_ERRTITLE") );
		return;
	}

	EULAForm* form = new EULAForm(m_pParent);
	regForm(id, form);

	if (form->setInfo(id))
	{
		form->Show(true);	
		form->Raise();
	}
}

class NeedToRunMessageHelper : public HelperButtonsI
{
public:
	NeedToRunMessageHelper(DesuraId pid)
		: m_Pid(pid)
	{
	}


	uint32 getCount() override
	{
		return 1;
	}

	const wchar_t* getLabel(uint32 index) override
	{
		return Managers::GetString(L"#LAUNCH");
	}

	const wchar_t* getToolTip(uint32 index) override
	{
		return L"";
	}

	void performAction(uint32 index) override
	{
		g_pMainApp->handleInternalLink(m_Pid, ACTION_LAUNCH, FormatArgs());
	}

	DesuraId m_Pid;
};

void InternalLink::showNeedToRun(DesuraId mid, DesuraId pid)
{
	gcRefPtr<UserCore::Item::ItemInfoI> mod = GetUserCore()->getItemManager()->findItemInfo(mid);
	gcRefPtr<UserCore::Item::ItemInfoI> parent = GetUserCore()->getItemManager()->findItemInfo(pid);

	if (!mod || !parent)
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_NAMERESOLVE"), Managers::GetString(L"#MF_ERRTITLE"));
		return;
	}

	gcWString msg(Managers::GetString(L"#MF_NEEDTORUN"), mod->getName(), parent->getName());

	NeedToRunMessageHelper helper(pid);
	gcMessageBox(g_pMainApp->getMainWindow(), msg, Managers::GetString(L"#MF_ERRTITLE"), wxICON_EXCLAMATION | wxOK, &helper);
}

void InternalLink::showUpdateLog(DesuraId id)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );
	if (!item)
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(),  Managers::GetString(L"#MF_UPDATELOG_ITEMERROR"), Managers::GetString(L"#MF_ERRTITLE") );
		return;
	}

	ChangeLogForm* form = new ChangeLogForm(m_pParent);
	regForm(id, form);

	form->setInfo(item);
	form->Show(true);	
	form->Raise();
}

void InternalLink::showExeSelect(DesuraId id, bool hasSeenCDKey)
{
	FINDFORM(id, ExeSelectForm);

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo(id);

	if (!item)
	{
		Warning("Cant find item for show exe select [{0}].\n", id.toInt64());
		return;
	}

	ExeSelectForm* form = new ExeSelectForm(m_pParent, hasSeenCDKey);
	m_vSubForms.push_back(form);

	form->setInfo(id);
	form->Show(true);
}

void InternalLink::showCDKey(DesuraId id, LinkArgs args)
{
	FINDFORM(id, CDKeyForm);

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!item)
	{
		Warning("Cant find item for show cd key [{0}].\n", id.toInt64());
		return;
	}

	std::string launch = args.getArgValue("launch");
	std::string exe = args.getArgValue("exe");


	CDKeyForm* form = new CDKeyForm(m_pParent, exe.c_str(), launch == "true");
	m_vSubForms.push_back(form);

	form->setInfo(id);
	form->Show(true);
}

void InternalLink::showGameDisk(DesuraId id, const char* exe, bool cdkey)
{
	FINDFORM(id, GameDiskForm);

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!item)
	{
		Warning("Cant find item for show game disk [{0}].\n", id.toInt64());
		return;
	}

	GameDiskForm* form = new GameDiskForm(m_pParent, exe, cdkey);
	m_vSubForms.push_back(form);

	form->setInfo(id);
	form->Show(true);
}

void InternalLink::showUpdateForm(DesuraId id, LinkArgs args)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!item)
		return;

	if (!HasAllFlags(item->getStatus(), UserCore::Item::ItemInfoI::STATUS_UPDATEAVAL))
		return;
	
	std::string reminder = args.getArgValue("reminder");

	//create new gather info form from
	UpdateInfoForm* form = new UpdateInfoForm(m_pParent);
	regForm(id, form);

	form->setInfo(id, reminder == "true", args);
	form->Show(true);
}






//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





void InternalLink::setPauseItem(DesuraId id, bool state)
{
	auto itemHandle = GetUserCore()->getItemManager()->findItemHandle(id);

	if (itemHandle)
	  itemHandle->setPaused(state);
	else if (state == false)
		launchItem(id, LinkArgs()); //if we paused and restarted desura we wont have a valid install form
}

void InternalLink::installedWizard()
{
	for (size_t x=0; x<m_vSubForms.size(); x++)
	{
		ModWizardForm* temp = dynamic_cast<ModWizardForm*>(m_vSubForms[x]);

		if (temp)
		{
			m_vSubForms[x]->Show(true);
			m_vSubForms[x]->Raise();
			return;
		}
	}

	ModWizardForm* form = new ModWizardForm(m_pParent);
	form->Show(true);
	m_vSubForms.push_back(form);
}

void InternalLink::showSettings(LinkArgs &args)
{
	gcString c(GetGCThemeManager()->getWebPage("settings"));

	if (args.containsArg("tab"))
	{
		gcString tab = args.getArgValue("tab");

		if (tab == "cip")
			tab = "games";

		c += "#" + tab;
	}
	else
	{
		c += "#general";
	}

	g_pMainApp->loadUrl(c.c_str(), SUPPORT);
}

void InternalLink::uploadMCF(DesuraId id)
{
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );
	if (!item && !GetUserCore()->isAdmin())
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_NONDEV_ERROR"), Managers::GetString(L"#MF_PERMISSION_ERRTITLE"));
		return;
	}

	//create new create from
	UploadMCFForm* form = new UploadMCFForm(m_pParent);
	form->setInfo(id);
	form->Show(true);	
	form->Raise();
	form->run();

	m_vSubForms.push_back(form);
}


void InternalLink::resumeUploadMCF(DesuraId id, LinkArgs args)
{
	std::string key = args.getArgValue("key");
	std::string uid = args.getArgValue("uid");

	auto pForm = findForm<UploadMCFForm>([key, uid](UploadMCFForm* pForm){
		return (!key.empty() && pForm->getKey() == key) || (!uid.empty() && pForm->getUid() == uid);
	}, m_vSubForms);

	if (pForm)
	{
		pForm->Raise();
		return;
	}
		
	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );
	if (!item && !GetUserCore()->isAdmin())
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_NONDEV_ERROR"), Managers::GetString(L"#MF_PERMISSION_ERRTITLE"));
		return;
	}

	//create new create from
	UploadMCFForm* form = new UploadMCFForm(m_pParent);

	if (!key.empty())
		form->setInfo_key(id, key.c_str());
	else if (!uid.empty())
		form->setInfo_uid(id, uid.c_str());
	else
		form->setInfo(id);

	form->Show(true);	
	form->Raise();
	form->run();

	m_vSubForms.push_back(form);
}



void InternalLink::createMCF(DesuraId id)
{
	gcTrace("Id: {0}", id);

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( id );

	if (!GetUserCore()->isAdmin() && !item)
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_NONDEV_ERROR"), Managers::GetString(L"#MF_PERMISSION_ERRTITLE"));
		return;
	}

	//create new create from
	CreateMCFForm* form = new CreateMCFForm(m_pParent);
	form->setInfo(id);
	form->onUploadTriggerEvent += delegate(this, &InternalLink::onUploadTrigger);
	form->Show(true);	
	form->Raise();
	form->run();
	
#ifdef NIX
	form->Raise();
#endif

	m_vSubForms.push_back(form);
}



void InternalLink::onUploadTrigger(ut& info)
{
	gcTrace("Id: {0}", info.id);

	FINDFORM(info.id, UploadMCFForm);

	gcRefPtr<UserCore::Item::ItemInfoI> item = GetUserCore()->getItemManager()->findItemInfo( info.id );
	if (!item && !GetUserCore()->isAdmin())
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_NONDEV_ERROR"), Managers::GetString(L"#MF_PERMISSION_ERRTITLE"));
		return;
	}

	if (item && !HasAllFlags(item->getStatus(), UserCore::Item::ItemInfoI::STATUS_DEVELOPER) && !GetUserCore()->isAdmin())
	{
		//cant upload show prompt
		gcMessageBox(g_pMainApp->getMainWindow(), Managers::GetString(L"#MF_UPERMISSION_ERROR"), Managers::GetString(L"#MF_PERMISSION_ERRTITLE"));
		return;
	}

	//create new create from
	UploadMCFForm* form = new UploadMCFForm(m_pParent);
	form->setInfo_path(info.id, info.path.c_str());

	if (info.caller)
	{
		wxPoint pos = info.caller->GetScreenPosition();
		form->SetPosition(pos);
	}

	form->Show(true);	
	form->Raise();
	form->run();

	m_vSubForms.push_back(form);
}




void InternalLink::showUpdateLogApp(uint32 version)
{
	ChangeLogForm * temp = nullptr;

	for (size_t x=0; x<m_vSubForms.size(); x++)
	{
		temp = dynamic_cast< ChangeLogForm *>(m_vSubForms[x]);

		if (!temp)
			continue;

		if (temp->isAppChangeLog())
			break;
	}

	if (temp)
	{
		temp->Show(true);
		temp->Raise();
		return;
	}

	uint32 iAppId = BUILDID_PUBLIC;

	std::string szAppid = UTIL::OS::getConfigValue(APPID);

	if (szAppid.size() > 0)
		iAppId = Safe::atoi(szAppid.c_str());

	ChangeLogForm* form = new ChangeLogForm(m_pParent);
	form->setInfo(iAppId);
	form->Show(true);	
	form->Raise();

	m_vSubForms.push_back(form);
}






//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////







void InternalLink::showNews(const std::vector<gcRefPtr<UserCore::Misc::NewsItem>> &newsItems, const std::vector<gcRefPtr<UserCore::Misc::NewsItem>> &giftItems)
{
	if (newsItems.size() > 0)
	{
		if (m_iNewsFormId != 0)
		{
			for (size_t x=0; x<m_vSubForms.size(); x++)
			{
				if (m_vSubForms[x]->GetId() == m_iNewsFormId)
				{
					NewsForm* temp = dynamic_cast<NewsForm*>(m_vSubForms[x]);

					if (temp)
						temp->loadNewsItems(newsItems);

					m_vSubForms[x]->Show(true);
					m_vSubForms[x]->Raise();
				}
			}
		}
		else
		{
			NewsForm* form = new NewsForm(m_pParent);
			form->loadNewsItems(newsItems);

			m_iNewsFormId = form->GetId();

			form->Show(true);
			form->Raise();

			m_vSubForms.push_back(form);
		}
	}

	if (giftItems.size() > 0)
	{
		if (m_iGiftFormId != 0)
		{
			for (size_t x=0; x<m_vSubForms.size(); x++)
			{
				if (m_vSubForms[x]->GetId() == m_iGiftFormId)
				{
					NewsForm* temp = dynamic_cast<NewsForm*>(m_vSubForms[x]);

					if (temp)
						temp->loadNewsItems(giftItems);

					m_vSubForms[x]->Show(true);
					m_vSubForms[x]->Raise();
				}
			}
		}
		else
		{
			NewsForm* form = new NewsForm(m_pParent);
			form->setAsGift();
			form->loadNewsItems(giftItems);

			m_iGiftFormId = form->GetId();

			form->Show(true);
			form->Raise();

			m_vSubForms.push_back(form);
		}
	}
}

void InternalLink::showAppUpdate(uint32 version)
{
	for (size_t x=0; x<m_vSubForms.size(); x++)	
	{															
		GCUpdateInfo *temp = dynamic_cast< GCUpdateInfo *>(m_vSubForms[x]);

		if (!temp)
		{
			continue;
		}
		else
		{
			temp->setInfo(version);
			return;
		}
	}

	GCUpdateInfo* temp = new GCUpdateInfo(m_pParent);
	temp->setInfo(version);
	//temp->Show(true);

	m_vSubForms.push_back(temp);
}
