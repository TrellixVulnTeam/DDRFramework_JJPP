/*!
 * File: FileManagerBase.h
 * Date: 2019/04/29 16:01
 *
 * Author: michael
 * Contact: michael2345@live.cn
 *
 * Description:File System Manager Base
 *
*/
#ifndef FileManagerBase_h__
#define FileManagerBase_h__



#include <vector>
#include <string>
#include "Singleton.h"
#include <set>
#include "LoggerDef.h"
#include "CommonFunc.h"
#include <mutex>

namespace DDRFramework
{

	template <class T>
	class treenode
	{
	public:
		treenode()
		{

		}
		~treenode()
		{
			for (auto sptreenode : m_leafmap)
			{
				sptreenode.reset();
			}
			m_leafmap.clear();
		}

		T m_value;
		std::weak_ptr<treenode<T>> m_parent;
		std::set<std::shared_ptr<treenode<T>>> m_leafmap;

		// insertion logic for if an insert is asked of me.
		// may append to children, or may pass off to one of the child nodes
		void insert(std::shared_ptr<treenode<T>> value) {

			if (m_leafmap.find(value) == m_leafmap.end())
			{
				m_leafmap.insert(value);
			}
		}
		
		void getfull(T& fullvalue)
		{
			fullvalue = m_value;
			getfulldeep(m_parent, fullvalue);

			fullvalue = replace_all(fullvalue, "\\\\", "/");
			fullvalue = replace_all(fullvalue, "\\", "/");
			fullvalue = replace_all(fullvalue, "///", "/");
			fullvalue = replace_all(fullvalue, "//", "/");

		}
		void getfulldeep(std::weak_ptr<treenode<T>> parentnode, T& value)
		{
			if (parentnode.lock())
			{
				value = parentnode.lock()->m_value + "/" + value;
				getfulldeep(parentnode.lock()->m_parent, value);
			}
			else
			{

			}
		}
	};


	template <class T>
	class tree
	{
	public:
		tree()
		{
		}
		~tree()
		{
			clear();
		}
		std::shared_ptr<treenode<T>> m_spRoot;

		void create()
		{

			if (m_spRoot)
			{
				clear();
			}
			else
			{

			}

			m_spRoot = std::make_shared<treenode<T>>();
		}

		void clear()
		{
			if (m_spRoot)
			{
				m_spRoot.reset();

			}
		}

	};


	class FileManagerBase
	{
	public:
		FileManagerBase();
		~FileManagerBase();

		void SetRootPath(std::string root);
		std::string GetRootPath();

		//return utf8 file path
		std::vector<std::string> Match(std::string fmt,std::string root = "" );
		std::vector<std::string> MatchDir(std::string dir, std::string fmt);
		void SetIgnoreMatchDir(std::set<string> ignoreList);



		std::string HttpAddr2BaseDir(std::string httpaddr);
		std::string GetRelativeDirFromHttp(std::string httpaddr);
		std::string GetRelativeDirFromFull(std::string fullpath);
		std::string GetFullDirFromRelative(std::string relativepath);
		bool FileExist(std::string url);               

	protected:

		void MatchNode(std::string dir, std::string finalfmt,int level,std::vector<string> &vec);

		std::string m_RootPath;

		std::mutex m_FileMutex;

		std::set<string> m_IgnoreList;
	};
}
#endif // FileManagerBase_h__
