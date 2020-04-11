#include "Goal.h"

#include "Donya/Useful.h"

#include "FilePath.h"
#include "Parameter.h"

namespace
{
	constexpr const char *MODEL_DIRECTORY	= "./Data/Models/Goal/";
	constexpr const char *MODEL_NAME		= "Goal";
	constexpr const char *EXTENSION			= ".bin";

	struct ModelData
	{

	};
}

bool Goal::LoadResource()
{

}


Donya::Vector3		Goal::GetPosition()	const { return wsPos; }
Donya::AABB			Goal::GetHitBox()	const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += wsPos;
	return tmp;
}
Donya::Vector4x4	Goal::CalcWorldMatrix( bool useForHitBox ) const
{
	const Donya::Vector3 scale = ( useForHitBox )
		? hitBox.size * 2.0f
		: Donya::Vector3{ drawScale, drawScale, drawScale };
	const Donya::Vector3 translation = ( useForHitBox )
		? GetHitBox().pos
		: wsPos;

	Donya::Vector4x4 W{};
	W._11 = scale.x;
	W._22 = scale.y;
	W._33 = scale.z;
	W *= orientation.RequireRotationMatrix();
	W._41 = translation.x;
	W._42 = translation.y;
	W._43 = translation.z;
	return W;
}

void Goal::Init( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE

	orientation = Donya::Quaternion::Identity();
}
void Goal::Uninit() {}

void Goal::Update( float elapsedTime )
{
	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( rotateAngle ) );
	orientation.RotateBy( rotation );
}

void Goal::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{

}
void Goal::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{

}

void Goal::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void Goal::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void Goal::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void Goal::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void Goal::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat( u8"回転角（Degree，フレーム辺り）", &rotateAngle );
	ImGui::DragFloat3( u8"ワールド座標", &wsPos.x );
	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );
	
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

