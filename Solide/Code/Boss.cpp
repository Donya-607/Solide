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

namespace BossModel
{
	enum Kind
	{
		First,

		KindCount
	};
	constexpr size_t KIND_COUNT = scast<size_t>( KindCount );
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Boss/";
	constexpr const char *MODEL_EXTENSION = ".bin";
	constexpr const char *MODEL_NAMES[KIND_COUNT]
	{
		"First",
	};

	static std::vector<std::shared_ptr<BossBase::ModelResource>> modelPtrs{};

	bool LoadModels()
	{
		// Already has loaded.
		if ( !modelPtrs.empty() ) { return true; }
		// else

		modelPtrs.resize( KIND_COUNT );

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

	bool IsOutOfRange( Kind kind )
	{
		return ( scast<int>( kind ) < 0 || Kind::KindCount <= kind ) ? true : false;
	}
	const std::shared_ptr<BossBase::ModelResource> GetModelPtr( Kind kind )
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
	// "[i]" of these vectors represents a value of static_cast<enumKind>( i ). This size was guaranteed to: size() == BossModel::KIND_COUNT

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
		DrawingParam	drawer;
		CollisionParam	collider;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( drawer		),
				CEREAL_NVP( collider	)
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
			if ( pVector->size() != BossModel::KIND_COUNT ) { return; };
			// else

			pVector->resize( BossModel::KIND_COUNT, initialValue );
		};

		ResizeIfNeeded( &m.drawer.accelerations, 1.0f );

		CollisionParam::PerKind defaultHitBox;
		defaultHitBox.hitBoxes.resize( 1 );
		defaultHitBox.hurtBoxes.resize( 1 );
		defaultHitBox.hitBoxes.back().size  = 1.0f;
		defaultHitBox.hurtBoxes.back().size = 1.0f;
		ResizeIfNeeded( &m.collider.collisions, defaultHitBox );
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
					for ( size_t i = 0; i < BossModel::KIND_COUNT; ++i )
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

							ImGui::TreePop();
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
				for ( size_t i = 0; i < BossModel::KIND_COUNT; ++i )
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

			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};


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


// Internal utility.
namespace
{
	Member FetchMember()
	{
		return ParamBoss::Get().Data();
	}
}


void BossBase::Init( const BossInitializer &param )
{
	ParamBoss::Get().Init();

	pos			= param.GetInitialPos();
	element		= Element::Type::Nil;
	velocity	= 0.0f;
	orientation	= param.GetInitialOrientation();
}

void BossBase::Update( float elapsedTime, const Donya::Vector3 &targetPos )
{
#if USE_IMGUI
	ParamBoss::Get().UseImGui();
#endif // USE_IMGUI

	if ( IsDead() ) { return; }
	// else
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

void BossBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP )
{
	if ( !Common::IsShowCollision() || !pRenderer ) { return; }
	// else
#if DEBUG_MODE
	constexpr Donya::Vector4 color{ 0.0f, 0.3f, 0.0f, 0.5f };
	Actor::DrawHitBox( pRenderer, matVP, Donya::Quaternion::Identity(), color );
#endif // DEBUG_MODE
}

void BossBase::MakeDamage( const Element &effect ) const
{
	element.Add( effect.Get() );
}
