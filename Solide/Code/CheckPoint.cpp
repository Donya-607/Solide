#include "CheckPoint.h"

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"
#include "SaveData.h"

Donya::AABB			CheckPoint::Instance::GetHitBox() const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += initializer.GetInitialPos();
	return tmp;
}
PlayerInitializer	CheckPoint::Instance::GetInitializer() const
{
	return initializer;
}
#if USE_IMGUI
void CheckPoint::Instance::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool useTreeNode )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );
	initializer.ShowImGuiNode( u8"初期化情報", stageNo, /* allowShowIONode = */ false );

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI

void CheckPoint::Init( int stageNumber )
{
	stageNo = stageNumber;
#if DEBUG_MODE
	LoadJson( stageNumber );
#else
	LoadBin( stageNumber );
#endif // DEBUG_MODE
}
void CheckPoint::Init( const SaveData &loadedData, int stageNumber )
{
	stageNo = stageNumber;

	points.clear();
	points = loadedData.remainingCheckPoints;
}
void CheckPoint::Uninit() {}
void CheckPoint::Update( float elapsedTime ) {}
void CheckPoint::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	if ( !pRenderer ) { return; }
	// else

	// No op.
}
void CheckPoint::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

	constexpr Donya::Vector4 blendColor{ 1.0f, 1.0f, 0.0f, 1.0f };

	auto DrawCube = [&]( const Donya::AABB &box )
	{
		Donya::Vector4x4 W{};
		W._11 = box.size.x * 2.0f;
		W._22 = box.size.y * 2.0f;
		W._33 = box.size.z * 2.0f;
		W._41 = box.pos.x;
		W._42 = box.pos.y;
		W._43 = box.pos.z;

		Donya::Model::Cube::Constant constant;
		constant.matWorld		= W;
		constant.matViewProj	= matVP;
		constant.drawColor		= blendColor.Product( color );
		constant.lightDirection	= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );
	};

	for ( auto &it : points )
	{
		DrawCube( it.GetHitBox() );
	}
}

size_t CheckPoint::GetPointCount() const { return points.size(); }
bool  CheckPoint::IsOutOfRange( size_t index ) const
{
	return ( GetPointCount() <= index ) ? true : false;
}
const CheckPoint::Instance *CheckPoint::GetPointPtrOrNullptr( size_t index ) const
{
	if ( IsOutOfRange( index ) ) { return nullptr; }
	// else
	return &points[index];
}
void  CheckPoint::RemovePoint( size_t index )
{
	if ( IsOutOfRange( index ) ) { return; }
	// else

	points.erase( points.begin() + index );
}

void CheckPoint::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void CheckPoint::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void CheckPoint::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void CheckPoint::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void CheckPoint::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"チェックポイントを追加" ) )
	{
		points.emplace_back( Instance{} );
	}
	if ( 1 < points.size() && ImGui::Button( u8"末尾を削除" ) )
	{
		points.pop_back();
	}

	if ( ImGui::TreeNode( u8"各々の設定" ) )
	{
		const size_t pointCount = GetPointCount();
		size_t eraseIndex = pointCount;

		std::string caption{};
		for ( size_t i = 0; i < pointCount; ++i )
		{
			caption = u8"[" + std::to_string( i ) + u8"]番";
			if ( ImGui::TreeNode( caption.c_str() ) )
			{
				if ( ImGui::Button( std::string{ caption + u8"を削除" }.c_str() ) )
				{
					eraseIndex = i;
				}

				points[i].ShowImGuiNode( "", stageNo, /* useTreeNode = */ false );
				
				ImGui::TreePop();
			}
		}

		if ( eraseIndex != pointCount )
		{
			points.erase( points.begin() + eraseIndex );
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
		loadStr += u8"(by:";
		loadStr += ( isBinary ) ? u8"Binary" : u8"Json";
		loadStr += u8")";

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

