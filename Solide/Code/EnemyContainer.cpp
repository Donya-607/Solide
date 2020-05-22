#include "EnemyContainer.h"

#include <algorithm>		// Use std::sort().

#if USE_IMGUI
#include "Donya/Useful.h"	// Convert the character codes.
#endif // USE_IMGUI

#include "FilePath.h"


namespace
{
#if USE_IMGUI
	bool wantPauseUpdates = false;
#endif // USE_IMGUI

}


namespace Enemy
{
	void Container::Init( int stageNumber )
	{
		stageNo = stageNumber;
	#if DEBUG_MODE
		LoadJson( stageNo );
	#else
		LoadBin( stageNo );
	#endif // DEBUG_MODE

		// If was loaded valid data.
		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->Init( pIt->GetInitializer() );
		}
	}
	void Container::Uninit()
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->Uninit();
		}
	}

	void Container::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
	#if USE_IMGUI
		if ( wantPauseUpdates ) { EraseEnemiesIfNeeded(); return; }
	#endif // USE_IMGUI

		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->Update( elapsedTime, targetPos );
		}

		EraseEnemiesIfNeeded();
	}
	void Container::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
	#if USE_IMGUI
		if ( wantPauseUpdates ) { return; }
	#endif // USE_IMGUI

		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->PhysicUpdate( solids, pTerrain, pTerrainMatrix );
		}
	}

	void Container::Draw( RenderingHelper *pRenderer )
	{
		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->Draw( pRenderer );
		}
	}
	void Container::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP )
	{
	#if DEBUG_MODE
		for ( auto &pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->DrawHitBox( pRenderer, VP );
		}
	#endif // DEBUG_MODE
	}

	void Container::AcquireHitBoxes( std::vector<Donya::AABB> *pAppendDest ) const
	{
		if ( !pAppendDest ) { return; }
		// else

		for ( const auto pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->AcquireHitBoxes( pAppendDest );
		}
	}
	void Container::AcquireHurtBoxes( std::vector<Donya::AABB> *pAppendDest ) const
	{
		if ( !pAppendDest ) { return; }
		// else

		for ( const auto pIt : enemyPtrs )
		{
			if ( !pIt ) { continue; }
			// else
			pIt->AcquireHurtBoxes( pAppendDest );
		}
	}

	size_t Container::GetEnemyCount() const
	{
		return enemyPtrs.size();
	}
	bool   Container::IsOutOfRange( size_t index ) const
	{
		return ( index < GetEnemyCount() ) ? false : true;
	}
	const  std::shared_ptr<Enemy::Base> Container::GetEnemyPtrOrNull( size_t index ) const
	{
		if ( IsOutOfRange( index ) ) { return nullptr; };
		// else
		return enemyPtrs[index];
	}

	void Container::EraseEnemiesIfNeeded()
	{
		auto result = std::remove_if
		(
			enemyPtrs.begin(), enemyPtrs.end(),
			[]( std::shared_ptr<Enemy::Base> &pElement )
			{
				return ( !pElement ) ? true : pElement->ShouldRemove();
			}
		);
		for ( auto it = result; it != enemyPtrs.end(); ++it )
		{
			if ( *it ) { ( *it )->Uninit(); }
		}
		enemyPtrs.erase( result, enemyPtrs.end() );
	}

	void Container::SortByDepth()
	{
		using ElementType = std::shared_ptr<Enemy::Base>;
		auto IsGreaterDepth = []( const ElementType &lhs, const ElementType &rhs )
		{
			return ( rhs->GetPosition().z < lhs->GetPosition().z );
		};

		std::sort( enemyPtrs.begin(), enemyPtrs.end(), IsGreaterDepth );
	}

	void Container::LoadBin ( int stageNumber )
	{
		constexpr bool fromBinary = true;
		Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNumber, fromBinary ).c_str(), ID, fromBinary );
	}
	void Container::LoadJson( int stageNumber )
	{
		constexpr bool fromBinary = false;
		Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNumber, fromBinary ).c_str(), ID, fromBinary );
	}
	#if USE_IMGUI
	void Container::SaveBin( int stageNumber )
	{
		constexpr bool fromBinary = true;

		const std::string filePath = MakeStageParamPath( ID, stageNumber, fromBinary );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
	}
	void Container::SaveJson( int stageNumber )
	{
		constexpr bool fromBinary = false;

		const std::string filePath = MakeStageParamPath( ID, stageNumber, fromBinary );
		MakeFileIfNotExists( filePath, fromBinary );

		Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
	}
	void Container::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::Checkbox( u8"敵たちの更新を止める", &wantPauseUpdates );

		static int addKind = 0;
		ImGui::SliderInt( u8"追加する種類", &addKind, 0, scast<int>( Enemy::Kind::KindCount ) - 1 );
		ImGui::Text( u8"種類名：%s", Donya::MultiToUTF8( Enemy::GetKindName( scast<Enemy::Kind>( addKind ) ) ).c_str() );

		static Enemy::InitializeParam appendInitializer{};
		appendInitializer.ShowImGuiNode( u8"追加時に適用する値" );

		if ( ImGui::Button( u8"追加" ) )
		{
			std::shared_ptr<Enemy::Base> tmp{};
			Enemy::AssignDerivedInstance( scast<Kind>( addKind ), &tmp );
			if ( tmp )
			{
				enemyPtrs.emplace_back( std::move( tmp ) );
				enemyPtrs.back()->Init( appendInitializer );
			}
		}
		if ( 1 <= enemyPtrs.size() && ImGui::Button( u8"末尾を削除" ) )
		{
			enemyPtrs.pop_back();
		}
	
		ImGui::Text( "" );
		ImGui::Text( u8"ソートはセーブ時にも自動で行われます" );
		if ( ImGui::Button( u8"ソート" ) )
		{
			SortByDepth();
		}

		// ShowImGuiNode() loop.
		{
			const size_t enemyCount = enemyPtrs.size();
			std::string caption{};
			for ( size_t i = 0; i < enemyCount; ++i )
			{
				if ( !enemyPtrs[i] ) { continue; }
				// else

				caption = u8"[" + std::to_string( i ) + u8"：" + Enemy::GetKindName( enemyPtrs[i]->GetKind() ) + u8"]";

				enemyPtrs[i]->ShowImGuiNode( caption );
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
				SortByDepth();

				SaveBin ( stageNo );
				SaveJson( stageNo );
			}
			if ( ImGui::Button( loadStr.c_str() ) )
			{
				for ( auto &pIt : enemyPtrs ) { if ( pIt ) { pIt->Uninit(); } }
				enemyPtrs.clear();

				( isBinary ) ? LoadBin( stageNo ) : LoadJson( stageNo );

				for ( auto &pIt : enemyPtrs ) { if ( pIt ) { pIt->Init( pIt->GetInitializer() ); } }
			}

			ImGui::TreePop();
		};
		ShowIONode();

		ImGui::TreePop();
	}
	#endif // USE_IMGUI
}
