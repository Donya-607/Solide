#include "Goal.h"

#include <memory>

#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/Useful.h"

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"

namespace
{
	constexpr const char *MODEL_DIRECTORY	= "./Data/Models/Landmark/";
	constexpr const char *MODEL_NAME		= "Goal.bin";

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

bool Goal::LoadResource()
{
	return LoadModel();
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
	if ( !useForHitBox ) { W *= orientation.MakeRotationMatrix(); }
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
	if ( !pRenderer ) { return; }
	// else

	const auto pModel = GetModelPtr();
	if ( !pModel ) { return; }
	// else

	Donya::Model::Constants::PerModel::Common constant{};
	constant.drawColor		= drawColor.Product( color );
	constant.worldMatrix	= CalcWorldMatrix( /* useForHitBox = */ false );
	pRenderer->UpdateConstant( constant );
	pRenderer->ActivateConstantModel();

	pRenderer->Render( pModel->model, pModel->pose );

	pRenderer->DeactivateConstantModel();
}
void Goal::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() ) { return; }
	// else

	Donya::Model::Cube::Constant constant;
	constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ true );
	constant.matViewProj	= matVP;
	constant.drawColor		= drawColor.Product( color );
	constant.lightDirection	= -Donya::Vector3::Up();
	pRenderer->ProcessDrawingCube( constant );
}

std::vector<int> Goal::GetUnlockStageNumbers() const
{
	return unlockStageNumbers;
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
	ImGui::DragFloat3( u8"ワールド座標", &wsPos.x,	0.01f );
	ImGui::DragFloat( u8"描画スケール",	&drawScale,	0.01f );
	ImGui::ColorEdit4( u8"描画色",		&drawColor.x );
	drawScale = std::max( 0.0f, drawScale );

	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );
	if ( ImGui::TreeNode( u8"解放するステージ番号" ) )
	{
		auto &data = unlockStageNumbers;

		if ( ImGui::TreeNode( u8"対象の追加" ) )
		{
			static int additionNo = 0;
			ImGui::InputInt( u8"加えるステージ番号", &additionNo );
			if ( ImGui::Button( u8"追加" ) )
			{
				data.emplace_back( additionNo );
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
				caption = "##" + std::to_string( i );
				ImGui::DragInt( caption.c_str(), &data[i], 1.0f, -1 );

				caption = u8"これを削除" + caption;
				ImGui::SameLine();
				if ( ImGui::Button( caption.c_str() ) )
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

