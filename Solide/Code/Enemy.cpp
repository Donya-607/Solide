#include "Enemy.h"

#include <vector>

#include "Donya/Loader.h"
#include "Donya/Useful.h"

namespace
{
	constexpr size_t KIND_COUNT = scast<size_t>( Enemy::Kind::KindCount );
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Enemy/";
	constexpr const char *MODEL_EXTENSION = "./Data/Models/Enemy/";
	constexpr const char *MODEL_NAMES[KIND_COUNT]
	{
		"Run",
	};

	static std::vector<std::shared_ptr<Enemy::ModelParam>> modelPtrs{};

	bool LoadModels()
	{
		// Already has loaded.
		if ( !modelPtrs.empty() ) { return true; }
		// else

		modelPtrs.resize( KIND_COUNT );

		Donya::Loader loader{};
		auto Load = [&loader]( const std::string &filePath, Enemy::ModelParam *pDest )->bool
		{
			loader.ClearData();

			bool  result  = loader.Load( filePath );
			if ( !result ) { return false; }
			// else

			const auto &source = loader.GetModelSource();
			pDest->model = Donya::Model::SkinningModel::Create( source, loader.GetFileDirectory() );
			pDest->motionHolder.AppendSource( source );

			return pDest->model.WasInitializeSucceeded();
		};

		bool result		= true;
		bool succeeded	= true;

		std::string filePath{};
		const std::string prefix = MODEL_DIRECTORY;
		for ( size_t i = 0; i < KIND_COUNT; ++i )
		{
			filePath = prefix + MODEL_NAMES[i] + MODEL_EXTENSION;
			if ( !Donya::IsExistFile( filePath ) )
			{
				const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
				Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
				continue;
			}
			// else

			auto &pModel = modelPtrs[i];
			pModel = std::make_shared<Enemy::ModelParam>();
			result = Load( filePath, &( *pModel ) ); // std::shared_ptr<T> -> T -> T *
			if ( !result )
			{
				const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
				const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
				_ASSERT_EXPR( 0, errMsg.c_str() );

				succeeded = false;
			}
		}

		if ( !succeeded )
		{
			modelPtrs.clear();
			return false;
		}
		// else
		return true;
	}

	bool IsOutOfRange( Enemy::Kind kind )
	{
		return ( scast<int>( kind ) < 0 || Enemy::Kind::KindCount <= kind ) ? true : false;
	}
	const std::shared_ptr<Enemy::ModelParam> GetModelPtr( Enemy::Kind kind )
	{
		if ( modelPtrs.empty() )
		{
			_ASSERT_EXPR( 0, L"Error : The models were not initialized.!" );
			return nullptr;
		}
		// else
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Model subscript out of range!" );
			return nullptr;
		}
		// else
		return modelPtrs[scast<int>( kind )];
	}
}


namespace Enemy
{
	bool LoadResources()
	{
		return LoadModels();
	}

#if USE_IMGUI
	void InitializeParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat3( u8"初期のワールド座標", &wsPos.x );

		Donya::Vector3 lookDir = orientation.LocalFront();
		ImGui::SliderFloat3( u8"初期の前方向", &lookDir.x, -1.0f, 1.0f );
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), lookDir.Unit(), Donya::Quaternion::Freeze::Up );

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	Donya::Vector3 MoveParam::CalcMoveDirection( const Donya::Vector3 moverPos, const Donya::Vector3 &targetPos ) const
	{
		if ( !alignToTarget ) { return direction; }
		// else

		const	Donya::Vector3 toTarget		= targetPos - moverPos;
		const	Donya::Vector3 projection	= direction * Donya::Dot( toTarget.Unit(), direction );
		return	projection;
	}
#if USE_IMGUI
	void MoveParam::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		ImGui::DragFloat( u8"移動速度（フレームあたり）",		&speed,			0.01f );
		ImGui::DragFloat( u8"移動範囲制限（ゼロで無制限）",	&rangeLimit,	0.01f );

		ImGui::SliderFloat3( u8"移動方向（単位ベクトル）",		&direction.x, -1.0f, 1.0f );
		direction.Normalize();
		ImGui::Checkbox( u8"自機と同列にあわせるか",			&alignToTarget );

		if ( ImGui::TreeNode( u8"正面方向の設定" ) )
		{
			if ( ImGui::RadioButton( u8"移動方向を向く",		lookDirection == LookDirection::MoveDirection ) )
			{ lookDirection = LookDirection::MoveDirection; }
			if ( ImGui::RadioButton( u8"自機の方向を向く",	lookDirection == LookDirection::Target ) )
			{ lookDirection = LookDirection::Target; }

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void Base::Draw( RenderingHelper *pRenderer )
	{
		if ( !pModelParam ) { return; }
		// else

		Donya::Model::Constants::PerModel::Common constant{};
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		constant.worldMatrix	= CalcWorldMatrix( /* useForHitBox = */ false );
		pRenderer->UpdateConstant( constant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( pModelParam->model, pose );

		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP )
	{
		Donya::Model::Cube::Constant constant{};
		constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ true );
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.5f };
		pRenderer->ProcessDrawingCube( constant );
	}
	Donya::Vector4x4 Base::CalcWorldMatrix( bool useForHitBox ) const
	{
		// Scales are 1.0f.
		Donya::Vector4x4 W{};

		if ( useForHitBox )
		{
			W._11 *= 2.0f;
			W._22 *= 2.0f;
			W._33 *= 2.0f;
			W._41 = pos.x;
			W._42 = pos.y;
			W._43 = pos.z;
			return W;
		}
		// else

		W = orientation.RequireRotationMatrix();
		W._41 = pos.x;
		W._42 = pos.y;
		W._43 = pos.z;
		return W;
	}


	void Straight::Init( const InitializeParam &argInitializer )
	{
		initializer	= argInitializer;

		pos			= initializer.wsPos;
		orientation	= initializer.orientation;

		animator.ResetTimer();

		pModelParam	= GetModelPtr( Kind::Straight );
		if ( pModelParam )
		{
			const auto &initialMotion = pModelParam->motionHolder.GetMotion( 0 );
			animator.SetRepeatRange( initialMotion );
			pose.AssignSkeletal
			(
				animator.CalcCurrentPose
				(
					initialMotion
				)
			);
		}

		velocity			= 0.0f;
		moveDistanceSum		= 0.0f;
		nowMoveToPositive	= true;
	}
	void Straight::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		AssignVelocity( targetPos );
		AssignOrientation( targetPos );

		moveDistanceSum += velocity.Length();
		if ( moveParam.rangeLimit <= moveDistanceSum )
		{
			// To inside of rangeLimit.
			const Donya::Vector3 movedDir = Donya::Vector3{ pos - initializer.wsPos }.Unit();
			const Donya::Vector3 correctMovedPos = initializer.wsPos + movedDir * moveParam.rangeLimit;
			velocity = correctMovedPos - pos;

			moveDistanceSum		= -moveParam.rangeLimit;
			nowMoveToPositive	= !nowMoveToPositive;
		}
	}
	void Straight::PhysicUpdate()
	{
		pos += velocity;
	}
	Donya::Vector3 Straight::CalcNowMoveDirection( const Donya::Vector3 &targetPos ) const
	{
		const float moveSign = ( nowMoveToPositive ) ? 1.0f : -1.0f;
		const Donya::Vector3 moveDirection = moveParam.CalcMoveDirection( pos, targetPos );
		return moveDirection * moveSign;
	}
	void Straight::AssignVelocity( const Donya::Vector3 &targetPos )
	{
		const Donya::Vector3 moveDirection = CalcNowMoveDirection( targetPos );
		velocity = moveDirection * moveParam.speed;
	}
	void Straight::AssignOrientation( const Donya::Vector3 &targetPos )
	{
		switch ( moveParam.lookDirection )
		{
		case MoveParam::LookDirection::MoveDirection:
			{
				const Donya::Vector3 moveDirection = CalcNowMoveDirection( targetPos );
				orientation = Donya::Quaternion::LookAt
				(
					Donya::Vector3::Front(),
					moveDirection.Unit(),
					Donya::Quaternion::Freeze::Up
				);
			}
			return;
		case MoveParam::LookDirection::Target:
			{
				const Donya::Vector3 toTarget = targetPos - pos;
				orientation = Donya::Quaternion::LookAt
				(
					Donya::Vector3::Front(),
					toTarget.Unit(),
					Donya::Quaternion::Freeze::Up
				);
			}
			return;
		default: return;
		}
	}
}
