#include "Boss.h"

#include <algorithm>			// For std::max(), min()
#include <array>

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include "Donya/Constant.h"		// For DEBUG_MODE macro.
#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/Shader.h"
#include "Donya/Sound.h"
#include "Donya/Useful.h"		// For ZeroEqual().

#if DEBUG_MODE
#include "Donya/Keyboard.h"
#endif // DEBUG_MODE

#include "Common.h"
#include "FilePath.h"
#include "Music.h"
#include "Parameter.h"

#undef max
#undef min


namespace
{
	constexpr size_t TYPE_COUNT = scast<size_t>( BossType::BossCount );
	std::string GetBossName( BossType type )
	{
		switch ( type )
		{
		case BossType::Null:	return "Empty";
		case BossType::First:	return "First";
		default: break;
		}
		_ASSERT_EXPR( 0, L"Error : Unexpected type!" );
		return "ERROR";
	}
}


namespace BossModel
{
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Boss/";
	constexpr const char *MODEL_EXTENSION = ".bin";
	constexpr const char *MODEL_NAMES[TYPE_COUNT]
	{
		"First",
	};

	static std::vector<std::shared_ptr<BossBase::ModelResource>> modelPtrs{};

	bool LoadModels()
	{
		// Already has loaded.
		if ( !modelPtrs.empty() ) { return true; }
		// else

		modelPtrs.resize( TYPE_COUNT );

		Donya::Loader loader{};
		auto Load = [&loader]( const std::string &filePath, BossBase::ModelResource *pDest )->bool
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
		for ( size_t i = 0; i < TYPE_COUNT; ++i )
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
			pModel = std::make_shared<BossBase::ModelResource>();
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

	bool IsOutOfRange( BossType kind )
	{
		return ( scast<int>( kind ) < 0 || BossType::BossCount <= kind ) ? true : false;
	}
	std::shared_ptr<BossBase::ModelResource> GetModelPtr( BossType kind )
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
	// "[i]" of these vectors represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == BossModel::TYPE_COUNT

	struct DrawingParam
	{
		float				drawScale = 1.0f;
		Donya::Quaternion	drawRotation;
		Donya::Vector3		drawOffset;
		std::vector<float>	accelerations;

		Donya::Vector4		oilColor{ 1.0f, 1.0f, 1.0f, 1.0f };
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
				CEREAL_NVP( accelerations	),
				CEREAL_NVP( oilColor		),
				CEREAL_NVP( oilAdjustment	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
	struct CollisionParam
	{
		struct PerKind
		{
			std::vector<Donya::AABB> hitBoxes;
			std::vector<Donya::AABB> hurtBoxes;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( hitBoxes	),
					CEREAL_NVP( hurtBoxes	)
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
		DrawingParam		drawer;
		CollisionParam		collider;
		std::vector<int>	initialHPs;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawer		),
				CEREAL_NVP( collider	),
				CEREAL_NVP( initialHPs	)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Member,			0 )
CEREAL_CLASS_VERSION( DrawingParam,		0 )
CEREAL_CLASS_VERSION( CollisionParam,	0 )

class ParamBoss : public ParameterBase<ParamBoss>
{
public:
	static constexpr const char *ID = "Boss";
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

		ResizeVectorIfNeeded();
	}
	Member Data() const { return m; }
private:
	void ResizeVectorIfNeeded()
	{
		auto ResizeIfNeeded = []( auto *pVector, auto initialValue )
		{
			if ( pVector->size() == TYPE_COUNT ) { return; };
			// else

			pVector->resize( TYPE_COUNT, initialValue );
		};

		ResizeIfNeeded( &m.drawer.accelerations, 1.0f );

		CollisionParam::PerKind defaultHitBox;
		defaultHitBox.hitBoxes.resize( 1 );
		defaultHitBox.hurtBoxes.resize( 1 );
		defaultHitBox.hitBoxes.back().size  = 1.0f;
		defaultHitBox.hurtBoxes.back().size = 1.0f;
		ResizeIfNeeded( &m.collider.collisions, defaultHitBox );
		
		ResizeIfNeeded( &m.initialHPs, 3 );
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

		if ( ImGui::TreeNode( u8"ボスのパラメータ調整" ) )
		{
			ResizeVectorIfNeeded();

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
					for ( size_t i = 0; i < TYPE_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
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
				auto ShowPerKind = []( const std::string &nodeCaption, CollisionParam::PerKind *p )
				{
					if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
					// else

					auto ShowHitBoxes = []( const std::string &nodeCaption, std::vector<Donya::AABB> *p )
					{
						if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
						// else
						
						if ( ImGui::Button( u8"追加" ) )
						{
							Donya::AABB tmp{};
							tmp.size = 1.0f;
							p->emplace_back( std::move( tmp ) );
						}
						if ( p->size() && ImGui::Button( u8"末尾を削除" ) )
						{
							p->pop_back();
						}

						const size_t count = p->size();
						size_t eraseIndex = count;
						std::string caption{};
						for ( size_t i = 0; i < count; ++i )
						{
							caption = Donya::MakeArraySuffix( i );

							if ( ImGui::Button( ( caption + u8"削除" ).c_str() ) )
							{
								eraseIndex = i;
							}

							ParameterHelper::ShowAABBNode( ( caption + u8"調整" ).c_str(), &p->at( i ) );
						}

						if ( eraseIndex != count )
						{
							p->erase( p->begin() + eraseIndex );
						}

						ImGui::TreePop();
					};

					ShowHitBoxes( u8"当たり判定", &p->hitBoxes	);
					ShowHitBoxes( u8"喰らい判定", &p->hurtBoxes	);

					ImGui::TreePop();
				};

				auto &vector = m.collider.collisions;

				std::string caption{};
				for ( size_t i = 0; i < TYPE_COUNT; ++i )
				{
					caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
				
					if ( ImGui::Button( u8"追加" ) )
					{
						CollisionParam::PerKind tmp;
						tmp.hitBoxes.resize( 1 );
						tmp.hurtBoxes.resize( 1 );
						tmp.hitBoxes.back().size = 1.0f;
						tmp.hurtBoxes.back().size = 1.0f;
						vector.emplace_back( std::move( tmp ) );
					}
					if ( vector.size() && ImGui::Button( u8"末尾を削除" ) )
					{
						vector.pop_back();
					}

					ShowPerKind( u8"調整", &vector[i] );
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"その他" ) )
			{
				if ( ImGui::TreeNode( u8"最大体力" ) )
				{
					std::string caption{};
					for ( size_t i = 0; i < TYPE_COUNT; ++i )
					{
						caption = "[" + std::to_string( i ) + ":" + BossModel::MODEL_NAMES[i] + "]";
						ImGui::InputInt( caption.c_str(), &m.initialHPs[i] );
						m.initialHPs[i] = std::max( 1, m.initialHPs[i] );
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
	Member FetchMember()
	{
		return ParamBoss::Get().Data();
	}

	Donya::Vector4x4 MakeWorldMatrix( const Donya::Vector3 &wsPos, const Donya::AABB &localHitBox, const Donya::Quaternion &rotation = Donya::Quaternion::Identity() )
	{
		// The size is half.
		// But that will using as scale, so we should multiply to double.
		
		Donya::Vector4x4 m{};
		m._11 = localHitBox.size.x * 2.0f;
		m._22 = localHitBox.size.y * 2.0f;
		m._33 = localHitBox.size.z * 2.0f;
		m    *= rotation.MakeRotationMatrix();
		m._41 = localHitBox.pos.x + wsPos.x;
		m._42 = localHitBox.pos.y + wsPos.y;
		m._43 = localHitBox.pos.z + wsPos.z;
		return m;
	}
	void DrawCube( RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		Donya::Model::Cube::Constant constant;
		constant.matWorld		= W;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		constant.lightDirection	= -Donya::Vector3::Up();
		pRenderer->ProcessDrawingCube( constant );
	}
}


bool				BossInitializer::ShouldGenerateBoss() const
{
	return ( type == BossType::Null || type == BossType::BossCount ) ? false : true;
}
BossType			BossInitializer::GetType() const { return type; }
Donya::Vector3		BossInitializer::GetInitialPos() const { return wsInitialPos; }
Donya::Quaternion	BossInitializer::GetInitialOrientation() const { return initialOrientation; }
void BossInitializer::LoadParameter( int stageNo )
{
#if DEBUG_MODE
	LoadJson( stageNo );
#else
	LoadBin( stageNo );
#endif // DEBUG_MODE
}
void BossInitializer::LoadBin ( int stageNo )
{
	constexpr bool fromBinary = true;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
void BossInitializer::LoadJson( int stageNo )
{
	constexpr bool fromBinary = false;
	Donya::Serializer::Load( *this, MakeStageParamPath( ID, stageNo, fromBinary ).c_str(), ID, fromBinary );
}
#if USE_IMGUI
void BossInitializer::SaveBin ( int stageNo )
{
	constexpr bool fromBinary = true;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void BossInitializer::SaveJson( int stageNo )
{
	constexpr bool fromBinary = false;

	const std::string filePath = MakeStageParamPath( ID, stageNo, fromBinary );
	MakeFileIfNotExists( filePath, fromBinary );

	Donya::Serializer::Save( *this, filePath.c_str(), ID, fromBinary );
}
void BossInitializer::ShowImGuiNode( const std::string &nodeCaption, int stageNo, bool allowShowIONode )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	int intType = scast<int>( type );
	ImGui::SliderInt( u8"ボスの種類", &intType, -1, TYPE_COUNT - 1 );
	type = scast<BossType>( intType );
	ImGui::Text( u8"現在の種類：%s", GetBossName( type ).c_str() );
	ImGui::Text( u8"%s だと出現しません", GetBossName( BossType::Null ).c_str() );

	ImGui::DragFloat3( u8"初期のワールド座標", &wsInitialPos.x, 0.01f );
	
	Donya::Vector3 lookDir = initialOrientation.LocalFront();
	ImGui::SliderFloat3( u8"初期の前方向", &lookDir.x, -1.0f, 1.0f );

	initialOrientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), lookDir.Unit(), Donya::Quaternion::Freeze::Up );

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
	if ( allowShowIONode )
	{
		ShowIONode();
	}

	ImGui::TreePop();
}
#endif // USE_IMGUI


bool BossBase::LoadModels()
{
	return BossModel::LoadModels();
}
bool BossBase::AssignDerivedClass( std::unique_ptr<BossBase> *pTarget, BossType assignType )
{
	switch ( assignType )
	{
	case BossType::First:	*pTarget = std::make_unique<BossFirst>(); return true;
	default: break;
	}
	_ASSERT_EXPR( 0, L"Error: Unexpected type!" );
	return false;
}
void BossBase::Init( const BossInitializer &param )
{
	ParamBoss::Get().Init();

	pos				= param.GetInitialPos();
	element			= Element::Type::Nil;
	velocity		= 0.0f;
	orientation		= param.GetInitialOrientation();

	const int intType = scast<int>( GetType() );
	const auto hpData = FetchMember().initialHPs;
	hp = ( intType < scast<int>( hpData.size() ) ) ? hpData[intType] : 1;

	model.pResource	= BossModel::GetModelPtr( GetType() );
	AssignSpecifyPose( 0 );
}
void BossBase::Update( float elapsedTime, const Donya::Vector3 &targetPos )
{
#if USE_IMGUI
	ParamBoss::Get().UseImGui();
#endif // USE_IMGUI

	if ( IsDead() ) { return; }
	// else

	UpdateMotion( elapsedTime, 0 );
}
void BossBase::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMat )
{
	if ( IsDead() ) { return; }
	// else

	const auto data = FetchMember();
	
	const auto result = Actor::Move( velocity, {}, solids, pTerrain, pTerrainMat );
	const Donya::Vector3 standingNormal = result.lastNormal;
	
	// If now standing on some plane, that means corrected to vertically.
	bool wasCorrectedV = !standingNormal.IsZero();
	if ( wasCorrectedV )
	{
		velocity.y = 0.0f;
	}
}
void BossBase::Draw( RenderingHelper *pRenderer ) const
{
	if ( !model.pResource ) { return; }
	// else

	const auto drawData = FetchMember().drawer;
	Donya::Model::Constants::PerModel::Common modelConstant{};
	modelConstant.drawColor		= CalcDrawColor();
	modelConstant.worldMatrix	= CalcWorldMatrix( /* useForDrawing = */ true );
	RenderingHelper::AdjustColorConstant colorConstant =
		( element.Has( Element::Type::Oil ) )
		? drawData.oilAdjustment
		: RenderingHelper::AdjustColorConstant::MakeDefault();
	pRenderer->UpdateConstant( modelConstant );
	pRenderer->UpdateConstant( colorConstant );
	pRenderer->ActivateConstantModel();
	pRenderer->ActivateConstantAdjustColor();

	pRenderer->Render( model.pResource->model, model.pose );

	pRenderer->DeactivateConstantAdjustColor();
	pRenderer->DeactivateConstantModel();
}
void BossBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP ) const
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else
#if DEBUG_MODE

	const auto data			= FetchMember();
	const auto &collisions	= data.collider.collisions;
	const int  intType		= scast<int>( GetType() );

	if ( intType < 0 || scast<int>( collisions.size() ) <= intType ) { return; }
	// else

	const auto &perType		= data.collider.collisions[intType];
	const auto &hitBoxes	= perType.hitBoxes;
	const auto &hurtBoxes	= perType.hurtBoxes;

	constexpr Donya::Vector4	hitColor { 0.3f, 0.0f, 0.0f, 0.5f };
	constexpr Donya::Vector4	hurtColor{ 0.0f, 0.3f, 0.0f, 0.5f };

	Donya::Vector4x4 W;
	for ( const auto &it : hitBoxes )
	{
		W = MakeWorldMatrix( pos, it, orientation );
		DrawCube( pRenderer, W, matVP, hitColor );
	}
	for ( const auto &it : hurtBoxes )
	{
		W = MakeWorldMatrix( pos, it, orientation );
		DrawCube( pRenderer, W, matVP, hurtColor );
	}
#endif // DEBUG_MODE
}
void BossBase::MakeDamage( const Element &effect ) const
{
	element.Add( effect.Get() );
}
bool BossBase::IsDead() const
{
	return ( hp <= 0 );
}
void BossBase::AssignSpecifyPose( int motionIndex )
{
	if ( !model.pResource ) { return; }
	// else

	const int motionCount = scast<int>( model.pResource->motionHolder.GetMotionCount() );
	motionIndex = std::max( 0, std::min( motionCount - 1, motionIndex ) );

	const auto &applyMotion = model.pResource->motionHolder.GetMotion( motionIndex );
	model.animator.SetRepeatRange( applyMotion );
	model.pose.AssignSkeletal
	(
		model.animator.CalcCurrentPose
		(
			applyMotion
		)
	);
}
void BossBase::UpdateMotion( float elapsedTime, int motionIndex )
{
	if ( !model.pResource ) { return; }
	// else

	const auto data			= FetchMember();
	const auto &speedSource = data.drawer.accelerations;

	const size_t intType	= scast<size_t>( GetType() );
	const float  playSpeed	= ( intType < speedSource.size() ) ? speedSource[intType] : 1.0f;
	model.animator.Update( elapsedTime * playSpeed );

	if ( model.pResource )
	{
		AssignSpecifyPose( motionIndex );
	}
}
Donya::Vector4		BossBase::CalcDrawColor() const
{
	const auto data = FetchMember();
	Donya::Vector4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	if ( element.Has( Element::Type::Oil ) ) { baseColor.Product( data.drawer.oilColor ); }

	return baseColor;
}
Donya::Vector4x4	BossBase::CalcWorldMatrix( bool useForDrawing ) const
{
	const auto data = FetchMember();
	Donya::Vector4x4 W{};
		
	if ( useForHitBox || useForHurtBox )
	{
		W._11 = 2.0f;
		W._22 = 2.0f;
		W._33 = 2.0f;
		W._41 = pos.x;
		W._42 = pos.y;
		W._43 = pos.z;

		const auto	&collisions	= data.collider.collisions;
		const int	intType		= scast<int>( GetType() );

		if ( intType < 0 || scast<int>( collisions.size() ) <= intType ) { return W; }
		// else

		const auto	&perType	= data.collider.collisions[intType];
		const auto	&applyBoxes	= ( useForHitBox ) ? perType.hitBoxes : perType.hurtBoxes;
		if ( applyBoxes.empty() ) { return W; }
		// else
		const auto &applyBox = applyBoxes.front();

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
		W *= data.drawer.drawRotation.MakeRotationMatrix();
	}

	W._41 = pos.x;
	W._42 = pos.y;
	W._43 = pos.z;

	if ( useForDrawing )
	{
		W._41 += data.drawer.drawOffset.x;
		W._42 += data.drawer.drawOffset.y;
		W._43 += data.drawer.drawOffset.z;
	}

	return W;
}


BossType BossFirst::GetType() const { return BossType::First; }
#if USE_IMGUI
void BossFirst::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::Text( u8"種類：%s", GetBossName( GetType() ).c_str() );

	ImGui::DragInt( u8"現在の体力", &hp ); hp = std::max( 0, hp );
	ImGui::DragFloat3( u8"現在の座標", &pos.x,		0.01f );
	ImGui::DragFloat3( u8"現在の速度", &velocity.x,	0.01f );

	Donya::Vector3 localFront = orientation.LocalFront();
	ImGui::SliderFloat3( u8"現在の前方向", &localFront.x,	 -1.0f, 1.0f );
	orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), localFront, Donya::Quaternion::Freeze::Up );

	element.ShowImGuiNode( /* useTreeNode = */ false, "" );

	ImGui::TreePop();
}
#endif // USE_IMGUI
