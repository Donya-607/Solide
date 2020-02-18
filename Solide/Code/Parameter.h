#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Donya/Template.h"
#include "Donya/UseImGui.h"

class ParameterBase
{
protected:
	std::string name;
public:
	ParameterBase( const std::string &name ) : name( name ) {}
	virtual ~ParameterBase() = default;
public:
	virtual void Init()     = 0;
	virtual void Uninit()   = 0;
	std::string GetName() const
	{
		return name;
	}
protected:
	virtual void LoadBin()  = 0;
	virtual void LoadJson() = 0;
	virtual void SaveBin()  = 0;
	virtual void SaveJson() = 0;
public:
#if USE_IMGUI
	virtual void UseImGui() = 0;
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
	void Register( const std::string &registerName )
	{
		storage.insert
		(
			std::make_pair<std::string, std::unique_ptr<ParameterBase>>
			(
				registerName,
				std::make_unique<DerivedParameter>()
			)
		);
	}

	std::unique_ptr<ParameterBase> *Find( const std::string &keyName );
};
