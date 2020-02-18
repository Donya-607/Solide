#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Donya/Template.h"
#include "Donya/UseImGui.h"

class ParameterBase
{
public:
	virtual void Init()     = 0;
	virtual void Uninit()   = 0;
protected:
	virtual void LoadBin()  = 0;
	virtual void LoadJson() = 0;
	virtual void SaveBin()  = 0;
	virtual void SaveJson() = 0;
public:
#if USE_IMGUI
	virtual void UseImGui() = 0;

	static void ShowIONode( ParameterBase *pParam );
#endif // USE_IMGUI
};

class ParameterStorage : public Donya::Singleton<ParameterStorage>
{
	friend Donya::Singleton<ParameterStorage>;
private:
	std::unordered_map<std::string, std::unique_ptr<ParameterBase>> storage;
public:
	void Reset();

	template<class DerivedParameter>
	void Register( std::string registerName )
	{
		storage.insert
		(
			std::make_pair<std::string, std::unique_ptr<ParameterBase>>
			(
				std::move( registerName ),
				std::make_unique<DerivedParameter>()
			)
		);
	}

	std::unique_ptr<ParameterBase> *Find( const std::string &keyName );
};
