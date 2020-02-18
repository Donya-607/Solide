#include "Parameter.h"

void ParameterStorage::Reset()
{
	storage.clear();
}

std::unique_ptr<ParameterBase> *ParameterStorage::Find( const std::string &keyName )
{
	auto find = storage.find( keyName );
	return ( find == storage.end() ) ? nullptr : &( find->second );
}
