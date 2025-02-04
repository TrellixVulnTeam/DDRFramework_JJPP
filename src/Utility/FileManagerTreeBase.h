/*!
 * File: FileManagerTreeBase.h
 * Date: 2019/04/29 16:01
 *
 * Author: michael
 * Contact: michael2345@live.cn
 *
 * Description:File System Manager Base
 *
*/
#ifndef FileManagerTreeBase_h__
#define FileManagerTreeBase_h__



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


	class FileManagerTreeBase
	{
	public:
		FileManagerTreeBase();
		~FileManagerTreeBase();

		void SetRootPath(std::string root);
		std::string GetRootPath();
		std::vector<std::string> CheckFiles();

		tree<std::string>& GetTree();


		void PrintTreeNode(std::shared_ptr<treenode<std::string>> sptreenode, int level = 0);

		//return utf8 file path
		std::vector<std::string> Match(std::string fmt);
		std::vector<std::string> MatchDir(std::string dir, std::string fmt);



		std::string HttpAddr2BaseDir(std::string httpaddr);
		std::string GetRelativeDirFromHttp(std::string httpaddr);
		std::string GetRelativeDirFromFull(std::string fullpath);
		std::string GetFullDirFromRelative(std::string relativepath);
		bool FileExist(std::string url);               

	protected:

		void CheckDir(std::string dir,std::string file, std::vector<std::string>& vec, std::shared_ptr<treenode<std::string>> sptreenode);



		std::vector<std::shared_ptr<treenode<std::string>>> MatchNode(std::string fmt);
		void MatchRelativeRoot(std::shared_ptr<treenode<std::string>> spnode, std::string format, std::vector<std::shared_ptr<treenode<std::string>>>& vec);
		void MatchFullPath(std::shared_ptr<treenode<std::string>> spnode, std::vector<string>& fmtvec, int level, std::vector<std::shared_ptr<treenode<std::string>>>& vec);

		std::string m_RootPath;
		tree<std::string> m_FileTree;


		std::mutex m_FileMutex;

	};
}
#endif // FileManagerTreeBase_h__
