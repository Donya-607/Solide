#include "CameraOption.h"

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"

void CameraOption::Init( int stageNumber )
{
	stageNo = stageNumber;
#if DEBUG_MODE
	LoadJson( stageNumber );
#else
	LoadBin( stageNumber );
#endif // DEBUG_MODE
}
void CameraOption::Uninit() {}

void CameraOption::Visualize( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

	auto DrawCube = [&]( const Donya::Vector3 &pos )
	{
		Donya::Vector4x4 W{};
		W._41 = pos.x;
		W._42 = pos.y;
		W._43 = pos.z;

		Donya::Model::Cube::Constant constant;
		constant.matWorld		= W;
		constant.matViewProj	= matVP;
		constant.drawColor		= color;
		constant.lightDirection	= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );
	};

	for ( auto &it : options )
	{
		DrawCube( it.wsPos );
	}
}

size_t CameraOption::GetOptionCount() const { return options.size(); }
bool  CameraOption::IsOutOfRange( size_t index ) const
{
	return ( GetOptionCount() <= index ) ? true : false;
}
const CameraOption::Instance *CameraOption::GetOptionPtrOrNullptr( size_t index ) const
{
	if ( IsOutOfRange( index ) ) { return nullptr; }
	// else
	return &options[index];
}

void CameraOption::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void CameraOption::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void CameraOption::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void CameraOption::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void CameraOption::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"オプション地点を追加" ) )
	{
		options.emplace_back( Instance{} );
	}
	if ( 1 < options.size() && ImGui::Button( u8"末尾を削除" ) )
	{
		options.pop_back();
	}

	if ( ImGui::TreeNode( u8"各々の設定" ) )
	{
		const size_t optionCount = GetOptionCount();
		size_t eraseIndex = optionCount;

		std::string caption{};
		for ( size_t i = 0; i < optionCount; ++i )
		{
			caption = u8"[" + std::to_string( i ) + u8"]番";
			if ( ImGui::TreeNode( caption.c_str() ) )
			{
				if ( ImGui::Button( std::string{ caption + u8"を削除" }.c_str() ) )
				{
					eraseIndex = i;
				}

				ImGui::DragFloat3( u8"オプション地点",		&options[i].wsPos.x,		0.01f );
				ImGui::DragFloat3( u8"オフセット・座標",		&options[i].offsetPos.x,	0.01f );
				ImGui::DragFloat3( u8"オフセット・注視点",	&options[i].offsetFocus.x,	0.01f );
				
				ImGui::TreePop();
			}
		}

		if ( eraseIndex != optionCount )
		{
			options.erase( options.begin() + eraseIndex );
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

