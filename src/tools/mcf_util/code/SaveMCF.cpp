/*
Desura is the leading indie game distribution platform
Copyright (C) 2013 Mark Chandler (Desura Net Pty Ltd)

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
#include "UtilFunction.h"



class ExtractMCF : public UtilFunction
{
public:
	ExtractMCF()
	{
		s_uiLastFlag = -1;
	}

	virtual uint32 getNumArgs()
	{
		return 2;
	}

	virtual const char* getArgDesc(size_t index)
	{
		if (index == 0)
			return "Src Mcf";

		return "Dest Mcf";
	}

	virtual const char* getFullArg()
	{
		return "extract";
	}

	virtual const char getShortArg()
	{
		return 'x';
	}

	virtual const char* getDescription()
	{
		return "Extracts a mcf into a folder";
	}

	virtual int performAction(std::vector<std::string> &args)
	{
		MCFCore::MCFI* mcfHandle = mcfFactory();

		mcfHandle->setFile(args[0].c_str());
		mcfHandle->parseMCF();

		mcfHandle->getProgEvent() += delegate(this, &ExtractMCF::mcfProgress);
		mcfHandle->getErrorEvent() += delegate(this, &ExtractMCF::mcfError);
		mcfHandle->saveFiles(args[1].c_str());

		mcfDelFactory(mcfHandle);
		return 0;
	}

protected:
	void mcfProgress(MCFCore::Misc::ProgressInfo& prog)
	{
		if (s_uiLastFlag != prog.flag)
		{
			if (prog.flag == 1)
				printf("\nInit Extraction...\n");
			else 
				printf("\nExtracting...\n");

			s_uiLastFlag = prog.flag;
		}

		printProgress(prog);
	}

	void mcfError(gcException& e)
	{
		gcString err("{0}", e);
		printf("\nMCF Error: %s\n", err.c_str());
	}

	uint32 s_uiLastFlag;
};




class SaveXML : public UtilFunction
{
public:
	virtual uint32 getNumArgs()
	{
		return 2;
	}

	virtual const char* getArgDesc(size_t index)
	{
		if (index == 0)
			return "Src Mcf";

		return "Dest Xml";
	}

	virtual const char* getFullArg()
	{
		return "savexml";
	}

	virtual const char getShortArg()
	{
		return 's';
	}

	virtual const char* getDescription()
	{
		return "Saves mcf xml to a file";
	}

	virtual int performAction(std::vector<std::string> &args)
	{
		MCFCore::MCFI* mcfSrc = mcfFactory();

		printf("Parsing MCF: %s\n", args[0].c_str());
		mcfSrc->setFile(args[0].c_str());
		mcfSrc->parseMCF();
		mcfSrc->saveXml(args[1].c_str());

		mcfDelFactory(mcfSrc);
		return 0;
	}
};


class UpdateMCF : public UtilFunction
{
public:
	virtual uint32 getNumArgs()
	{
		return 2;
	}

	virtual const char* getArgDesc(size_t index)
	{
		if (index == 0)
			return "Src Mcf";

		return "Dest Mcf";
	}

	virtual const char* getFullArg()
	{
		return "update";
	}

	virtual const char getShortArg()
	{
		return 'u';
	}

	virtual const char* getDescription()
	{
		return "Updates a mcf to newest mcf format";
	}

	virtual int performAction(std::vector<std::string> &args)
	{
		MCFCore::MCFI* mcfSrc = mcfFactory();
		
		printf("Parsing MCF: %s\n", args[0].c_str());
		mcfSrc->setFile(args[0].c_str());
		mcfSrc->parseMCF();

		printf("Exporting MCF: %s\n", args[1].c_str());
		mcfSrc->exportMcf(args[1].c_str());

		mcfDelFactory(mcfSrc);

		return 0;
	}
};


class OptimiseMCF : public UtilFunction
{
public:
	virtual uint32 getNumArgs()
	{
		return 3;
	}

	virtual const char* getArgDesc(size_t index)
	{
		if (index == 0)
			return "Last Build Src Mcf";

		if (index == 1)
			return "Current Build Src Mcf";

		return "Dest Mcf";
	}

	virtual const char* getFullArg()
	{
		return "optimise";
	}

	virtual const char getShortArg()
	{
		return 'o';
	}

	virtual const char* getDescription()
	{
		return "Moves all changed and new files to the start of the mcf";
	}

	virtual int performAction(std::vector<std::string> &args)
	{
		MCFCore::MCFI* mcfPrevSrc = mcfFactory();
		MCFCore::MCFI* mcfCurSrc = mcfFactory();

		printf("Parsing Prev MCF: %s\n", args[0].c_str());
		mcfPrevSrc->setFile(args[0].c_str());
		mcfPrevSrc->parseMCF();

		printf("Parsing Cur MCF: %s\n", args[1].c_str());
		mcfCurSrc->setFile(args[1].c_str());
		mcfCurSrc->parseMCF();

		printf("Exporting Optimised MCF: %s\n", args[2].c_str());
		mcfCurSrc->optimiseAndSaveMcf(mcfPrevSrc, args[2].c_str());

		mcfDelFactory(mcfPrevSrc);
		mcfDelFactory(mcfCurSrc);
		return 0;
	}
};

REG_FUNCTION(ExtractMCF)
REG_FUNCTION(SaveXML)
REG_FUNCTION(UpdateMCF)
REG_FUNCTION(OptimiseMCF)
