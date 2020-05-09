#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Serializer.h"
#include "Donya/Template.h"
#include "Donya/UseImGui.h"

#include "CheckPoint.h"

struct SaveData
{
	bool								isEmpty				= true;	// True is indicates a value of this save data is not changes yet.
	int									currentStageNumber	= 0;
	std::shared_ptr<PlayerInitializer>	pCurrentIntializer;
	std::vector<CheckPoint::Instance>	remainingCheckPoints;
	std::vector<int>					unlockedStageNumbers;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( isEmpty					),
			CEREAL_NVP( currentStageNumber		),
			CEREAL_NVP( pCurrentIntializer		),
			CEREAL_NVP( remainingCheckPoints	),
			CEREAL_NVP( unlockedStageNumbers	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Clear();
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
	void Load();
	void Save();
	void Clear();
	void InitializeIfDataIsEmpty();
public:
	bool IsEmptyCurrentData() const;
	bool IsUnlockedStageNumber( int stageNo ) const;
public:
	SaveData GetNowData() const;
	void Write( const SaveData &overwriteData );
	void Write( int currentStageNumber );
	void Write( const PlayerInitializer &currentInitializer );
	void Write( const CheckPoint &remainingCheckPoints );
	void UnlockStage( int unlockStageNumber );
private:
	void LoadBin();
	void LoadJson();
	void SaveBin();
	void SaveJson();
#if USE_IMGUI
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
