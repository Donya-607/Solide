#include "ObstacleContainer.h"

#if USE_IMGUI
#include "Donya/Useful.h" // Convert the character codes.
#endif // USE_IMGUI

#include "FilePath.h"

void ObstacleContainer::Init( int stageNumber )
{
	stageNo = stageNumber;
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE

	// If was loaded valid data.
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Init( pIt->GetPosition() );
	}
}
void ObstacleContainer::Uninit()
{
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Uninit();
	}
}

void ObstacleContainer::Update( float elapsedTime )
{
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Update( elapsedTime );
	}
}

void ObstacleContainer::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Draw( eyePos, transNear, transFar, transLowerAlpha, VP, lightDir, color );
	}
}

std::vector<Donya::AABB> ObstacleContainer::GetHitBoxes() const
{
	std::vector<Donya::AABB> hitBoxes{};
	for ( const auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else

		hitBoxes.emplace_back( pIt->GetHitBox() );
	}

	return hitBoxes;
}

std::string ObstacleContainer::MakeSerializePath( int stageNumber, bool fromBinary ) const
{
	const std::string postfix = "[" + std::to_string( stageNumber ) + "]";
	return GenerateSerializePath( ID + postfix, fromBinary );
}

void ObstacleContainer::LoadBin ( int stageNumber )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeSerializePath( stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
void ObstacleContainer::LoadJson( int stageNumber )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeSerializePath( stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void ObstacleContainer::SaveBin( int stageNumber )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Save( *this, MakeSerializePath( stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
void ObstacleContainer::SaveJson( int stageNumber )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Save( *this, MakeSerializePath( stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
void ObstacleContainer::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	static int addKind = 0;
	ImGui::SliderInt( u8"追加する種類", &addKind, 0, ObstacleBase::GetModelKindCount() - 1 );
	ImGui::Text( u8"種類名：%s", Donya::MultiToUTF8( ObstacleBase::GetModelName( addKind ) ).c_str() );

	auto &data = pObstacles;
	if ( ImGui::Button( u8"追加" ) )
	{
		std::shared_ptr<ObstacleBase> tmp{};

		ObstacleBase::AssignDerivedModel( addKind, &tmp );

		if ( tmp )
		{
			data.push_back( std::move( tmp ) );
		}
	}
	if ( 1 <= data.size() && ImGui::Button( u8"末尾を削除" ) )
	{
		data.pop_back();
	}

	{
		const size_t count = data.size();
		size_t removeIndex = count;
		std::string caption{};
		for ( size_t i = 0; i < count; ++i )
		{
			if ( !data[i] ) { continue; }
			// else

			caption = u8"[" + std::to_string( i ) + u8"：" + ObstacleBase::GetModelName( data[i]->GetKind() ) + u8"]";

			if ( data[i]->ShowImGuiNode( caption ) )
			{
				removeIndex = i;
			}
		}

		if ( removeIndex != count )
		{
			data.erase( data.begin() + removeIndex );
		}
	}

	auto ShowIONode = [&]()
	{
		if ( !ImGui::TreeNode( u8"ファイル I/O" ) ) { return; }
		// else

		const std::string strIndex = u8"[" + std::to_string( stageNo ) + u8"]";

		static bool isBinary = true;
		if ( ImGui::RadioButton( "Binary", isBinary ) ) { isBinary = true;  }
		if ( ImGui::RadioButton( "Json",  !isBinary ) ) { isBinary = false; }

		std::string loadStr = u8"ロード" + strIndex;
		loadStr += u8"（by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8"）";

		if ( ImGui::Button( ( u8"セーブ" + strIndex ).c_str() ) )
		{
			SaveBin ( stageNo );
			SaveJson( stageNo );
		}
		if ( ImGui::Button( loadStr.c_str() ) )
		{
			( isBinary ) ? LoadBin( stageNo ) : LoadJson( stageNo );
		}

		ImGui::TreePop();
	};

	ShowIONode();

	ImGui::TreePop();
}
#endif // USE_IMGUI
