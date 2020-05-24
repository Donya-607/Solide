#include "Enemy.h"

#include <vector>

#include <cereal/types/vector.hpp>

#include "Donya/Color.h"
#include "Donya/Loader.h"
#include "Donya/Useful.h"

#include "Effect.h"
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
		"GateKeeper",
		"Chaser",
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
	// The vector of parameters contain some value per kind of enemy.
	// "[i]" of these vectors represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == Enemy::KIND_COUNT

	struct DrawingParam
	{
		float				drawScale = 1.0f;
		Donya::Quaternion	drawRotation;
		Donya::Vector3		drawOffset;
		std::vector<float>	accelerations;

		Donya::Vector4 oilColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		RenderingHelper::AdjustColorConstant oilAdjustment;
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
				archive
				(
					CEREAL_NVP( oilColor		),
					CEREAL_NVP( oilAdjustment	)
				);
			}
			if ( 2 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct CollisionParam
	{
		struct PerKind
		{
			Donya::AABB hitBox;
			Donya::AABB hurtBox;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( hitBox  ),
					CEREAL_NVP( hurtBox )
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		std::vector<PerKind> collisions;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( collisions )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct Member
	{
		DrawingParam	drawer;
		CollisionParam	collider;

		int				removeOilFrame = 1;
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
				archive( CEREAL_NVP( collider ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( removeOilFrame ) );
			}
			if ( 3 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( DrawingParam,				1 )
CEREAL_CLASS_VERSION( CollisionParam,			0 )
CEREAL_CLASS_VERSION( CollisionParam::PerKind,	0 )
CEREAL_CLASS_VERSION( Member,					2 )

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

		ResizeKindVectors();
	}
	Member Data() const { return m; }
private:
	void ResizeKindVectors()
	{
		// Returns true if was resized.
		auto ResizeIfNeeded = []( auto *pVector )->bool
		{
			if ( pVector->size() != KIND_COUNT )
			{
				pVector->resize( KIND_COUNT );
				return true;
			}
			// else
			return false;
		};

		if ( ResizeIfNeeded( &m.drawer.accelerations ) )
		{
			for ( auto &it :  m.drawer.accelerations )
			{
				it = 1.0f;
			}
		}
		
		if ( ResizeIfNeeded( &m.collider.collisions ) )
		{
			for ( auto &it : m.collider.collisions )
			{
				it.hitBox.size		= 0.5f;
				it.hitBox.exist		= true;
				it.hurtBox.size		= 0.5f;
				it.hurtBox.exist	= true;
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
			ResizeKindVectors();

			if ( ImGui::TreeNode( u8"描画系" ) )
			{
				ImGui::DragFloat(		u8"描画スケール",		&m.drawer.drawScale,		0.01f );
				ImGui::DragFloat3(		u8"描画オフセット",	&m.drawer.drawOffset.x,		0.01f );
				ImGui::SliderFloat4(	u8"描画姿勢",		&m.drawer.drawRotation.x,	-1.0f, 1.0f );
				m.drawer.drawScale = std::max( 0.0f, m.drawer.drawScale );
				if ( ImGui::Button( u8"描画姿勢を正規化" ) ) { m.drawer.drawRotation.Normalize(); }

				if ( ImGui::TreeNode( u8"再生速度の倍率" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < KIND_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + MODEL_NAMES[i] + "]";
						ImGui::DragFloat( caption.c_str(), &m.drawer.accelerations[i], 0.001f );
					}

					ImGui::TreePop();
				}

				if ( ImGui::TreeNode( u8"状態毎の描画色" ) )
				{
					ImGui::ColorEdit4( u8"オイル時・描画色", &m.drawer.oilColor.x );
					ParameterHelper::ShowConstantNode( u8"オイル時・加算マテリアル色", &m.drawer.oilAdjustment );
					
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"当たり判定系" ) )
			{
				std::string caption{};
				for ( size_t i = 0; i < KIND_COUNT; ++i )
				{
					caption = "[" + std::to_string( i ) + ":" + MODEL_NAMES[i] + "]";
					
					ParameterHelper::ShowAABBNode( caption + u8"当たり判定", &m.collider.collisions[i].hitBox  );
					ParameterHelper::ShowAABBNode( caption + u8"喰らい判定", &m.collider.collisions[i].hurtBox );
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"その他" ) )
			{
				ImGui::DragInt( u8"付いた油がはがれる時間（フレーム）", &m.removeOilFrame );
				m.removeOilFrame = std::max( 0, m.removeOilFrame );

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

	static constexpr int RECURSION_RAY_COUNT	= 4;

	Member FetchMember()
	{
		return ParamEnemy::Get().Data();
	}

	bool IsTargetClose( float searchRadius, const Donya::Vector3 &basePos, const Donya::Vector3 &targetPos )
	{
		const float distance = Donya::Vector3{ targetPos - basePos }.Length();
		return ( distance < searchRadius ) ? true : false;
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

		ImGui::DragFloat ( u8"索敵範囲（半径）",		&searchRadius,	0.01f );
		ImGui::DragFloat3( u8"初期のワールド座標",	&wsPos.x,		0.01f );

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


	Base::~Base()
	{
		if ( pEffect )
		{
			pEffect->Stop();
			pEffect.reset();
		}
	}
	void Base::Init( const InitializeParam &argInitializer )
	{
		initializer	= argInitializer;

		pos			= initializer.wsPos;
		orientation	= initializer.orientation;
		element		= Element::Type::Nil;

		animator.ResetTimer();

		pModelParam	= GetModelPtr( GetKind() );
		if ( pModelParam && pModelParam->motionHolder.GetMotionCount() )
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
	void Base::Uninit()
	{
		if ( pEffect )
		{
			pEffect->Stop();
			pEffect.reset();
		}
	}
	void Base::Draw( RenderingHelper *pRenderer )
	{
		if ( !pModelParam ) { return; }
		// else

		const auto drawData = FetchMember().drawer;
		Donya::Model::Constants::PerModel::Common modelConstant{};
		modelConstant.drawColor		= CalcDrawColor();
		modelConstant.worldMatrix	= CalcWorldMatrix( /* useForHitBox = */ false, /* useForHurtBox = */ false, /* useForDrawing = */ true );
		RenderingHelper::AdjustColorConstant colorConstant =
			( element.Has( Element::Type::Oil ) )
			? drawData.oilAdjustment
			: RenderingHelper::AdjustColorConstant::MakeDefault();
		pRenderer->UpdateConstant( modelConstant );
		pRenderer->UpdateConstant( colorConstant );
		pRenderer->ActivateConstantModel();
		pRenderer->ActivateConstantAdjustColor();

		pRenderer->Render( pModelParam->model, pose );

		pRenderer->DeactivateConstantAdjustColor();
		pRenderer->DeactivateConstantModel();
	}
	void Base::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP )
	{
		Donya::Model::Cube::Constant constant{};
		constant.matViewProj	= VP;

		constexpr float	boxAlpha		= 0.5f;
		constexpr auto	hitBoxColor		= Donya::Color::Code::RED;
		constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ true, /* useForHurtBox = */ false, /* useForDrawing = */ true );
		constant.drawColor		= Donya::Vector4{ Donya::Color::MakeColor( hitBoxColor ), boxAlpha };
		pRenderer->ProcessDrawingCube( constant );

		constexpr auto	hurtBoxColor	= Donya::Color::Code::LIME;
		constant.matWorld		= CalcWorldMatrix( /* useForHitBox = */ false, /* useForHurtBox = */ true, /* useForDrawing = */ true );
		constant.drawColor		= Donya::Vector4{ Donya::Color::MakeColor( hurtBoxColor ), boxAlpha };
		pRenderer->ProcessDrawingCube( constant );
	}
	void Base::MakeDamage( const Element &effect ) const
	{
		element.Add( effect.Get() );
	}
	void Base::AcquireHitBoxes( std::vector<Donya::AABB> *pAppendDest ) const
	{
		if ( !pAppendDest ) { return; }
		// else

		pAppendDest->emplace_back( std::move( AcquireHitBox( /* wantWorldSpace = */ true ) ) );
	}
	void Base::AcquireHurtBoxes( std::vector<Donya::AABB> *pAppendDest ) const
	{
		if ( !pAppendDest ) { return; }
		// else

		pAppendDest->emplace_back( std::move( AcquireHurtBox( /* wantWorldSpace = */ true ) ) );
	}
	Donya::AABB Base::AcquireHitBox( bool wantWorldSpace ) const
	{
		const auto	data		= FetchMember();
		const auto	&collisions	= data.collider.collisions;
		const int	intKind		= scast<int>( GetKind() );

		if ( intKind < 0 || scast<int>( collisions.size() ) <= intKind ) { return Donya::AABB::Nil(); }
		// else

		Donya::AABB wsHitBox = collisions[intKind].hitBox;
		if ( wantWorldSpace )
		{
			wsHitBox.pos += GetPosition();
		}
		return wsHitBox;
	}
	Donya::AABB Base::AcquireHurtBox( bool wantWorldSpace ) const
	{
		const auto	data		= FetchMember();
		const auto	&collisions	= data.collider.collisions;
		const int	intKind		= scast<int>( GetKind() );

		if ( intKind < 0 || scast<int>( collisions.size() ) <= intKind ) { return Donya::AABB::Nil(); }
		// else

		Donya::AABB wsHurtBox = collisions[intKind].hurtBox;
		if ( wantWorldSpace )
		{
			wsHurtBox.pos += GetPosition();
		}

		return wsHurtBox;
	}
	void Base::OiledUpdate()
	{
		if ( !element.Has( Element::Type::Oil ) ) { oiledTimer = 0; return; }
		// else

		oiledTimer++;
		if ( FetchMember().removeOilFrame <= oiledTimer )
		{
			element.Subtract( Element::Type::Oil );
			oiledTimer = 0;
		}
	}
	void Base::BurningUpdate()
	{
		if ( !pEffect && element.Has( Element::Type::Flame ) )
		{
			pEffect = std::make_shared<EffectHandle>
				(
					EffectHandle::Generate( EffectAttribute::Flame, pos )
					);
		}

		if ( !pEffect ) { return; }
		// else

		pEffect->SetPosition( pos );
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
	Donya::Vector4			Base::CalcDrawColor() const
	{
		const auto data = FetchMember();
		Donya::Vector4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		if ( element.Has( Element::Type::Oil	) ) { baseColor.Product( data.drawer.oilColor	); }
		
		return baseColor;
	}
	Donya::Vector4x4		Base::CalcWorldMatrix( bool useForHitBox, bool useForHurtBox, bool useForDrawing ) const
	{
		Donya::Vector4x4 W{};
		
		if ( useForHitBox || useForHurtBox )
		{
			W._11 = 2.0f;
			W._22 = 2.0f;
			W._33 = 2.0f;
			W._41 = pos.x;
			W._42 = pos.y;
			W._43 = pos.z;

			const auto	data		= FetchMember();
			const auto	&collisions	= data.collider.collisions;
			const int	intKind		= scast<int>( GetKind() );

			if ( intKind < 0 || scast<int>( collisions.size() ) <= intKind ) { return W; }
			// else

			const auto	&perKind	= data.collider.collisions[intKind];
			const auto	&applyBox	= ( useForHitBox ) ? perKind.hitBox : perKind.hurtBox;

			W._11 *= applyBox.size.x;
			W._22 *= applyBox.size.y;
			W._33 *= applyBox.size.z;
			W._41 += applyBox.pos.x;
			W._42 += applyBox.pos.y;
			W._43 += applyBox.pos.z;

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
	Base::AABBResult		Base::CalcCorrectedVector( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
	{
		AABBResult defaultResult{};
		defaultResult.correctedVector = vector;
		defaultResult.wasHit = false;

		return	( solids.empty() )
				? defaultResult
				: CalcCorrectedVectorImpl( vector, solids );
	}
	Base::RecursionResult	Base::CalcCorrectedVector( int recursionLimit, const Donya::Vector3 &vector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix ) const
	{
		RecursionResult initial{};
		initial.correctedVector			= vector;
		initial.raycastResult.wasHit	= false;

		return	( !pTerrain || !pTerrainMatrix )
				? initial
				: CalcCorrectedVectorImpl( recursionLimit, 0, initial, *pTerrain, *pTerrainMatrix );
	}
	Base::AABBResult		Base::CalcCorrectedVectorImpl( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
	{
		AABBResult result{};
		result.correctedVector = vector;
		result.wasHit = false;

		Donya::Vector3 moveSign // The moving direction of myself. Take a value of +1.0f or -1.0f.
		{
			scast<float>( Donya::SignBit( vector.x ) ),
			scast<float>( Donya::SignBit( vector.y ) ),
			scast<float>( Donya::SignBit( vector.z ) )
		};
		if ( moveSign.IsZero() ) { return result; }
		// else

		// HACK : Should I do not use hit-box? Currently, the collision processes does not use hit-box, using the point only.
		Donya::AABB movedBody = AcquireHitBox( /* wantWorldSpace = */ true );
		movedBody.pos += vector;

		Donya::AABB other{};

		auto FindCollidingAABB	= []( const Donya::AABB &myself, const std::vector<Donya::AABB> &solids, bool exceptMyself = true )
		{
			for ( const auto &it : solids )
			{
				if ( exceptMyself && it == myself ) { continue; }
				// else

				if ( Donya::AABB::IsHitAABB( myself, it ) )
				{
					return it;
				}
			}

			return Donya::AABB::Nil();
		};
	
		auto CalcPenetration	= []( const Donya::AABB &myself, const Donya::Vector3 &myMoveSign, const Donya::AABB &other )
		{
			Donya::Vector3 plusPenetration
			{
				fabsf( ( myself.pos.x + myself.size.x ) - ( other.pos.x - other.size.x ) ),
				fabsf( ( myself.pos.y + myself.size.y ) - ( other.pos.y - other.size.y ) ),
				fabsf( ( myself.pos.z + myself.size.z ) - ( other.pos.z - other.size.z ) )
			};
			Donya::Vector3 minusPenetration
			{
				fabsf( ( myself.pos.x - myself.size.x ) - ( other.pos.x + other.size.x ) ),
				fabsf( ( myself.pos.y - myself.size.y ) - ( other.pos.y + other.size.y ) ),
				fabsf( ( myself.pos.z - myself.size.z ) - ( other.pos.z + other.size.z ) )
			};
			Donya::Vector3 penetration{}; // Store absolute value.
			penetration.x
				= ( myMoveSign.x < 0.0f ) ? minusPenetration.x
				: ( myMoveSign.x > 0.0f ) ? plusPenetration.x
				: 0.0f;
			penetration.y
				= ( myMoveSign.y < 0.0f ) ? minusPenetration.y
				: ( myMoveSign.y > 0.0f ) ? plusPenetration.y
				: 0.0f;
			penetration.z
				= ( myMoveSign.z < 0.0f ) ? minusPenetration.z
				: ( myMoveSign.z > 0.0f ) ? plusPenetration.z
				: 0.0f;
			return penetration;
		};
		auto CalcResolver		= []( const Donya::Vector3 &penetration, const Donya::Vector3 &myMoveSign )
		{
			// Prevent the two edges onto same place(the collision detective allows same(equal) value).
			constexpr float ERROR_MARGIN = 0.0001f;

			Donya::Vector3 resolver
			{
				( penetration.x + ERROR_MARGIN ) * -myMoveSign.x,
				( penetration.y + ERROR_MARGIN ) * -myMoveSign.y,
				( penetration.z + ERROR_MARGIN ) * -myMoveSign.z
			};
			return resolver;
		};

		constexpr unsigned int MAX_LOOP_COUNT = 1000U;
		unsigned int loopCount{};
		while ( ++loopCount < MAX_LOOP_COUNT )
		{
			other = FindCollidingAABB( movedBody, solids );
			if ( other == Donya::AABB::Nil() ) { break; } // Does not detected a collision.
			// else

			// Store absolute value.
			const Donya::Vector3 penetration	= CalcPenetration( movedBody, moveSign, other );
			const Donya::Vector3 resolver		= CalcResolver( penetration, moveSign );

			// Repulse to the more little(but greater than zero) axis side of penetration.

			enum AXIS { X = 0, Y, Z };
			auto CalcLowestAxis	= []( const Donya::Vector3 &v )->AXIS
			{
				// Fail safe.
				if ( v.IsZero() ) { return X; }
				// else

				auto Increment = []( AXIS axis )
				{
					return scast<AXIS>( scast<int>( axis ) + 1 );
				};

				auto IsLowerThanOther  = [&Increment]( Donya::Vector3 v, AXIS targetAxis )
				{
					for ( AXIS i = X; i <= Z; i = Increment( i ) )
					{
						// Except the same axis.
						if ( i == targetAxis ) { continue; }
						if ( ZeroEqual( v[i] ) ) { continue; }
						// else

						if ( v[i] < v[targetAxis] )
						{
							return false;
						}
					}

					return true;
				};
				auto AssignInitialAxis = [&Increment]( Donya::Vector3 v )->AXIS
				{
					for ( AXIS i = X; i <= Z; i = Increment( i ) )
					{
						if ( ZeroEqual( v[i] ) ) { continue; }
						// else
						return i;
					}

					// Fail safe.
					return Y;
				};

				AXIS lowestAxis = AssignInitialAxis( v );
				for ( AXIS i = X; i <= Z; i = Increment( i ) )
				{
					if ( ZeroEqual( v[i] ) ) { continue; }
					// else

					if ( IsLowerThanOther( v, i ) )
					{
						lowestAxis = i;
					}
				}

				return lowestAxis;
			};
			const AXIS min		= CalcLowestAxis( penetration );

			movedBody.pos[min] += resolver[min];
			moveSign[min]		= scast<float>( Donya::SignBit( resolver[min] ) );
			result.wasHit		= true;

			if ( moveSign.IsZero()  ) { break; }
			// else

			/*
			// Repulse to the more little(but greater than zero) axis side of penetration.
			if ( ( penetration.y < penetration.x && !ZeroEqual( penetration.y ) ) || ZeroEqual( penetration.x ) )
			{
				movedBody.pos.y += resolver.y;
				moveVelocity.y  =  0.0f;
				moveSign.y = scast<float>( Donya::SignBit( resolver.y ) );
			}
			else if ( !ZeroEqual( penetration.x ) )
			{
				movedBody.pos.x += resolver.x;
				moveVelocity.x  =  0.0f;
				moveSign.x = scast<float>( Donya::SignBit( resolver.x ) );
			}
			*/
		}

		const Donya::Vector3 &destination = movedBody.pos - AcquireHitBox( /* wantWorldSpace = */ false ).pos/* Except the offset of hitBox */;
		
		result.correctedVector = destination - pos;
		return result;
	}
	Base::RecursionResult	Base::CalcCorrectedVectorImpl( int recursionLimit, int recursionCount, RecursionResult inheritedResult, const Donya::Model::PolygonGroup &terrain, const Donya::Vector4x4 &terrainMatrix ) const
	{
		constexpr float ERROR_ADJUST = 0.001f;

		// If we can't resolve with very small movement, we give-up the moving.
		if ( recursionLimit <= recursionCount || inheritedResult.correctedVector.Length() < ERROR_ADJUST )
		{
			inheritedResult.correctedVector = Donya::Vector3::Zero();
			return inheritedResult;
		}
		// else

		const Donya::Vector3 wsRayStart		=  pos;
		const Donya::Vector3 wsRayEnd		=  wsRayStart + inheritedResult.correctedVector;

		const Donya::Model::RaycastResult currentResult = terrain.RaycastWorldSpace( terrainMatrix, wsRayStart, wsRayEnd );

		// The moving vector(ray) didn't collide to anything, so we can move directly.
		if ( !currentResult.wasHit ) { return inheritedResult; }
		// else

		const Donya::Vector3 internalVec	=  wsRayEnd - currentResult.intersection;
		const Donya::Vector3 wsFaceNormal	=  currentResult.nearestPolygon.normal;
		const Donya::Vector3 projVelocity	= -wsFaceNormal * Dot( internalVec, -wsFaceNormal );

		constexpr float ERROR_MAGNI = 1.0f + ERROR_ADJUST;
		inheritedResult.correctedVector		-= projVelocity * ERROR_MAGNI;
		inheritedResult.raycastResult		=  currentResult;

		// Recurse by corrected velocity.
		// This recursion will stop when the corrected velocity was not collided.
		return CalcCorrectedVectorImpl( recursionLimit, recursionCount + 1, inheritedResult, terrain, terrainMatrix );
	}
#if USE_IMGUI
	void AssignDerivedInstance( Kind kind, std::shared_ptr<Base> *pBasePtr )
	{
		if ( IsOutOfRange( kind ) ) { pBasePtr->reset(); return; }
		// else

		switch ( kind )
		{
		case Enemy::Kind::Straight:		*pBasePtr = std::make_shared<Straight>();	return;
		case Enemy::Kind::Archer:		*pBasePtr = std::make_shared<Archer>();		return;
		case Enemy::Kind::GateKeeper:	*pBasePtr = std::make_shared<GateKeeper>();	return;
		case Enemy::Kind::Chaser:		*pBasePtr = std::make_shared<Chaser>();		return;
		default: pBasePtr->reset(); return;
		}
	}
#endif // USE_IMGUI


	bool WillSlip( const Element &element, const Donya::Vector3 &velocity )
	{
		return element.Has( Element::Type::Oil ) && !velocity.IsZero();
	}
	bool WillBurn( const Element &element )
	{
		return element.Has( Element::Type::Flame ) && element.Has( Element::Type::Oil );
	}


#pragma region Straight

	void Straight::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		velocity			= 0.0f;
		moveDistanceSum		= 0.0f;
		nowMoveToPositive	= true;
	}
	void Straight::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		captured = ( moveParam.alignToTarget )
		? IsTargetClose( initializer.searchRadius, pos, targetPos )
		: false;

		AssignVelocity( targetPos );
		AssignOrientation( targetPos );

		if ( moveParam.alignToTarget )
		{
			moveDistanceSum		= 0.0f;
			nowMoveToPositive	= true;
		}
		else
		{
			// TODO : Enemy::Straight
			// 当たり判定にて補正された際に，総移動距離が正しくなくなる。
			// 正しくするには，移動距離の取得を補正後の移動ベクトルで行い，
			// 移動制限を超えた際の処理（if文内）を，補正無しで想定される位置に代入するのではなく，
			// 移動ベクトルの長さを「総移動距離 == 移動制限」となるような値に縮小する必要がある。

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
		UpdateMotion( elapsedTime, USE_MOTION_INDEX );
		OiledUpdate();
		BurningUpdate();
	}
	void Straight::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
		const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );

		velocity = aabbResult.correctedVector;

		pos += velocity;
	}
	bool Straight::ShouldRemove() const
	{
	#if USE_IMGUI
		if ( wantRemoveByGui ) { return true; }
	#endif // USE_IMGUI
		return WillSlip( element, velocity ) || WillBurn( element );
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
			if ( !captured ) { velocity = 0.0f; return; }
			// else

			const float moveAmount = moveDirection.Length();
			if ( moveAmount <= moveParam.speed )
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
				if ( !captured ) { return; }
				// else

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
	void Straight::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		const std::string buttonCaption = ( useTreeNode )
						? nodeCaption + u8"を削除"
						: u8"削除";
		if ( ImGui::Button( buttonCaption.c_str() ) )
		{
			wantRemoveByGui = true;
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

			Donya::Vector3 localFront = orientation.LocalFront();
			ImGui::SliderFloat3( u8"現在の姿勢", &localFront.x, -1.0f, 1.0f );
			orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

			element.ShowImGuiNode( /* useTreeNode = */ false, "" );

			ImGui::TreePop();
		}

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI

// region Straight
#pragma endregion


#pragma region Archer

	void Archer::MoverBase::Init( Archer &target )
	{
		target.timer = 0;
		target.animator.ResetTimer();
		target.animator.DisableLoop();

		if ( target.pModelParam )
		{
			const auto &initialMotion = target.pModelParam->motionHolder.GetMotion( AcquireMotionIndex() );
			target.animator.SetRepeatRange( initialMotion );
			target.pose.AssignSkeletal( target.animator.CalcCurrentPose( initialMotion ) );
		}
	}
	void Archer::MoverBase::LookToTarget( Archer &target, const Donya::Vector3 &targetPos )
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

	void Archer::Wait::Init( Archer &target )
	{
		MoverBase::Init( target );

		wasSearched = false;

		target.element.Subtract( Element::Type::Flame );
	}
	void Archer::Wait::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( !wasSearched )
		{
			if ( IsTargetClose( target.initializer.searchRadius, target.pos, targetPos ) )
			{
				wasSearched = true;
			}
		}

		if ( target.aimToTarget && wasSearched )
		{
			if ( target.waitFrame <= target.timer )
			{
				LookToTarget( target, targetPos );

				target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
			}
		}
	}
	int  Archer::Wait::AcquireMotionIndex() const
	{
		return MOTION_INDEX_BEGIN;
	}
	bool Archer::Wait::ShouldChangeState( Archer &target ) const
	{
		return	( target.aimToTarget )
				? target.animator.WasEnded()
				: target.waitFrame <= target.timer;
	}
	std::function<void()> Archer::Wait::GetChangeStateMethod( Archer &target ) const
	{
		if ( target.aimToTarget && !wasSearched )
		{
			return [&]() { target.AssignMover<Wait>(); };
		}
		// else
		return [&]() { target.AssignMover<Aim>(); };
	}
	#if USE_IMGUI
	std::string Archer::Wait::GetStateName() const { return u8"待機"; }
	#endif // USE_IMGUI

	void Archer::Aim::Init( Archer &target )
	{
		MoverBase::Init( target );

		target.element.Add( Element::Type::Flame );
	}
	void Archer::Aim::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( target.aimToTarget )
		{
			MoverBase::LookToTarget( target, targetPos );
		}

		target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
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
#if USE_IMGUI
	std::string Archer::Aim::GetStateName() const { return u8"狙い"; }
#endif // USE_IMGUI

	void Archer::Fire::Update( Archer &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( target.intervalFrame <= target.timer )
		{
			if ( target.timer == target.intervalFrame )
			{
				target.GenerateShot();
				target.element.Subtract( Element::Type::Flame );
			}

			target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
		}
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
#if USE_IMGUI
	std::string Archer::Fire::GetStateName() const { return u8"ショット"; }
#endif // USE_IMGUI

	void Archer::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		timer = 0;
		AssignMover<Wait>();
	}
	void Archer::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		pMover->Update( *this, elapsedTime, targetPos );

		if ( pMover->ShouldChangeState( *this ) )
		{
			auto ChangeMover = pMover->GetChangeStateMethod( *this );
			ChangeMover();
		}

		OiledUpdate();
		BurningUpdate();
	}
	void Archer::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		// No op.
	}
	bool Archer::ShouldRemove() const
	{
	#if USE_IMGUI
		if ( wantRemoveByGui ) { return true; }
	#endif // USE_IMGUI
		return WillBurn( element );
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
		desc.addElement		=  Element::Type::Flame;

		Bullet::BulletAdmin::Get().Append( desc );
	}
#if USE_IMGUI
	void Archer::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		const std::string buttonCaption = ( useTreeNode )
						? nodeCaption + u8"を削除"
						: u8"削除";
		if ( ImGui::Button( buttonCaption.c_str() ) )
		{
			wantRemoveByGui = true;
		}

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			initializer.ShowImGuiNode( u8"初期化情報" );
			shotDesc.ShowImGuiNode( u8"ショット情報" );
			ImGui::DragInt(		u8"待機時間（フレーム）",			&waitFrame		);
			ImGui::DragInt(		u8"狙いを定める時間（フレーム）",	&aimingFrame	);
			ImGui::DragInt(		u8"向き確定から攻撃までの時間（フレーム）",		&intervalFrame	);
			ImGui::Checkbox(	u8"自機を狙うか",					&aimToTarget	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在のパラメータ" ) )
		{
			std::string stateCaption = u8"現在のステート：" + pMover->GetStateName();
			ImGui::Text( stateCaption.c_str() );
			ImGui::DragFloat3(		u8"現在のワールド座標",	&pos.x, 0.01f );

			Donya::Vector3 localFront = orientation.LocalFront();
			ImGui::SliderFloat3(	u8"現在の姿勢", &localFront.x, -1.0f, 1.0f );
			orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

			ImGui::DragInt(			u8"内部タイマ",			&timer );
			element.ShowImGuiNode( /* useTreeNode = */ false, "" );

			ImGui::TreePop();
		}

		if ( ImGui::Button( u8"撃つ" ) )
		{
			// GenerateShot();
			AssignMover<Fire>();
		}

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI

// region Archer
#pragma endregion


#pragma region GateKeeper

	void GateKeeper::MoverBase::Init( GateKeeper &target )
	{
		target.timer = 0;
		target.animator.ResetTimer();
		target.animator.DisableLoop();

		if ( target.pModelParam )
		{
			const auto &initialMotion = target.pModelParam->motionHolder.GetMotion( AcquireMotionIndex() );
			target.animator.SetRepeatRange( initialMotion );
			target.pose.AssignSkeletal( target.animator.CalcCurrentPose( initialMotion ) );
		}
	}

	void GateKeeper::Wait::Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;

		if ( target.waitFrame <= target.timer )
		{
			target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
		}
	}
	int  GateKeeper::Wait::AcquireMotionIndex() const
	{
		return MOTION_INDEX_BEGIN;
	}
	bool GateKeeper::Wait::ShouldChangeState( GateKeeper &target ) const
	{
		return ( target.waitFrame <= target.timer && target.animator.WasEnded() );
	}
	std::function<void()> GateKeeper::Wait::GetChangeStateMethod( GateKeeper &target ) const
	{
		return [&]() { target.AssignMover<Attack>(); };
	}
	#if USE_IMGUI
	std::string GateKeeper::Wait::GetStateName() const { return u8"待機"; }
	#endif // USE_IMGUI

	void GateKeeper::Attack::Init( GateKeeper &target )
	{
		MoverBase::Init( target );

		yaw = 0.0f;
	}
	void GateKeeper::Attack::Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		yaw += target.rotateSpeed;

		target.timer++;
		target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
	}
	int  GateKeeper::Attack::AcquireMotionIndex() const
	{
		return MOTION_INDEX_PROCESS;
	}
	bool GateKeeper::Attack::ShouldChangeState( GateKeeper &target ) const
	{
		return ( target.attackFrame <= target.timer );
	}
	std::function<void()> GateKeeper::Attack::GetChangeStateMethod( GateKeeper &target ) const
	{
		return [&]() { target.AssignMover<Rest>(); };
	}
	Donya::Quaternion GateKeeper::Attack::GetRotation() const
	{
		return Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( yaw ) );
	}
#if USE_IMGUI
	std::string GateKeeper::Attack::GetStateName() const { return u8"攻撃"; }
#endif // USE_IMGUI

	void GateKeeper::Rest::Update( GateKeeper &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		target.timer++;
		target.UpdateMotion( elapsedTime, AcquireMotionIndex() );
	}
	int  GateKeeper::Rest::AcquireMotionIndex() const
	{
		return MOTION_INDEX_END;
	}
	bool GateKeeper::Rest::ShouldChangeState( GateKeeper &target ) const
	{
		return target.animator.WasEnded();
	}
	std::function<void()> GateKeeper::Rest::GetChangeStateMethod( GateKeeper &target ) const
	{
		return [&]() { target.AssignMover<Wait>(); };
	}
#if USE_IMGUI
	std::string GateKeeper::Rest::GetStateName() const { return u8"休憩"; }
#endif // USE_IMGUI

	void GateKeeper::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		timer = 0;
		element.Add( Element::Type::Flame );
		AssignMover<Wait>();
	}
	void GateKeeper::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		pMover->Update( *this, elapsedTime, targetPos );

		if ( pMover->ShouldChangeState( *this ) )
		{
			auto ChangeMover = pMover->GetChangeStateMethod( *this );
			ChangeMover();
		}

		OiledUpdate();
		BurningUpdate();
	}
	void GateKeeper::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		// No op.
	}
	void GateKeeper::Draw( RenderingHelper *pRenderer )
	{
		const Donya::Quaternion physicOrientation = orientation;
		orientation.RotateBy( pMover->GetRotation() );

		Base::Draw( pRenderer );

		orientation = physicOrientation;
	}
	bool GateKeeper::ShouldRemove() const
	{
	#if USE_IMGUI
		if ( wantRemoveByGui ) { return true; }
	#endif // USE_IMGUI
		return WillBurn( element );
	}
	Kind GateKeeper::GetKind() const
	{
		return Kind::GateKeeper;
	}
#if USE_IMGUI
	void GateKeeper::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		const std::string buttonCaption = ( useTreeNode )
						? nodeCaption + u8"を削除"
						: u8"削除";
		if ( ImGui::Button( buttonCaption.c_str() ) )
		{
			wantRemoveByGui = true;
		}

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			initializer.ShowImGuiNode( u8"初期化情報" );
			ImGui::DragInt(		u8"待機時間（フレーム）",		&waitFrame		);
			ImGui::DragInt(		u8"攻撃する時間（フレーム）",	&attackFrame	);
			ImGui::DragFloat(	u8"回転角度（degree）",		&rotateSpeed	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在のパラメータ" ) )
		{
			std::string stateCaption = u8"現在のステート：" + pMover->GetStateName();
			ImGui::Text( stateCaption.c_str() );
			ImGui::DragFloat3(		u8"現在のワールド座標",	&pos.x, 0.01f );

			Donya::Vector3 localFront = orientation.LocalFront();
			ImGui::SliderFloat3(	u8"現在の姿勢", &localFront.x, -1.0f, 1.0f );
			orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

			ImGui::DragInt(			u8"内部タイマ",			&timer );
			element.ShowImGuiNode( /* useTreeNode = */ false, "" );

			ImGui::TreePop();
		}

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI

// region GateKeeper
#pragma endregion


#pragma region Chaser

	void Chaser::MoverBase::Init( Chaser &target )
	{
		target.animator.ResetTimer();
		target.animator.DisableLoop();

		if ( target.pModelParam )
		{
			const auto &initialMotion = target.pModelParam->motionHolder.GetMotion( AcquireMotionIndex( target ) );
			target.animator.SetRepeatRange( initialMotion );
			target.pose.AssignSkeletal( target.animator.CalcCurrentPose( initialMotion ) );
		}
	}
	bool Chaser::MoverBase::IsTargetClose( Chaser &target, const Donya::Vector3 &targetPos ) const
	{
		const Donya::Vector3 toTarget = targetPos - target.GetPosition();
		return ( toTarget.Length() < target.initializer.searchRadius ) ? true : false;
	}
	void Chaser::MoverBase::LookToVelocity( Chaser &target )
	{
		target.orientation = Donya::Quaternion::LookAt
		(
			target.orientation,
			target.velocity.Unit()
		);
	}

	void Chaser::Return::Update( Chaser &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		const Donya::Vector3 returnVec = target.initializer.wsPos - target.pos;
		const float distance = returnVec.Length();

		if ( ZeroEqual( distance ) )
		{
			target.velocity		= 0.0f;
			target.orientation	= target.initializer.orientation;
			target.UpdateMotion( elapsedTime, AcquireMotionIndex( target ) );
			return;
		}
		// else

		if ( distance < target.returnSpeed )
		{
			target.velocity = returnVec.Unit() * distance;
		}
		else
		{
			target.velocity = returnVec.Unit() * target.returnSpeed;
		}

		LookToVelocity( target );
		target.UpdateMotion( elapsedTime, AcquireMotionIndex( target ) );
	}
	int  Chaser::Return::AcquireMotionIndex( const Chaser &target ) const
	{
		return ( target.velocity.IsZero() ) ? MOTION_INDEX_BEGIN : MOTION_INDEX_PROCESS;
	}
	bool Chaser::Return::ShouldChangeState( Chaser &target, const Donya::Vector3 &targetPos ) const
	{
		return IsTargetClose( target, targetPos );
	}
	std::function<void()> Chaser::Return::GetChangeStateMethod( Chaser &target ) const
	{
		return [&]() { target.AssignMover<Chase>(); };
	}
	#if USE_IMGUI
	std::string Chaser::Return::GetStateName() const { return u8"戻り＆待機"; }
	#endif // USE_IMGUI

	void Chaser::Chase::Update( Chaser &target, float elapsedTime, const Donya::Vector3 &targetPos )
	{
		if ( IsTargetClose( target, targetPos ) )
		{
			target.destPos = targetPos;
		}

		const Donya::Vector3 chaseVec = target.destPos - target.pos;
		const float distance = chaseVec.Length();

		if ( ZeroEqual( distance ) )
		{
			target.velocity = 0.0f;
			return;
		}
		// else

		if ( distance < target.chaseSpeed )
		{
			target.velocity = chaseVec.Unit() * distance;
		}
		else
		{
			target.velocity = chaseVec.Unit() * target.chaseSpeed;
		}

		LookToVelocity( target );
		target.UpdateMotion( elapsedTime, AcquireMotionIndex( target ) );
	}
	int  Chaser::Chase::AcquireMotionIndex( const Chaser &target ) const
	{
		return MOTION_INDEX_PROCESS;
	}
	bool Chaser::Chase::ShouldChangeState( Chaser &target, const Donya::Vector3 &targetPos ) const
	{
		return ( target.pos == target.destPos && !IsTargetClose( target, targetPos ) );
	}
	std::function<void()> Chaser::Chase::GetChangeStateMethod( Chaser &target ) const
	{
		return [&]() { target.AssignMover<Return>(); };
	}
#if USE_IMGUI
	std::string Chaser::Chase::GetStateName() const { return u8"追跡"; }
#endif // USE_IMGUI

	void Chaser::Init( const InitializeParam &argInitializer )
	{
		Base::Init( argInitializer );

		destPos  = pos;
		velocity = 0.0f;
		AssignMover<Return>();
	}
	void Chaser::Update( float elapsedTime, const Donya::Vector3 &targetPos )
	{
		pMover->Update( *this, elapsedTime, targetPos );

		if ( pMover->ShouldChangeState( *this, targetPos ) )
		{
			auto ChangeMover = pMover->GetChangeStateMethod( *this );
			ChangeMover();
		}

		OiledUpdate();
		BurningUpdate();
	}
	void Chaser::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
		const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );

		velocity = aabbResult.correctedVector;

		pos += velocity;
	}
	bool Chaser::ShouldRemove() const
	{
	#if USE_IMGUI
		if ( wantRemoveByGui ) { return true; }
	#endif // USE_IMGUI
		return false; // Chaser can't defeat.
	}
	Kind Chaser::GetKind() const
	{
		return Kind::Chaser;
	}
#if USE_IMGUI
	void Chaser::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
	{
		if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		const std::string buttonCaption = ( useTreeNode )
						? nodeCaption + u8"を削除"
						: u8"削除";
		if ( ImGui::Button( buttonCaption.c_str() ) )
		{
			wantRemoveByGui = true;
		}

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			initializer.ShowImGuiNode( u8"初期化情報" );
			ImGui::DragFloat(	u8"追跡速度",	&chaseSpeed		);
			ImGui::DragFloat(	u8"戻る速度",	&returnSpeed	);

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在のパラメータ" ) )
		{
			std::string stateCaption = u8"現在のステート：" + pMover->GetStateName();
			ImGui::Text( stateCaption.c_str() );
			ImGui::DragFloat3(		u8"現在のワールド座標",	&pos.x,			0.01f );
			ImGui::DragFloat3(		u8"現在の速度",			&velocity.x,	0.01f );

			Donya::Vector3 localFront = orientation.LocalFront();
			ImGui::SliderFloat3(	u8"現在の姿勢", &localFront.x, -1.0f, 1.0f );
			orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront );

			element.ShowImGuiNode( /* useTreeNode = */ false, "" );

			ImGui::TreePop();
		}

		if ( useTreeNode ) { ImGui::TreePop(); }
	}
#endif // USE_IMGUI

// region Chaser
#pragma endregion

}
