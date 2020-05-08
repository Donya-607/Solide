#include "SaveData.h"

#include "FilePath.h"
#include "StageNumberDefine.h"

bool SaveData::IsEmpty() const
{
	if ( pCurrentIntializer				) { return false; }
	if ( remainingInitializers.size()	) { return false; }
	if ( unlockedStageNumbers.size()	) { return false; }
	// else
	return true;
}
void SaveData::ClearData()
{
	currentStageNumber = 0;
	pCurrentIntializer.reset();
	remainingInitializers.clear();
	unlockedStageNumbers.clear();
}


SaveDataAdmin::SaveDataAdmin() = default;

void SaveDataAdmin::LoadData()
{
#if DEBUG_MODE
	LoadJson();
#else
	LoadBin();
#endif // DEBUG_MODE

	InitializeIfDataIsEmpty();
}
void SaveDataAdmin::SaveData()
{
#if DEBUG_MODE
	SaveJson();
	SaveBin();
#else
	SaveBin();
#endif // DEBUG_MODE
}

void SaveDataAdmin::InitializeIfDataIsEmpty()
{
	if ( !savedata.IsEmpty() ) { return; }
	// else

	savedata.currentStageNumber = 0;
	savedata.unlockedStageNumbers.emplace_back( TITLE_STAGE_NO	);
	savedata.unlockedStageNumbers.emplace_back( SELECT_STAGE_NO	);
	savedata.unlockedStageNumbers.emplace_back( FIRST_STAGE_NO	);
}

void SaveDataAdmin::LoadBin()
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( savedata, MakeSaveDataPath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void SaveDataAdmin::LoadJson()
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( savedata, MakeSaveDataPath( ID, fromBinary ).c_str(), ID, fromBinary );
}
void SaveDataAdmin::SaveBin()
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeSaveDataPath( ID, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( savedata, filePath.c_str(), ID, fromBinary );
}
void SaveDataAdmin::SaveJson()
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeSaveDataPath( ID, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( savedata, filePath.c_str(), ID, fromBinary );
}

#if USE_IMGUI
void SaveDataAdmin::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else



	ImGui::TreePop();
}
#endif // USE_IMGUI