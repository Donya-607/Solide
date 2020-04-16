#include "Enemy.h"

#include <vector>

#include "Donya/Loader.h"
#include "Donya/Useful.h"

#include "FilePath.h"
#include "Parameter.h"

namespace
{
	constexpr size_t KIND_COUNT = scast<size_t>( Enemy::Kind::KindCount );
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Enemy/";
	constexpr const char *MODEL_EXTENSION = ".bin";
	constexpr const char *MODEL_NAMES[KIND_COUNT]
	{
		"Run",
		"Archer",
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
namespace
{
	struct DrawingParam
	{
		// "[i]" of these vectors represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == Enemy::KIND_COUNT
		float				drawScale = 1.0f;
		Donya::Quaternion	drawRotation;
		Donya::Vector3		drawOffset;
		std::vector<float>	accelerations;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawScale		),
				CEREAL_NVP( drawRotation	),
				CEREAL_NVP( drawOffset		),
				CEREAL_NVP( accelerations	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct Member
	{
		DrawingParam drawer;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawer )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}

class ParamEnemy : public ParameterBase<ParamEnemy>
{
public:
	static constexpr const char *ID = "Enemy";
private:
	Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		constexpr bool fromBinary = false;
	#else
		constexpr bool fromBinary = true;
	#endif // DEBUG_MODE

		Load( m, fromBinary );

		ResizeMotionVector();
	}
	Member Data() const { return m; }
private:
	void ResizeMotionVector()
	{
		if ( m.drawer.accelerations.size() != KIND_COUNT )
		{
			m.drawer.accelerations.resize( KIND_COUNT );
			for ( auto &it : m.drawer.accelerations )
			{
				it = 1.0f;
			}
		}
	}
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"敵のパラメータ調整" ) )
		{
			if ( ImGui::TreeNode( u8"描画系" ) )
			{
				ImGui::DragFloat(		u8"描画スケール",		&m.drawer.drawScale,		0.01f );
				ImGui::DragFloat3(		u8"描画オフセット",	&m.drawer.drawOffset.x,		0.01f );
				ImGui::SliderFloat4(	u8"描画姿勢",		&m.drawer.drawRotation.x,	-1.0f, 1.0f );
				m.drawer.drawScale = std::max( 0.0f, m.drawer.drawScale );
				if ( ImGui::Button( u8"描画姿勢を正規化" ) ) { m.drawer.drawRotation.Normalize(); }

				if ( ImGui::TreeNode( u8"再生速度の倍率" ) )
				{
					ResizeMotionVector();

					std::string caption{};
					for ( size_t i = 0; i < KIND_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + MODEL_NAMES[i] + "]";
						ImGui::DragFloat( caption.c_str(), &m.drawer.accelerations[i], 0.001f );
					}

					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};
// Internal utility.
namespace
{
	static constexpr int MOTION_INDEX_BEGIN		= 0;
	static constexpr int MOTION_INDEX_PROCESS	= 1;
	static constexpr int MOTION_INDEX_END		= 2;

	Member FetchMember()
	{
		return ParamEnemy::Get().Data();
	}
}


namespace Enemy
{
	bool LoadResources()
	{
		ParamEnemy::Get().Init();

		bool succeeded = true;
		if ( !LoadModels() ) { succeeded = false; }

		return succeeded;
	}

#if USE_IMGUI
	std::string GetKindName( Kind kind )
	{
		if ( IsOutOfRange( kind ) ) { return "ERROR_OUT_OF_RANGE"; }
		// else
		return MODEL_NAMES[scast<int>( kind )];
	}
	void UseImGui()
	{
		ParamEnemy::Get().UseImGui();
	}

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


	void Base::Init( const InitializeParam &argInitializer )
	{
		initializer	= argInitializer;

		pos			= initializer.wsPos;
		orientation	= initializer.orientation;

		animator.ResetTimer();

		pModelParam	= GetModelPtr( GetKind() );
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

	}
	void Base::Draw( RenderingHelper *pRenderer )
	{
		if ( !pModelParam ) { return; }
		// else

		Donya::Model::Constants::PerModel::Common constant{};
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		constant.worldMatrix	= CalcWorldMatrix( /* useForHitBox = */ false, /* useForDrawing = */ true );
		pRenderer->UpdateConstant( constant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( pModelParam->model, pose );

		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP )
	{
		Donya::Model::Cube::Constant constant{};
		constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ true, /* useForDrawing = */ true );
		constant.matViewProj	= VP;
		constant.drawColor		= Donya::Vector4{ 1.0f, 1.0f, 1.0f, 0.5f };
		pRenderer->ProcessDrawingCube( constant );
	}
	void Base::UpdateMotion( float elapsedTime, int useMotionIndex )
	{
		const auto data			= FetchMember();
		const auto &speedSource = data.drawer.accelerations;

		const size_t intKind	= scast<size_t>( GetKind() );
		const float  playSpeed	= ( intKind < speedSource.size() ) ? speedSource[intKind] : 1.0f;
		animator.Update( elapsedTime * playSpeed );

		if ( pModelParam )
		{
			pose.AssignSkeletal( animator.CalcCurrentPose( pModelParam->motionHolder.GetMotion( useMotionIndex ) ) );
		}
	}
	Donya::Vector4x4 Base::CalcWorldMatrix( bool useForHitBox, bool useForDrawing ) const
	{
		Donya::Vector4x4 W{};
		
		if ( useForHitBox )
		{
			W._11 = 2.0f;
			W._22 = 2.0f;
			W._33 = 2.0f;
			W._41 = pos.x;
			W._42 = pos.y;
			W._43 = pos.z;
			return W;
		}
		// else

		if ( useForDrawing )
		{
			const auto data = FetchMember();
			W._11 = data.drawer.drawScale;
			W._22 = data.drawer.drawScale;
			W._33 = data.drawer.drawScale;
		}
		else
		{
			// Scales are 1.0f.
		}

		W *= orientation.MakeRotationMatrix();
		if ( useForDrawing )
		{
			const auto data = FetchMember();
			W *= data.drawer.drawRotation.MakeRotationMatrix();
		}

		W._41 = pos.x;
		W._42 = pos.y;
		W._43 = pos.z;

		if ( useForDrawing )
		{
			const auto data = FetchMember();
			W._41 += data.drawer.drawOffset.x;
			W._42 += data.drawer.drawOffset.y;
			W._43 += data.drawer.drawOffset.z;
		}

		return W;
	}
#if USE_IMGUI
	void AssignDerivedInstance( Kind kind, std::shared_ptr<Base> *pBasePtr )
	{
		if ( IsOutOfRange( kind ) ) { pBasePtr->reset(); return; }
		// else

		switch ( kind )
		{
		case Enemy::Kind::Straight:	*pBasePtr = std::make_shared<Straight>();	return;
		default: pBasePtr->reset(); return;
		}
	}
#endif // USE_IMGUI


	void Straight::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		velocity			= 0.0f;
		moveDistanceSum		= 0.0f;
		nowMoveToPositive	= true;
	}
	void Straight::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		AssignVelocity( targetPos );
		AssignOrientation( targetPos );

		if ( moveParam.alignToTarget )
		{
			moveDistanceSum		= 0.0f;
			nowMoveToPositive	= true;
		}
		else
		{
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

		constexpr int USE_MOTION_INDEX = 0;
		Base::UpdateMotion( elapsedTime, USE_MOTION_INDEX );
	}
	void Straight::PhysicUpdate()
	{
		pos += velocity;
	}
	bool Straight::ShouldRemove() const
	{
		return false;
	}
	Kind Straight::GetKind() const
	{
		return Kind::Straight;
	}
	Donya::Vector3 Straight::CalcNowMoveDirection( const Donya::Vector3 &targetPos ) const
	{
		const float moveSign = ( nowMoveToPositive || moveParam.alignToTarget ) ? 1.0f : -1.0f;
		const Donya::Vector3 moveDirection = moveParam.CalcMoveDirection( pos, targetPos );
		return moveDirection * moveSign;
	}
	void Straight::AssignVelocity( const Donya::Vector3 &targetPos )
	{
		const Donya::Vector3 moveDirection = CalcNowMoveDirection( targetPos );

		if ( moveParam.alignToTarget )
		{
			const float distance = moveDirection.Length();
			if ( distance <= moveParam.speed )
			{
				velocity = moveDirection;
			}
			else
			{
				velocity = moveDirection.Unit() * moveParam.speed;
			}
		}
		else
		{
			velocity = moveDirection.Unit() * moveParam.speed;
		}
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
					orientation,
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
					orientation,
					toTarget.Unit(),
					Donya::Quaternion::Freeze::Up
				);
			}
			return;
		default: return;
		}
	}
#if USE_IMGUI
	void Straight::ShowImGuiNode( const std::string &nodeCaption, bool *pWantRemove )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( pWantRemove )
		{
			const  std::string  buttonCaption = nodeCaption + u8"を削除";
			if ( ImGui::Button( buttonCaption.c_str() ) )
			{
				*pWantRemove = true;
			}
			else
			{
				*pWantRemove = false;
			}
		}

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			initializer.ShowImGuiNode( u8"初期化情報" );
			moveParam.ShowImGuiNode( u8"移動パラメータ関連" );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在のパラメータ" ) )
		{
			ImGui::DragFloat3( u8"現在のワールド座標",	&pos.x,				0.01f	);
			ImGui::DragFloat3( u8"現在の速度",			&velocity.x,		0.01f	);
			ImGui::DragFloat(  u8"現在の移動量合計",		&moveDistanceSum,	0.01f	);
			ImGui::Checkbox(   u8"正の方向へ動いているか",&nowMoveToPositive			);
			ImGui::SliderFloat4( u8"現在の姿勢",			&orientation.x, -1.0f, 1.0f );

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void Archer::MoverBase::Init( Archer &target )
	{
		target.timer = 0;
		target.animator.ResetTimer();

		if ( target.pModelParam )
		{
			const auto &initialMotion = target.pModelParam->motionHolder.GetMotion( AcquireMotionIndex() );
			target.animator.SetRepeatRange( initialMotion );
			target.pose.AssignSkeletal( target.animator.CalcCurrentPose( initialMotion ) );
		}
	}

	void Archer::Wait::Init( Archer &target )
	{
		MoverBase::Init( target );

		wasSearched = false;
	}
	void Archer::Wait::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( !wasSearched )
		{
			const float distance = Donya::Vector3{ targetPos - target.pos }.Length();
			if ( distance < target.searchRadius )
			{
				wasSearched = true;
			}
		}
	}
	int  Archer::Wait::AcquireMotionIndex() const
	{
		return MOTION_INDEX_BEGIN;
	}
	bool Archer::Wait::ShouldChangeState( Archer &target ) const
	{
		const bool waitWasFinished = ( target.waitFrame <= target.timer );
		return	( target.aimToTarget )
				? waitWasFinished && wasSearched
				: waitWasFinished;
	}
	std::function<void()> Archer::Wait::GetChangeStateMethod( Archer &target ) const
	{
		return [&]() { target.AssignMover<Aim>(); };
	}

	void Archer::Aim::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( target.aimToTarget )
		{
			Donya::Vector3 toTarget = targetPos - target.pos;
			toTarget.y = 0.0f; // Disallow x-axis rotation.

			target.orientation = Donya::Quaternion::LookAt
			(
				target.orientation,
				toTarget.Unit(),
				Donya::Quaternion::Freeze::Up
			);
		}
	}
	int  Archer::Aim::AcquireMotionIndex() const
	{
		return MOTION_INDEX_PROCESS;
	}
	bool Archer::Aim::ShouldChangeState( Archer &target ) const
	{
		return ( target.aimingFrame <= target.timer );
	}
	std::function<void()> Archer::Aim::GetChangeStateMethod( Archer &target ) const
	{
		return [&]() { target.AssignMover<Fire>(); };
	}

	void Archer::Fire::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		
	}
	int  Archer::Fire::AcquireMotionIndex() const
	{
		return MOTION_INDEX_END;
	}
	bool Archer::Fire::ShouldChangeState( Archer &target ) const
	{
		return target.animator.WasEnded();
	}
	std::function<void()> Archer::Fire::GetChangeStateMethod( Archer &target ) const
	{
		return [&]() { target.AssignMover<Wait>(); };
	}

	void Archer::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		timer = 0;
		AssignMover<Wait>();
	}
	void Archer::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		pMover->Update( *this, elapsedTime, targetPos );

		UpdateMotion( elapsedTime, pMover->AcquireMotionIndex() );
	}
	void Archer::PhysicUpdate()
	{
		// No op.
	}
	bool Archer::ShouldRemove() const
	{
		return false;
	}
	Kind Archer::GetKind() const
	{
		return Kind::Archer;
	}
	void Archer::GenerateShot()
	{
		auto desc = shotDesc;
		// The "direction", and "generatePos" are local space.
		desc.direction		=  orientation.RotateVector( desc.direction   );
		desc.generatePos	=  orientation.RotateVector( desc.generatePos );
		desc.generatePos	+= GetPosition();

		Bullet::BulletAdmin::Get().Append( desc );
	}
#if USE_IMGUI
	void Archer::ShowImGuiNode( const std::string &nodeCaption, bool *pWantRemove )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		if ( pWantRemove )
		{
			const  std::string  buttonCaption = nodeCaption + u8"を削除";
			if ( ImGui::Button( buttonCaption.c_str() ) )
			{
				*pWantRemove = true;
			}
			else
			{
				*pWantRemove = false;
			}
		}

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			initializer.ShowImGuiNode( u8"初期化情報" );
			shotDesc.ShowImGuiNode( u8"ショット情報" );
			ImGui::DragInt(		u8"狙いを定める時間（フレーム）",	&aimingFrame  );
			ImGui::Checkbox(	u8"自機を狙うか",					&aimToTarget  );
			ImGui::DragFloat(	u8"索敵範囲（半径）",				&searchRadius );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在のパラメータ" ) )
		{
			ImGui::DragFloat3(		u8"現在のワールド座標",	&pos.x, 0.01f );
			ImGui::SliderFloat4(	u8"現在の姿勢",			&orientation.x, -1.0f, 1.0f );
			ImGui::DragInt(			u8"内部タイマ",			&timer );

			ImGui::TreePop();
		}

		if ( ImGui::Button( u8"撃つ" ) )
		{
			GenerateShot();
		}

		ImGui::TreePop();
	}
#endif // USE_IMGUI
}
