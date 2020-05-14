#include "ObstacleContainer.h"

#include <algorithm>	// For sort by depth.

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

	auto result = std::remove_if
	(
		pObstacles.begin(), pObstacles.end(),
		[]( std::shared_ptr<ObstacleBase> &pElement )
		{
			return ( !pElement ) ? true : pElement->ShouldRemove();
		}
	);
	for ( auto it = result; it != pObstacles.end(); ++it )
	{
		if ( *it ) { ( *it )->Uninit(); }
	}
	pObstacles.erase( result, pObstacles.end() );
}

void ObstacleContainer::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->Draw( pRenderer, color );
	}
}
void ObstacleContainer::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
{
#if DEBUG_MODE
	for ( auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else
		pIt->DrawHitBox( pRenderer, VP, color );
	}
#endif // DEBUG_MODE
}

void ObstacleContainer::SortByDepth()
{
	using ElementType = std::shared_ptr<ObstacleBase>;
	auto IsGreaterDepth = []( const ElementType &lhs, const ElementType &rhs )
	{
		return ( rhs->GetPosition().z < lhs->GetPosition().z );
	};

	std::sort( pObstacles.begin(), pObstacles.end(), IsGreaterDepth );
}

void ObstacleContainer::GenerateHardenedBlock( const Donya::Vector3 &wsGenPos )
{
	const int kindCount = ObstacleBase::GetModelKindCount();
	for ( int i = 0; i < kindCount; ++i )
	{
		if ( !ObstacleBase::IsHardenedKind( i ) ) { continue; }
		// else

		std::shared_ptr<ObstacleBase> tmp{};
		ObstacleBase::AssignDerivedModel( i, &tmp );

		if ( tmp )
		{
			pObstacles.push_back( std::move( tmp ) );
			pObstacles.back()->Init( wsGenPos );
		}

		break;
	}
}

size_t	ObstacleContainer::GetObstacleCount() const
{
	return pObstacles.size();
}
bool	ObstacleContainer::IsOutOfRange( size_t index ) const
{
	return ( GetObstacleCount() <= index ) ? true : false;
}
std::shared_ptr<ObstacleBase> ObstacleContainer::GetObstaclePtrOrNullptr( size_t index ) const
{
	if ( IsOutOfRange( index ) ) { return nullptr; }
	// else
	return pObstacles[index];
}

std::vector<Donya::AABB> ObstacleContainer::GetHitBoxes() const
{
	std::vector<Donya::AABB> hitBoxes{};
	for ( const auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else

		if ( ObstacleBase::IsWaterKind( pIt->GetKind() ) ) { continue; }
		// else

		hitBoxes.emplace_back( pIt->GetHitBox() );
	}

	return hitBoxes;
}
std::vector<Donya::AABB> ObstacleContainer::GetWaterHitBoxes() const
{
	std::vector<Donya::AABB> hitBoxes{};
	for ( const auto &pIt : pObstacles )
	{
		if ( !pIt ) { continue; }
		// else

		if ( !ObstacleBase::IsWaterKind( pIt->GetKind() ) ) { continue; }
		// else

		hitBoxes.emplace_back( pIt->GetHitBox() );
	}

	return hitBoxes;
}

void ObstacleContainer::LoadBin ( int stageNumber )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
void ObstacleContainer::LoadJson( int stageNumber )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNumber, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void ObstacleContainer::SaveBin( int stageNumber )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNumber, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void ObstacleContainer::SaveJson( int stageNumber )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNumber, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
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
	
	ImGui::Text( "" );
	ImGui::Text( u8"ソートはセーブ時にも自動で行われます" );
	if ( ImGui::Button( u8"ソート" ) )
	{
		SortByDepth();
	}

	if ( ImGui::TreeNode( u8"実体たち" ) )
	{
		const size_t count = data.size();
		std::string caption{};
		for ( size_t i = 0; i < count; ++i )
		{
			if ( !data[i] ) { continue; }
			// else

			caption = u8"[" + std::to_string( i ) + u8"：" + ObstacleBase::GetModelName( data[i]->GetKind() ) + u8"]";

			data[i]->ShowImGuiNode( caption ) ;
		}

		ImGui::TreePop();
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
			SortByDepth();

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
