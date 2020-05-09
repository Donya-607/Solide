#include "SaveData.h"

#include <algorithm>

#include "FilePath.h"
#include "StageNumberDefine.h"

bool SaveData::IsEmpty() const
{
	if ( pCurrentIntializer				) { return false; }
	if ( remainingCheckPoints.size()	) { return false; }
	if ( unlockedStageNumbers.size()	) { return false; }
	// else
	return true;
}
void SaveData::Clear()
{
	currentStageNumber = 0;
	pCurrentIntializer.reset();
	remainingCheckPoints.clear();
	unlockedStageNumbers.clear();
}

SaveDataAdmin::SaveDataAdmin() = default;

void SaveDataAdmin::Load()
{
#if DEBUG_MODE
	LoadJson();
#else
	LoadBin();
#endif // DEBUG_MODE

	InitializeIfDataIsEmpty();
}
void SaveDataAdmin::Save()
{
#if DEBUG_MODE
	SaveJson();
	SaveBin();
#else
	SaveBin();
#endif // DEBUG_MODE
}
void SaveDataAdmin::Clear()
{
	savedata.Clear();
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

bool SaveDataAdmin::IsEmptyCurrentData() const
{
	return savedata.IsEmpty();
}
bool SaveDataAdmin::IsUnlockedStageNumber( int stageNo ) const
{
	const auto &data	= savedata.unlockedStageNumbers;
	const auto itr		= std::find( data.begin(), data.end(), stageNo );
	return ( itr == data.end() ) ? false : true;
}

SaveData SaveDataAdmin::GetNowData() const
{
	return savedata;
}
void SaveDataAdmin::Write( const SaveData &updatedData )
{
	savedata = updatedData;
}
void SaveDataAdmin::Write( int nowStageNo )
{
	savedata.currentStageNumber = nowStageNo;
}
void SaveDataAdmin::Write( const PlayerInitializer &nowInitializer )
{
	*savedata.pCurrentIntializer = nowInitializer;
}
void SaveDataAdmin::Write( const CheckPoint &nowRemCheckPoints )
{
	auto &dest = savedata.remainingCheckPoints;
	dest.clear();

	const auto   &source    = nowRemCheckPoints;
	const size_t pointCount = source.GetPointCount();
	for ( size_t i = 0; i < pointCount; ++i )
	{
		const auto *pPoint = source.GetPointPtrOrNullptr( i );
		if ( !pPoint ) { continue; }
		// else

		dest.emplace_back( *pPoint );
	}
}
void SaveDataAdmin::UnlockStage( int unlockStageNo )
{
	if ( IsUnlockedStageNumber( unlockStageNo ) ) { return; }
	// else
	savedata.unlockedStageNumbers.emplace_back( unlockStageNo );
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

	ImGui::DragInt( u8"保存されたステージ番号", &savedata.currentStageNumber, 1.0f, -1 );
	const int stageNo = savedata.currentStageNumber;

	if ( ImGui::TreeNode( u8"自機の初期化情報" ) )
	{
		if ( ImGui::TreeNode( u8"適用されているもの" ) )
		{
			auto &ptr = savedata.pCurrentIntializer;

			if ( ptr )
			{
				ptr->ShowImGuiNode( u8"適用されている情報", stageNo, /* allowShowIONode = */ false );
			}
			else
			{
				const std::string buttonCaption{ u8"【適用されている情報】を作成" };
				if ( ImGui::Button( buttonCaption.c_str() ) )
				{
					ptr = std::make_shared<PlayerInitializer>();
				}
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"中間地点たち" ) )
		{
			auto &data = savedata.remainingCheckPoints;

			if ( ImGui::Button( u8"追加" ) )
			{
				data.emplace_back( CheckPoint::Instance{} );
			}
			if ( 1 <= data.size() && ImGui::Button( u8"末尾を削除" ) )
			{
				data.pop_back();
			}

			if ( ImGui::TreeNode( u8"実体たち" ) )
			{
				const size_t pointCount = data.size();
				size_t eraseIndex = pointCount;

				std::string caption{};
				for ( size_t i = 0; i < pointCount; ++i )
				{
					caption = u8"[" + std::to_string( i ) + u8"]番";

					if ( ImGui::Button( std::string{ caption + u8"を削除" }.c_str() ) )
					{
						eraseIndex = i;
					}

					data[i].ShowImGuiNode( caption, stageNo, /* useTreeNode = */ true );
				}

				if ( eraseIndex != pointCount )
				{
					data.erase( data.begin() + eraseIndex );
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}


		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"解放済みのステージたち" ) )
	{
		auto &data = savedata.unlockedStageNumbers;

		if ( ImGui::TreeNode( u8"対象の追加" ) )
		{
			static int additionNo = 0;
			ImGui::InputInt( u8"加えるステージ番号", &additionNo );

			if ( IsUnlockedStageNumber( additionNo ) )
			{
				ImGui::TextDisabled( u8"すでに存在する番号です" );
			}
			else
			{
				if ( ImGui::Button( u8"追加" ) )
				{
					data.emplace_back( additionNo );
				}
			}

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"中身" ) )
		{
			if ( ImGui::Button( u8"昇順にソート" ) )
			{
				std::sort( data.begin(), data.end(), std::less<int>() );
			}
			if ( ImGui::Button( u8"降順にソート" ) )
			{
				std::sort( data.begin(), data.end(), std::greater<int>() );
			}

			const size_t numberCount = data.size();
			size_t eraseIndex = numberCount;

			std::string caption{};
			for ( size_t i = 0; i < numberCount; ++i )
			{
				ImGui::DragInt( "", &data[i], 1.0f, -1 );

				ImGui::SameLine();
				if ( ImGui::Button( u8"これを削除" ) )
				{
					eraseIndex = i;
				}
			}

			if ( eraseIndex != numberCount )
			{
				data.erase( data.begin() + eraseIndex );
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI