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


int					Warp::GetDestinationStageNo()	const { return destStageNo;				}
Donya::Vector3		Warp::GetPosition()				const { return wsPos;					}
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
std::vector<Timer>	Warp::GetBorderTimes()			const { return borderTimes;				}
Donya::Vector3		Warp::GetReturningPosition()	const { return GetPosition() + returningPosOffset; }
Donya::Quaternion	Warp::GetReturningOrientation()	const { return returningOrientation;	}

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
	constexpr float multiply = 0.4f;
	constexpr Donya::Vector4 darken{ multiply, multiply, multiply, 1.0f };
	return	( IsUnlocked() )
			? drawColor.Product( color )
			: drawColor.Product( color ).Product( darken );
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
	ImGui::ColorEdit4( u8"描画色",		&drawColor.x );
	drawScale = std::max( 0.0f, drawScale );

	if ( ImGui::TreeNode( u8"ランクのボーダーラインとなる時間の設定" ) )
	{
		ParameterHelper::ResizeByButton( &borderTimes );
		ImGui::Text( "" );

		auto GetRankStr = []( size_t index )->std::string
		{
			const std::string ranks = "SABCDE";
			// The size() method returns character count("SABCDE" size is 6), does not contain the null-termination.
			if ( ranks.size() <= index ) { index = ranks.size() - 1; }
			return std::string{ ranks[index] } + u8"ランク";
		};

		std::string caption{};
		std::string rankStr{};

		const size_t count = borderTimes.size();
		size_t eraseIndex = count;
		for ( size_t i = 0; i < count; ++i )
		{
			rankStr = GetRankStr( i );
			caption = rankStr + u8"を削除";
			if ( ImGui::Button( caption.c_str() ) )
			{
				eraseIndex = i;
			}
			ImGui::SameLine();

			caption = rankStr + u8"となるボーダー";
			borderTimes[i].ShowImGuiNode( caption, /* useTreeNode = */ true );
		}

		if ( eraseIndex != count )
		{
			borderTimes.erase( borderTimes.begin() + eraseIndex );
		}

		ImGui::TreePop();
	}

	if ( ImGui::TreeNode( u8"戻ってきたときの設定" ) )
	{
		ImGui::DragFloat3( u8"戻ってくる相対位置（自身からの相対）", &returningPosOffset.x, 0.01f );

		Donya::Vector3 front = returningOrientation.LocalFront();
		ImGui::SliderFloat3( u8"戻ってきたときの前方向", &front.x, -1.0f, 1.0f );
		returningOrientation = Donya::Quaternion::LookAt
		(
			Donya::Vector3::Front(), front.Unit(),
			Donya::Quaternion::Freeze::Up
		);

		ImGui::TreePop();
	}

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

