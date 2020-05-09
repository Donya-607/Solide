#include "Warp.h"

#include <memory>

#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"
#include "SaveData.h"


namespace
{
	constexpr const char *MODEL_DIRECTORY	= "./Data/Models/Landmark/";
	constexpr const char *MODEL_NAME		= "Warp.bin";

	struct ModelData
	{
		Donya::Model::StaticModel	model;
		Donya::Model::Pose			pose;
	};
	static std::shared_ptr<ModelData> pModel{};

	bool LoadModel()
	{
		bool result		= true;
		bool succeeded	= true;

		Donya::Loader loader{};
		auto Load = [&loader]( const std::string &filePath, ModelData *pDest )->bool
		{
			loader.ClearData();

			bool result = loader.Load( filePath );
			if ( !result ) { return false; }
			// else

			const auto &source = loader.GetModelSource();
			pDest->model = Donya::Model::StaticModel::Create( source, loader.GetFileDirectory() );
			pDest->pose.AssignSkeletal( source.skeletal );

			return pDest->model.WasInitializeSucceeded();
		};

		const std::string filePath{ MODEL_DIRECTORY + std::string{ MODEL_NAME } };
		if ( !Donya::IsExistFile( filePath ) )
		{
			const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
			Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
			return false;
		}
		// else

		pModel = std::make_shared<ModelData>();
		result = Load( filePath, &( *pModel ) ); // std::shared_ptr<T> -> T -> T *
		if ( !result )
		{
			const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
			const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
			_ASSERT_EXPR( 0, errMsg.c_str() );

			succeeded = false;
		}

		return succeeded;
	}
	std::shared_ptr<ModelData> GetModelPtr()
	{
		return pModel;
	}
}


int					Warp::GetDestinationStageNo()	const { return destStageNo;	}
Donya::Vector3		Warp::GetPosition()				const { return wsPos;		}
Donya::AABB			Warp::GetHitBox()				const
{
	Donya::AABB tmp = hitBox;
	tmp.pos += wsPos;
	return tmp;
}
Donya::Vector4x4	Warp::CalcWorldMatrix( bool useForHitBox ) const
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
	W._41 = translation.x;
	W._42 = translation.y;
	W._43 = translation.z;
	return W;
}

void Warp::Init() {}
void Warp::Uninit() {}
void Warp::Update( float elapsedTime )
{
	unlocked = SaveDataAdmin::Get().IsUnlockedStageNumber( destStageNo );
}
void Warp::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	if ( !pRenderer ) { return; }
	// else

	const auto pModel = GetModelPtr();
	if ( !pModel ) { return; }
	// else

	Donya::Model::Constants::PerModel::Common constant{};
	constant.drawColor		= MakeDrawColor( color );
	constant.worldMatrix	= CalcWorldMatrix( /* useForHitBox = */ false );
	pRenderer->UpdateConstant( constant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( pModel->model, pModel->pose );

	pRenderer->DeactivateConstantModel();
}
void Warp::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() ) { return; }
	// else

	Donya::Model::Cube::Constant constant;
	constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ true );
	constant.matViewProj	= matVP;
	constant.drawColor		= MakeDrawColor( color );
	constant.lightDirection	= -Donya::Vector3::Up();
	pRenderer->ProcessDrawingCube( constant );
}

bool Warp::IsUnlocked() const
{
	return unlocked;
}
Donya::Vector4 Warp::MakeDrawColor( const Donya::Vector4 &color ) const
{
	constexpr float darken = 0.5f;
	return	( IsUnlocked() )
			? drawColor.Product( color )
			: drawColor.Product( color ) * darken;
}

#if USE_IMGUI
void Warp::ShowImGuiNode( const std::string &nodeCaption, bool *wantRemoveMe )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( wantRemoveMe )
	{
		const std::string caption = nodeCaption + u8"を削除";
		*wantRemoveMe = ImGui::Button( caption.c_str() );
	}

	ImGui::InputInt( u8"遷移先ステージ番号", &destStageNo );
	ImGui::Text( u8"0：セレクト画面" );
	ImGui::Text( u8"-1：タイトル画面" );
	ImGui::Text( "" );

	ImGui::DragFloat3( u8"ワールド座標", &wsPos.x, 0.01f );
	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );
	ImGui::DragFloat( u8"描画スケール",	&drawScale, 0.01f );
	ImGui::ColorEdit4( u8"描画色",		&drawColor.x );
	drawScale = std::max( 0.0f, drawScale );

	ImGui::TreePop();
}
#endif // USE_IMGUI



bool WarpContainer::LoadResource()
{
	return LoadModel();
}

void WarpContainer::Init( int stageNumber )
{
	stageNo = stageNumber;
#if DEBUG_MODE
	LoadJson( stageNumber );
#else
	LoadBin( stageNumber );
#endif // DEBUG_MODE

	for ( auto &it : warps )
	{
		it.Init();
	}
}
void WarpContainer::Uninit()
{
	for ( auto &it : warps )
	{
		it.Uninit();
	}
}
void WarpContainer::Update( float elapsedTime )
{
	for ( auto &it : warps )
	{
		it.Update( elapsedTime );
	}
}
void WarpContainer::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	if ( !pRenderer ) { return; }
	// else

	for ( auto &it : warps )
	{
		it.Draw( pRenderer, color );
	}
}
void WarpContainer::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else

	for ( auto &it : warps )
	{
		it.DrawHitBox( pRenderer, matVP, color );
	}
}

size_t		WarpContainer::GetWarpCount() const { return warps.size(); }
bool		WarpContainer::IsOutOfRange( size_t warpIndex ) const
{
	return ( GetWarpCount() <= warpIndex ) ? true : false;
}
const Warp *WarpContainer::GetWarpPtrOrNullptr( size_t warpIndex ) const
{
	if ( IsOutOfRange( warpIndex ) ) { return nullptr; }
	// else
	return &warps[warpIndex];
}

void WarpContainer::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void WarpContainer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void WarpContainer::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void WarpContainer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void WarpContainer::ShowImGuiNode( const std::string &nodeCaption, int stageNo )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::Button( u8"ワープを追加" ) )
	{
		warps.emplace_back( Warp{} );
		warps.back().Init();
	}
	if ( 1 < warps.size() && ImGui::Button( u8"末尾を削除" ) )
	{
		warps.pop_back();
	}

	if ( ImGui::TreeNode( u8"各々の設定" ) )
	{
		const size_t warpCount = GetWarpCount();
		size_t eraseIndex = warpCount;

		std::string caption{};
		for ( size_t i = 0; i < warpCount; ++i )
		{
			caption = u8"[" + std::to_string( i ) + u8"]番";

			bool wantRemove = false;
			warps[i].ShowImGuiNode( caption, &wantRemove );

			if ( wantRemove )
			{
				eraseIndex = i;
			}
		}

		if ( eraseIndex != warpCount )
		{
			warps.erase( warps.begin() + eraseIndex );
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

