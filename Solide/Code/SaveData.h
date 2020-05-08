#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Serializer.h"
#include "Donya/Template.h"
#include "Donya/UseImGui.h"

#include "Player.h"	// Use PlayerInitializer only.

struct SaveData
{
	int									currentStageNumber = 0;
	std::unique_ptr<PlayerInitializer>	pCurrentIntializer;
	std::vector<PlayerInitializer>		remainingInitializers;
	std::vector<int>					unlockedStageNumbers;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( currentStageNumber		),
			CEREAL_NVP( pCurrentIntializer		),
			CEREAL_NVP( remainingInitializers	),
			CEREAL_NVP( unlockedStageNumbers	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	bool IsEmpty() const;
	void ClearData();
};


class SaveDataAdmin : public Donya::Singleton<SaveDataAdmin>
{
	friend Donya::Singleton<SaveDataAdmin>;
private:
	static constexpr const char *ID = "SaveData";
	SaveData savedata;
private:
	SaveDataAdmin();
public:
	void LoadData();
	void SaveData();
private:
	void InitializeIfDataIsEmpty();
private:
	void LoadBin();
	void LoadJson();
	void SaveBin();
	void SaveJson();
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
