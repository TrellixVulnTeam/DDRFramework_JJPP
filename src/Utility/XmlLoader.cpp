#include "XmlLoader.h"

#include "Logger.h"
#include "tinyxml.h"

namespace DDRFramework
{
	XmlLoader::XmlLoader(std::string fileName) :m_FileName(fileName)
	{
		try
		{

			TiXmlDocument doc(m_FileName.c_str());
			bool loadOkay = doc.LoadFile();

			if (!loadOkay)
			{
				LevelLog(DDRFramework::Log::Level::ERR,"Could not load test file %s. Error='%s'. Exiting.\n", fileName.c_str(), doc.ErrorDesc());
				//exit(1);
			}

			TiXmlNode* rootNode = 0;
			TiXmlNode* sheetNode = 0;
			TiXmlElement* rootElement = 0;
			TiXmlElement* sheetElement = 0;
			TiXmlElement* itemElement = 0;

			rootNode = doc.FirstChild("Root");
			if (!rootNode)
			{
				throw std::exception();
			}

			rootElement = rootNode->ToElement();

			bool firstSheet = true;
			for (sheetNode = rootElement->FirstChild(); sheetNode != 0; sheetNode = sheetNode->NextSibling())
			{
				sheetElement = sheetNode->ToElement();


				std::shared_ptr<KVMapVector> spVector = std::make_shared<KVMapVector>();
				m_SheetMap[sheetElement->Value()] = spVector;
				if (firstSheet)
				{
					m_DefaultSheetName = sheetElement->Value();
					firstSheet = false;
				}

				std::shared_ptr<KeyVector> spColumnKeyVector = std::make_shared<KeyVector>();
				m_ColumnKeyMap[sheetElement->Value()] = spColumnKeyVector;


				std::shared_ptr<KeyIndexMap> spRowKeyIndexMap = std::make_shared<KeyIndexMap>();
				m_RowKeyIndexMap[sheetElement->Value()] = spRowKeyIndexMap;
				std::shared_ptr<IndexKeyMap> spRowIndexKeyMap = std::make_shared<IndexKeyMap>();
				m_RowIndexKeyMap[sheetElement->Value()] = spRowIndexKeyMap;



				TiXmlElement* itemElement = 0;
				TiXmlNode* itemNode = 0;

				bool firstElement = true;
				int rownum = 0;
				for (itemNode = sheetElement->FirstChild(); itemNode != 0; itemNode = itemNode->NextSibling())
				{
					itemElement = itemNode->ToElement();

					spRowKeyIndexMap->insert(std::make_pair(itemElement->Attribute("Key"), rownum));
					spRowIndexKeyMap->insert(std::make_pair(rownum, itemElement->Attribute("Key")));



					std::shared_ptr<KVMap> spMap = std::make_shared<KVMap>();
					TiXmlAttribute* attr;
					for (attr = itemElement->FirstAttribute(); attr != 0; attr = attr->Next())
					{

						spMap->insert(std::pair<std::string, std::string>(attr->Name(), attr->Value()));

						if (firstElement)
						{

							spColumnKeyVector->push_back(attr->Name());
						}

					}
					firstElement = false;

					spVector->push_back(spMap);


					rownum++;
				}

			}
		}
		catch (std::exception& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"XmlLoader Construct Failed : %s -- %s", fileName.c_str(), e.what());
		}
		catch (...)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"XmlLoader Construct Failed : %s", fileName.c_str());
		}
	}

	XmlLoader::~XmlLoader()
	{
	}

	std::string XmlLoader::GetValue(std::string sheet, int count, std::string key)
	{
		try {
			if (m_SheetMap.find(sheet) != m_SheetMap.end() && m_SheetMap[sheet]->size() > count && m_SheetMap[sheet]->at(count)->find(key) != m_SheetMap[sheet]->at(count)->end())
			{
				std::string s = m_SheetMap[sheet]->at(count)->at(key);
				return s;
			}
		}
		catch (std::exception& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"%s", e.what());
		}
		return "";
	}

	std::string XmlLoader::GetDefaultSheetName()
	{
		return m_DefaultSheetName;
	}


	std::string XmlLoader::GetValue(int count, std::string key)
	{
		try {
			if (m_SheetMap.find(m_DefaultSheetName) != m_SheetMap.end() && m_SheetMap[m_DefaultSheetName]->size() > count && m_SheetMap[m_DefaultSheetName]->at(count)->find(key) != m_SheetMap[m_DefaultSheetName]->at(count)->end())
			{
				std::string s = m_SheetMap[m_DefaultSheetName]->at(count)->at(key);
				return s;
			}
		}
		catch (std::exception& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"%s", e.what());
		}
		return "";
	}

	
	
	std::string XmlLoader::GetValue(std::string sheet,std::string key)
	{
		try
		{
			if (m_RowKeyIndexMap.find(sheet) != m_RowKeyIndexMap.end() && m_RowKeyIndexMap[sheet]->find(key) != m_RowKeyIndexMap[sheet]->end())
			{
				int rowkeyindex = m_RowKeyIndexMap[sheet]->at(key);

				if (m_SheetMap.find(sheet) != m_SheetMap.end() && m_SheetMap[sheet]->size() > rowkeyindex && m_SheetMap[sheet]->at(rowkeyindex)->find("Value")  != m_SheetMap[sheet]->at(rowkeyindex)->end())
				{
					std::string s = m_SheetMap[sheet]->at(rowkeyindex)->at("Value");
					return s;
				}
			}

		}
		catch (std::exception& e)
		{
			LevelLog(DDRFramework::Log::Level::ERR,"%s", e.what());
		}
		return "";

	}

	std::string XmlLoader::GetValue(std::string key)
	{
		std::string s = GetValue(m_DefaultSheetName, key);
		return s;

	}




	std::string XmlLoader::GetRCValue(std::string rowkey, std::string colkey)
	{
		return GetRCValue(m_DefaultSheetName, rowkey, colkey);
	}

	std::string XmlLoader::GetRCValue(std::string sheet, std::string rowkey, std::string colkey)
	{
		int index = RowGetKeyIndex(sheet, rowkey);
		if (index != -1)
		{
			return GetValue(index, colkey);
		}
		return "";
	}

	int XmlLoader::GetElementCount(std::string sheet)
	{
		return  m_SheetMap[sheet]->size();
	}
	int XmlLoader::GetElementCount()
	{
		return  GetElementCount(m_DefaultSheetName);
	}

	 std::string XmlLoader::GetColumnKey(int count)
	{
		return GetColumnKey(m_DefaultSheetName,count);

	}
	std::string XmlLoader::GetColumnKey(std::string sheet, int count)
	{
		return m_ColumnKeyMap[sheet]->at(count);
	}

	int XmlLoader::ColumnGetKeyCount()
	{
		return ColumnGetKeyCount(m_DefaultSheetName);
	}
	int XmlLoader::ColumnGetKeyCount(std::string sheet)
	{
		return m_ColumnKeyMap[sheet]->size();
	}


	std::string XmlLoader::GetRowKey(int count)
	{
		return GetRowKey(m_DefaultSheetName, count);

	}
	std::string XmlLoader::GetRowKey(std::string sheet, int count)
	{
		return m_RowIndexKeyMap[sheet]->at(count);
	}
	int XmlLoader::RowGetKeyCount()
	{
		return RowGetKeyCount(m_DefaultSheetName);
	}
	int XmlLoader::RowGetKeyCount(std::string sheet)
	{
		return m_RowIndexKeyMap[sheet]->size();
	}



	void XmlLoader::SetValue(std::string sheet, int count, std::string key, std::string value)
	{
		m_SheetMap[sheet]->at(count)->at(key) = value;
	}
	void XmlLoader::SetValue(int count, std::string key, std::string value)
	{
		SetValue(m_DefaultSheetName, count, key, value);

	}
	void XmlLoader::SetValue(std::string key, std::string value)
	{
		std::string v = "Value";
		SetValue(m_RowKeyIndexMap[m_DefaultSheetName]->at(key), v, value);

	}
	void XmlLoader::DoSave()
	{
		if (!m_FileName.empty())
		{
			DoSave(m_FileName);

		}
	}
	void XmlLoader::DoSave(std::string filename)
	{
		TiXmlDocument doc;

		TiXmlNode* rootNode = new TiXmlElement("Root");
		TiXmlNode* sheetNode = 0;
		TiXmlElement* rootElement = 0;
		TiXmlElement* sheetElement = 0;
		TiXmlElement* itemElement = 0;

		rootElement = rootNode->ToElement();

		bool firstSheet = true;
		for (auto sheetMapPair : m_SheetMap)
		{
			sheetNode = new TiXmlElement(sheetMapPair.first.c_str());

			auto elementVec = sheetMapPair.second;
			for (auto elementVecIter : *elementVec)
			{
				itemElement = new TiXmlElement("Element");

				for (auto attrMapPair : *elementVecIter)
				{
					std::string key = attrMapPair.first;
					std::string value = attrMapPair.second;
					itemElement->SetAttribute(key.c_str(), value.c_str());
				}

				sheetNode->LinkEndChild(itemElement);
			}
			rootElement->LinkEndChild(sheetNode);
		}
		doc.LinkEndChild(rootNode);
		doc.SaveFile(filename.c_str());
	}
}
