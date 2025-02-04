#include "FileManagerTreeBase.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <regex>
#include <mutex>
#include "../../thirdparty/cppfs/include/cppfs/fs.h"
#include "../../thirdparty/cppfs/include/cppfs/FileHandle.h"
#include "../../thirdparty/cppfs/include/cppfs/FileIterator.h"
#include "../../thirdparty/cppfs/include/cppfs/FilePath.h"

#ifdef _WINDOWS
#include "../../thirdparty/cppfs/include/cppfs/windows/LocalFileSystem.h"
#endif

#include "src/Utility/CommonFunc.h"
#include "Src/Utility/LoggerDef.h"
using namespace cppfs;

namespace DDRFramework
{

	FileManagerTreeBase::FileManagerTreeBase()
	{

		cppfs::FilePath path(DDRFramework::getexepath());
		m_RootPath = path.directoryPath();

		CheckFiles();

	}
	FileManagerTreeBase::~FileManagerTreeBase()
	{

	}

	void FileManagerTreeBase::SetRootPath(std::string root)
	{
		m_RootPath = root;
		CheckFiles();
	}


	std::string FileManagerTreeBase::GetRootPath()
	{
		return m_RootPath;
	}

	void FileManagerTreeBase::CheckDir(std::string dir, std::string file, std::vector<std::string>& vec, std::shared_ptr <treenode<std::string>> sptreenode)
	{
		std::string full = dir + "/" + file;
		cppfs::FileHandle fhandel = fs::open(full);
		if (fhandel.exists())
		{
			if (fhandel.isDirectory())
			{
				auto files = fhandel.listFiles();




				for (auto subfile : files)
				{
					auto sp = std::make_shared<treenode<std::string>>();
					sp->m_value = subfile;
					sp->m_parent = sptreenode;
					sptreenode->insert(sp);


					CheckDir(full , subfile, vec, sp);
				}
			}
			else
			{
				sptreenode->m_value = file;
				vec.push_back(dir);
			}

		}
	}

	void FileManagerTreeBase::MatchRelativeRoot(std::shared_ptr<treenode<std::string>> spnode, std::string format, std::vector<std::shared_ptr<treenode<std::string>>>& vec)
	{
		if (std::regex_match(spnode->m_value, std::regex(format)))
		{
			vec.push_back(spnode);
		}

		for (auto spleaf : spnode->m_leafmap)
		{
			MatchRelativeRoot(spleaf, format, vec);
		}
	}

	void FileManagerTreeBase::MatchFullPath(std::shared_ptr<treenode<std::string>> spnode, std::vector<string>& fmtvec, int level, std::vector<std::shared_ptr<treenode<std::string>>>& vec)
	{
		if (std::regex_match(spnode->m_value, std::regex(fmtvec[level])))
		{
			if (level == fmtvec.size() - 1)
			{
				vec.push_back(spnode);
				return;
			}
			else
			{
				for (auto spleaf : spnode->m_leafmap)
				{
					MatchFullPath(spleaf, fmtvec, level + 1, vec);
				}

			}

		}
	}

	std::vector<string> FileManagerTreeBase::CheckFiles()
	{

		std::lock_guard<std::mutex> lock(m_FileMutex);

		m_FileTree.create();

		std::vector<std::string> files;

		m_FileTree.m_spRoot->m_value = m_RootPath;
		m_FileTree.m_spRoot->m_parent = std::weak_ptr<treenode<std::string>>();
		CheckDir(m_RootPath, "", files,m_FileTree.m_spRoot);
		return files;

	}

	DDRFramework::tree<std::string>& FileManagerTreeBase::GetTree()
	{
		return m_FileTree;
	}

	void FileManagerTreeBase::PrintTreeNode(std::shared_ptr<treenode<std::string>> sptreenode, int level /*= 0*/)
	{

		std::lock_guard<std::mutex> lock(m_FileMutex);
		printf("\n");
		for (int i = 0; i < level; i++)
		{
			printf("    ");
		}
		printf(sptreenode->m_value.c_str());

		for (auto sp : sptreenode->m_leafmap)
		{
			PrintTreeNode(sp, level + 1);
		}
	}

	
	std::vector<std::string> FileManagerTreeBase::Match(std::string fmt)
	{
		std::lock_guard<std::mutex> lock(m_FileMutex);
		std::vector<std::shared_ptr<treenode<std::string>>> vec = MatchNode(fmt);
		std::vector<std::string> files;
		for (auto sp : vec)
		{
			std::string value;
			sp->getfull(value);

			value = DDRFramework::MBToUTF8String(value);

			files.push_back(value);
		}
		return files;
	}

	std::vector<std::string> FileManagerTreeBase::MatchDir(std::string dir, std::string fmt)
	{
		std::lock_guard<std::mutex> lock(m_FileMutex);
		std::string finalfmt = DDRFramework::getStarWildRegex(fmt, false);


		std::vector<std::string> vec;
		std::string full = m_RootPath + "/" + dir;
		cppfs::FileHandle fhandel = fs::open(full);
		if (fhandel.exists())
		{
			if (fhandel.isDirectory())
			{
				auto files = fhandel.listFiles();

				
				for (auto subfile : files)
				{
					if (std::regex_match(subfile, std::regex(finalfmt)))
					{
						auto value = dir + "/" + subfile;
						value = DDRFramework::MBToUTF8String(value);
						vec.push_back(value);
					}


				}
			}

		}
		return vec;
	}

	std::vector<std::shared_ptr<treenode<std::string>>> FileManagerTreeBase::MatchNode(std::string fmt)
	{
		auto& tree = GetTree();
		fmt = replace_all(fmt, "\\", "/");
		fmt = replace_all(fmt, "//", "/");
		auto vec = split(fmt, '/');

		std::vector<string> wildvec;
		int i = 0;
		for (auto s : vec)
		{
			std::string finalstr = DDRFramework::getStarWildRegex(s, i == 0 ? true : false);
			wildvec.push_back(finalstr);
			i++;
		}

		//find relative root
		std::vector<std::shared_ptr<treenode<std::string>>> matchrootnode;
		if (wildvec.size() > 0)
		{
			std::string rootfmt = wildvec[0];
			MatchRelativeRoot(tree.m_spRoot, rootfmt, matchrootnode);
		}


		std::vector<std::shared_ptr<treenode<std::string>>> matchfullnode;
		for (auto spNode : matchrootnode)
		{
			MatchFullPath(spNode, wildvec, 0, matchfullnode);
		}
		return matchfullnode;
	}




	std::string FileManagerTreeBase::HttpAddr2BaseDir(std::string httpaddr)
	{
		int index = httpaddr.find_first_of(':');
		index = httpaddr.find(':', index + 1);
		index = httpaddr.find('/', index + 1);

		std::string full = httpaddr.replace(httpaddr.begin(), httpaddr.begin() + index, m_RootPath);
		full = replace_all(full, "///", "/");
		full = replace_all(full, "//", "/");

		return full;
	}

	std::string FileManagerTreeBase::GetRelativeDirFromHttp(std::string httpaddr)
	{
		int index = httpaddr.find_first_of(':');
		index = httpaddr.find(':', index + 1);
		index = httpaddr.find('/', index + 1);

		std::string relativepath = httpaddr.replace(httpaddr.begin(), httpaddr.begin() + index, "");
		relativepath = replace_all(relativepath, "///", "/");
		relativepath = replace_all(relativepath, "//", "/");

		return relativepath;

	}	
	std::string FileManagerTreeBase::GetRelativeDirFromFull(std::string fullpath)
	{
		std::string relativepath = fullpath.replace(fullpath.begin(), fullpath.begin() + m_RootPath.length(), "");
		relativepath = replace_all(relativepath, "///", "/");
		relativepath = replace_all(relativepath, "//", "/");

		return relativepath;

	}
	std::string FileManagerTreeBase::GetFullDirFromRelative(std::string relativepath)
	{
		std::string full = m_RootPath + relativepath;	
		full = replace_all(relativepath, "//", "/");
		return full;
	}

	bool FileManagerTreeBase::FileExist(std::string url)
	{
		std::lock_guard<std::mutex> lock(m_FileMutex);
		std::string full = url;
		if (url.substr(0, 4) == "http")
		{
			full = HttpAddr2BaseDir(url);
		}

		cppfs::FileHandle fhandel = fs::open(full);
		if (fhandel.exists())
		{
			return true;
		}
		return false;
	}


}