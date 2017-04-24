#ifndef SAFEMAP_H
#define SAFEMAP_H

#include <KBASE/CritSec.h>
#include <KBASE/AutoLock.h>
#include <map>

template<typename KeyType,typename ValueType>
class SafeMap
{
public:
	SafeMap(){}
	virtual ~SafeMap(){}

	typedef std::map<KeyType,ValueType> KEY_VALUE_MAP;

	void ReSet()
	{
		m_it=m_Map.begin();
	}
	bool GetNext(ValueType & value)
	{
		KAutoLock l(m_CritSec);
		if (m_it==m_Map.end())
		{
			return false;
		}
		value=m_it->second;
		m_it++;
		return true;
	}
	void DeleteFromKey(const KeyType & nKey)
	{
		KAutoLock l(m_CritSec);
		typename KEY_VALUE_MAP::iterator it=m_Map.find(nKey);
		if (it!=m_Map.end())
		{
			if (it==m_it)
			{
				m_it++;
			}
			m_Map.erase(it);
		}
	}
	void AddValue(const KeyType & nKey,const ValueType & value)
	{
		KAutoLock l(m_CritSec);
		m_Map[nKey]=value;
	}

private:
	KEY_VALUE_MAP m_Map;
	typename KEY_VALUE_MAP::iterator m_it;
	KCritSec m_CritSec;
};

#endif
