#include "Bullet.h"

#include <algorithm>
#include <array>

#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/Useful.h"

#include "Effect.h"
#include "FilePath.h"
#include "Section.h" // Use for DrawHitBox.
#include "Parameter.h"

namespace Bullet
{
	namespace
	{
		constexpr size_t KIND_COUNT = scast<size_t>( Kind::KindCount );
		constexpr const char *MODEL_DIRECTORY = "./Data/Models/Bullet/";
		constexpr const char *EXTENSION = ".bin";
		constexpr std::array<const char *, KIND_COUNT> MODEL_NAMES
		{
			"Oil",
			"Flame",
			"Ice",
			"Arrow",
			"Breath",
			"Burning",
		};

		struct StorageBundle
		{
			Donya::Model::StaticModel	model;
			Donya::Model::Pose			pose;
		};
		static std::array<std::shared_ptr<StorageBundle>, KIND_COUNT> models{};

		bool LoadModels()
		{
			auto  HasLoaded = []( const std::array<std::shared_ptr<StorageBundle>, KIND_COUNT> &models)
			{
				for ( const auto &it : models )
				{
					if ( it )
					{
						return false;
					}
				}
				return true;
			};
			if ( !HasLoaded( models ) ) { return true; }
			// else

			Donya::Loader loader{};
			auto Load = [&loader]( const std::string &filePath, StorageBundle *pDest )->bool
			{
				bool result = true;

				loader.ClearData();
				if ( !loader.Load( filePath ) ) { return false; }
				// else

				const auto &source = loader.GetModelSource();

				pDest->model = Donya::Model::StaticModel::Create( source, loader.GetFileDirectory() );
				pDest->pose.AssignSkeletal( source.skeletal );

				return pDest->model.WasInitializeSucceeded();
			};

			bool result		= true;
			bool succeeded	= true;

			std::string filePath{};
			const std::string prefix = MODEL_DIRECTORY;
			for ( size_t i = 0; i < KIND_COUNT; ++i )
			{
				filePath = prefix + MODEL_NAMES[i] + EXTENSION;
				if ( !Donya::IsExistFile( filePath ) )
				{
					const std::string outputMsgBase{ "Error : The model file does not exist. That is : " };
					Donya::OutputDebugStr( ( outputMsgBase + "[" + filePath + "]" + "\n" ).c_str() );
					continue;
				}
				// else

				models[i] = std::make_shared<StorageBundle>();
				result = Load( filePath, &( *models[i] ) ); // std::shared_ptr<T> -> T -> T *
				if ( !result )
				{
					const std::wstring errMsgBase{ L"Failed : Loading a model. That is : " };
					const std::wstring errMsg = errMsgBase + Donya::MultiToWide( filePath );
					_ASSERT_EXPR( 0, errMsg.c_str() );

					succeeded = false;
					models[i].reset(); // Indicates that it has not been loaded.
				}
			}

			return succeeded;
		}

		bool IsOutOfRange( Kind kind )
		{
			constexpr int intEnd  = scast<int>( Kind::KindCount );
			const     int intKind = scast<int>( kind );
			return ( intKind < 0 || intEnd <= intKind ) ? true : false;
		}
		std::shared_ptr<StorageBundle> GetModelPtr( Kind kind )
		{
			if ( IsOutOfRange( kind ) )
			{
				_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
				return nullptr;
			}
			// else

			return models[scast<int>( kind )];
		}

		void DrawModel( Kind kind, RenderingHelper *pRenderer, const BulletBase &bullet, const Donya::Vector4 &color )
		{
			if ( !pRenderer ) { return; }
			// else

			const auto pModel = GetModelPtr( kind );
			if ( !pModel ) { return; }
			// else

			Donya::Model::Constants::PerModel::Common constant{};
			constant.drawColor		= color;
			constant.worldMatrix	= bullet.GetWorldMatrix();
			pRenderer->UpdateConstant( constant );
			pRenderer->ActivateConstantModel();

			pRenderer->Render( pModel->model, pModel->pose );

			pRenderer->DeactivateConstantModel();
		}
	}

	// HACK : This method using switch-case statement.
	std::shared_ptr<BulletBase> MakeBullet( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return nullptr;
		}
		// else

		switch ( kind )
		{
		case Kind::Oil:			return std::make_shared<Impl::OilBullet>();
		case Kind::FlameSmoke:	return std::make_shared<Impl::FlameSmoke>();
		case Kind::IceSmoke:	return std::make_shared<Impl::IceSmoke>();
		case Kind::Arrow:		return std::make_shared<Impl::Arrow>();
		case Kind::Breath:		return std::make_shared<Impl::Breath>();
		case Kind::Burning:		return std::make_shared<Impl::Burning>();
		default: _ASSERT_EXPR( 0, L"Error : Unexpected bullet kind!" );	break;
		}
		return nullptr;
	}

	// HACK : These parameter structs has many same member.

	struct OilMember
	{
		float			gravity		= 1.0f;	// Absolute value.
		float			drawScale	= 1.0f;
		Donya::AABB		hitBox;
		Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
		int				aliveFrame = 1;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( gravity ),
				CEREAL_NVP( hitBox )
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( drawScale ) );
			}
			if ( 2 <= version )
			{
				archive( CEREAL_NVP( color ) );
			}
			if ( 3 <= version )
			{
				archive( CEREAL_NVP( aliveFrame ) );
			}
			if ( 4 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragInt   ( u8"生存時間（フレーム）", &aliveFrame );
			aliveFrame	= std::max( 0, aliveFrame );

			ImGui::DragFloat ( u8"重力",			&gravity,	0.01f );
			ImGui::DragFloat ( u8"描画スケール",	&drawScale,	0.01f );
			gravity		= std::max( 0.0f, gravity );
			drawScale	= std::max( 0.0f, drawScale );


			ImGui::ColorEdit4( u8"色",			&color.x );

			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};
	
	struct SmokeMember
	{
		struct General
		{
			int				aliveFrame	= 1;
			float			drawScale	= 1.0f;
			Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( aliveFrame	),
					CEREAL_NVP( drawScale	),
					CEREAL_NVP( color		)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		
		struct Flame
		{
			General			general;
			Donya::Sphere	hitBox;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( general	),
					CEREAL_NVP( hitBox	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};
		
		struct Ice
		{
			General			general;
			Donya::Sphere	hitBox;
		private:
			friend class cereal::access;
			template<class Archive>
			void serialize( Archive &archive, std::uint32_t version )
			{
				archive
				(
					CEREAL_NVP( general	),
					CEREAL_NVP( hitBox	)
				);

				if ( 1 <= version )
				{
					// archive( CEREAL_NVP( x ) );
				}
			}
		};

		Flame	flame;
		Ice		ice;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( flame	),
				CEREAL_NVP( ice		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			auto Clamp = []( auto *pTarget, auto min, auto max )
			{
				*pTarget = std::max( min, std::min( max, *pTarget ) );
			};

			auto GeneralNode = [&Clamp]( const std::string &nodeCaption, General *pTarget )
			{
				if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
				// else
				/*
					int				aliveFrame	= 1;
					float			drawScale	= 1.0f;
					Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
				*/
				ImGui::DragInt(		u8"生存時間（フレーム）",	&pTarget->aliveFrame );
				ImGui::DragFloat(	u8"描画スケール",			&pTarget->drawScale, 0.01f );
				ImGui::ColorEdit4(	u8"色",					&pTarget->color.x );
				Clamp( &pTarget->aliveFrame,	0,		pTarget->aliveFrame	);
				Clamp( &pTarget->drawScale,		0.0f,	pTarget->drawScale	);

				ImGui::TreePop();
			};

			if ( ImGui::TreeNode( u8"炎" ) )
			{
				GeneralNode( u8"共通部分", &flame.general );
				ParameterHelper::ShowSphereNode( u8"当たり判定", &flame.hitBox );

				ImGui::TreePop();
			}
			if ( ImGui::TreeNode( u8"冷気" ) )
			{
				GeneralNode( u8"共通部分", &ice.general );
				ParameterHelper::ShowSphereNode( u8"当たり判定", &ice.hitBox );

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};

	struct ArrowMember
	{
		int				aliveFrame	= 1;
		float			gravity		= 1.0f;	// Absolute value.
		float			drawScale	= 1.0f;
		Donya::AABB		hitBox;
		Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( aliveFrame	),
				CEREAL_NVP( gravity		),
				CEREAL_NVP( hitBox		),
				CEREAL_NVP( drawScale	),
				CEREAL_NVP( color		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragInt   ( u8"生存時間（フレーム）", &aliveFrame );
			aliveFrame	= std::max( 0, aliveFrame );

			ImGui::DragFloat ( u8"重力",			&gravity,	0.01f );
			ImGui::DragFloat ( u8"描画スケール",	&drawScale,	0.01f );
			gravity		= std::max( 0.0f, gravity );
			drawScale	= std::max( 0.0f, drawScale );


			ImGui::ColorEdit4( u8"色",			&color.x );

			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};

	struct BreathMember
	{
		int				aliveFrame	= 1;
		float			drawScale	= 1.0f;
		Donya::AABB		hitBox;
		Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( aliveFrame	),
				CEREAL_NVP( hitBox		),
				CEREAL_NVP( drawScale	),
				CEREAL_NVP( color		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragInt   ( u8"生存時間（フレーム）", &aliveFrame );
			aliveFrame	= std::max( 0, aliveFrame );

			ImGui::DragFloat ( u8"描画スケール",	&drawScale,	0.01f );
			drawScale	= std::max( 0.0f, drawScale );


			ImGui::ColorEdit4( u8"色",			&color.x );

			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};

	struct BurningMember
	{
		int				aliveFrame	= 1;
		float			drawScale	= 1.0f;
		Donya::AABB		hitBox;
		Donya::Vector4	color{ 1.0f, 1.0f, 1.0f, 1.0f };
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( aliveFrame	),
				CEREAL_NVP( hitBox		),
				CEREAL_NVP( drawScale	),
				CEREAL_NVP( color		)
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragInt   ( u8"生存時間（フレーム）", &aliveFrame );
			aliveFrame	= std::max( 0, aliveFrame );

			ImGui::DragFloat ( u8"描画スケール",	&drawScale,	0.01f );
			drawScale	= std::max( 0.0f, drawScale );


			ImGui::ColorEdit4( u8"色",			&color.x );

			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};

	struct Member
	{
		OilMember		oil;
		SmokeMember		smoke;
		ArrowMember		arrow;
		BreathMember	breath;
		BurningMember	burning;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( oil		),
				CEREAL_NVP( smoke	)
			);

			if ( 1 <= version )
			{
				archive( CEREAL_NVP( arrow ) );
			}
			if ( 2 <= version )
			{
				archive
				(
					CEREAL_NVP( breath  ),
					CEREAL_NVP( burning )
				);
			}
			if ( 3 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
}
CEREAL_CLASS_VERSION( Bullet::OilMember,			3 )
CEREAL_CLASS_VERSION( Bullet::SmokeMember,			0 )
CEREAL_CLASS_VERSION( Bullet::SmokeMember::General,	0 )
CEREAL_CLASS_VERSION( Bullet::SmokeMember::Flame,	0 )
CEREAL_CLASS_VERSION( Bullet::SmokeMember::Ice,		0 )
CEREAL_CLASS_VERSION( Bullet::ArrowMember,			0 )
CEREAL_CLASS_VERSION( Bullet::BreathMember,			0 )
CEREAL_CLASS_VERSION( Bullet::BurningMember,		0 )
CEREAL_CLASS_VERSION( Bullet::Member,				2 )

class ParamBullet : public ParameterBase<ParamBullet>
{
public:
	static constexpr const char *ID = "Bullet";
private:
	Bullet::Member m;
public:
	void Init() override
	{
	#if DEBUG_MODE
		constexpr bool fromBinary = false;
	#else
		constexpr bool fromBinary = true;
	#endif // DEBUG_MODE

		Load( m, fromBinary );
	}
	Bullet::Member Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"弾関連のパラメータ" ) )
		{
			m.oil.ShowImGuiNode( u8"オイル弾" );

			m.smoke.ShowImGuiNode( u8"ケムリ関係" );

			m.arrow.ShowImGuiNode( u8"矢関係" );
			
			m.breath.ShowImGuiNode( u8"ブレス" );
			
			m.burning.ShowImGuiNode( u8"炎上" );

			ShowIONode( m );
			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};


namespace
{
	static constexpr int RECURSION_RAY_COUNT = 4;

#if USE_IMGUI
	std::string GetKindName( Bullet::Kind kind )
	{
		return	( kind == Bullet::Kind::KindCount )
				? "ERROR_KIND"
				: Bullet::MODEL_NAMES[scast<int>( kind )];
	}
#endif // USE_IMGUI
}


namespace Bullet
{
	bool LoadBulletsResource()
	{
		bool succeeded = true;

		if ( !LoadModels() ) { succeeded = false; }

		ParamBullet::Get().Init();

		return succeeded;
	}
	std::string GetBulletName( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return "";
		}
		// else

		return std::string{ MODEL_NAMES[scast<int>( kind )] };
	}
#if USE_IMGUI
	void UseBulletsImGui()
	{
		ParamBullet::Get().UseImGui();
	}

	void BulletAdmin::FireDesc::ShowImGuiNode( const std::string &nodeCaption, bool generatePosIsRelative )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		int intKind = scast<int>( kind );
		ImGui::SliderInt( u8"種類の変更", &intKind, 0, KIND_COUNT - 1 );
		kind = scast<Kind>( intKind );
		ImGui::Text( u8"いま：%s", GetKindName( kind ).c_str() );

		addElement.ShowImGuiNode( /* useTreeNode = */ false, u8"追加属性" );
		ImGui::Text( "" );

		ImGui::DragFloat( u8"速度", &speed, 0.01f );
		speed = std::max( 0.0f, speed );

		ImGui::SliderFloat3( u8"方向", &direction.x, -1.0f, 1.0f );
		if ( ImGui::Button( u8"方向を正規化" ) ) { direction.Normalize(); }

		std::string genPosCaption{ u8"生成位置" };
		genPosCaption += ( generatePosIsRelative ) ? u8"（相対）" : u8"（絶対）";
		ImGui::DragFloat3( genPosCaption.c_str(), &generatePos.x, 0.01f );


		ImGui::TreePop();
	}
#endif // USE_IMGUI


	void BulletAdmin::Init()
	{
		bulletPtrs.clear();
	}
	void BulletAdmin::Uninit()
	{
		bulletPtrs.clear();
	}
	void BulletAdmin::Update( float elapsedTime )
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Update( elapsedTime );
		}

		auto result = std::remove_if
		(
			bulletPtrs.begin(), bulletPtrs.end(),
			[]( std::shared_ptr<BulletBase> &pElement )
			{
				return ( !pElement ) ? true : pElement->ShouldRemove();
			}
		);
		for ( auto it = result; it != bulletPtrs.end(); ++it )
		{
			if ( *it ) { ( *it )->Uninit(); }
		}
		bulletPtrs.erase( result, bulletPtrs.end() );
	}
	void BulletAdmin::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->PhysicUpdate( solids, pTerrain, pTerrainMatrix );
		}
	}
	void BulletAdmin::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->Draw( pRenderer, color );
		}
	}
	void BulletAdmin::DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->DrawHitBox( pRenderer, VP, color );
		}
	}
	void BulletAdmin::Append( const FireDesc &param )
	{
		std::shared_ptr<BulletBase> tmp = MakeBullet( param.kind );
		if ( !tmp ) { assert( !"Unexpected error in bullet." ); return; }
		// else

		bulletPtrs.emplace_back( std::move( tmp ) );
		bulletPtrs.back()->Init( param );
	}
	size_t BulletAdmin::GetBulletCount() const
	{
		return bulletPtrs.size();
	}
	bool BulletAdmin::IsOutOfRange( size_t index ) const
	{
		return ( index < GetBulletCount() ) ? false : true;
	}
	const std::shared_ptr<BulletBase> BulletAdmin::GetBulletPtrOrNull( size_t index ) const
	{
		if ( IsOutOfRange( index ) ) { return nullptr; }
		// else
		return bulletPtrs[index];
	}


	void BulletBase::Init( const BulletAdmin::FireDesc &param )
	{
		AttachSelfKind();
		element.Add(  param.addElement.Get() );
		pos			= param.generatePos;
		velocity	= param.direction * param.speed;
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), param.direction );
	}
	void BulletBase::Update( float elapsedTime )
	{
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), velocity.Unit() );
	}
	void BulletBase::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
	{
		const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
		const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );

		velocity = aabbResult.correctedVector;

		pos += velocity;
	}
	void BulletBase::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
	{
		Bullet::DrawModel( kind, pRenderer, *this, color );
	}
	void BulletBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		if ( !pRenderer ) { return; }
		// else

		const auto &body = GetHitBoxAABB();
		if ( body == Donya::AABB::Nil() ) { return; }
		// else

		Donya::Vector4x4 world{};
		world._11 = body.size.x * 2.0f; // Half size -> Whole size
		world._22 = body.size.y * 2.0f; // Half size -> Whole size
		world._33 = body.size.z * 2.0f; // Half size -> Whole size
		world._41 = body.pos.x;
		world._42 = body.pos.y;
		world._43 = body.pos.z;

		Donya::Model::Cube::Constant constant{};
		constant.matWorld		= world;
		constant.matViewProj	= VP;
		constant.drawColor		= color;
		pRenderer->ProcessDrawingCube( constant );
	}
	BulletBase::AABBResult		BulletBase::CalcCorrectedVector( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
	{
		AABBResult defaultResult{};
		defaultResult.correctedVector = vector;
		defaultResult.wasHit = false;

		return	( solids.empty() )
				? defaultResult
				: CalcCorrectedVectorImpl( vector, solids );
	}
	BulletBase::RecursionResult	BulletBase::CalcCorrectedVector( int recursionLimit, const Donya::Vector3 &vector, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix ) const
	{
		RecursionResult initial{};
		initial.correctedVector			= vector;
		initial.raycastResult.wasHit	= false;

		return	( !pTerrain || !pTerrainMatrix )
				? initial
				: CalcCorrectedVectorImpl( recursionLimit, 0, initial, *pTerrain, *pTerrainMatrix );
	}
	BulletBase::AABBResult		BulletBase::CalcCorrectedVectorImpl( const Donya::Vector3 &vector, const std::vector<Donya::AABB> &solids ) const
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
		Donya::AABB movedBody = GetHitBoxAABB();
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

		const Donya::Vector3 &destination = movedBody.pos;
		// const Donya::Vector3 &destination = movedBody.pos - hitBox.pos/* Except the offset of hitBox */;
		
		result.correctedVector = destination - pos;
		return result;
	}
	BulletBase::RecursionResult BulletBase::CalcCorrectedVectorImpl( int recursionLimit, int recursionCount, RecursionResult inheritedResult, const Donya::Model::PolygonGroup &terrain, const Donya::Vector4x4 &terrainMatrix ) const
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
	void BulletBase::GiveElement( Element::Type addType )
	{
		element.Add( addType );
	}
	Donya::Vector4x4 BulletBase::GetWorldMatrix() const
	{
		Donya::Vector4x4 world = orientation.MakeRotationMatrix();
		world._41 = pos.x;
		world._42 = pos.y;
		world._43 = pos.z;
		return world;
	}
	void BulletBase::HitToObject() const
	{
		wasHitToObject = true;
	}
#if USE_IMGUI
	bool BulletBase::ShowImGuiNode( const std::string &nodeCaption )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
		// else

		bool wantRemove = false;
		if ( ImGui::Button( u8"取り除く" ) ) { wantRemove = true; }

		ImGui::Text( u8"種類：%s", GetKindName( kind ).c_str() );

		ImGui::DragFloat3( u8"ワールド座標", &pos.x,			0.1f );
		ImGui::DragFloat3( u8"ワールド速度", &velocity.x,		0.1f );
		ImGui::SliderFloat4( u8"姿勢",		&orientation.x,	-1.0f, 1.0f );

		ImGui::TreePop();
		return wantRemove;
	}
#endif // USE_IMGUI
}

namespace Bullet
{
	namespace Impl
	{
		OilBullet::~OilBullet()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void OilBullet::Uninit()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void OilBullet::Update( float elapsedTime )
		{
			aliveTime++;

			if ( shouldStay )
			{
				velocity = 0.0f;
			}
			else
			{
				velocity.y -= ParamBullet::Get().Data().oil.gravity;
				orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), velocity.Unit() );
			}

			if ( !pEffect && element.Has( Element::Type::Flame ) )
			{
				pEffect = std::make_shared<EffectHandle>
				(
					EffectHandle::Generate( EffectAttribute::Flame, pos )
				);
			}
			if ( pEffect )
			{
				pEffect->SetPosition( pos );
			}
		}
		void OilBullet::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
		{
			if ( shouldStay ) { return; }
			// else

			const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
			const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );
			if ( raycastResult.raycastResult.wasHit || aabbResult.wasHit )
			{
				shouldStay = true;
			}

			velocity = aabbResult.correctedVector;

			pos += velocity;
		}
		void OilBullet::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
		{
			BulletBase::Draw( pRenderer, ParamBullet::Get().Data().oil.color.Product( color ) );
		}
		void OilBullet::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, ParamBullet::Get().Data().oil.color.Product( color ) );
		}
		void OilBullet::AttachSelfKind()
		{
			kind	= Kind::Oil;
			element	= Element::Type::Oil;
		}
		bool OilBullet::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().oil.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::AABB OilBullet::GetHitBoxAABB() const
		{
			Donya::AABB tmp = ParamBullet::Get().Data().oil.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 OilBullet::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().oil;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}


		void SmokeBase::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
		{
			//Smoke is not collision to anything.
			pos += velocity;
		}
		void SmokeBase::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &argColor )
		{
			BulletBase::Draw( pRenderer, color.Product( argColor ) );
		}
		void SmokeBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, ParamBullet::Get().Data().smoke.flame.general.color.Product( color ) );
		}
		
		void FlameSmoke::Update( float elapsedTime )
		{
			aliveTime++;
			color = ParamBullet::Get().Data().smoke.flame.general.color;
		}
		void FlameSmoke::AttachSelfKind()
		{
			kind	= Kind::FlameSmoke;
			element	= Element::Type::Flame;
		}
		bool FlameSmoke::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().smoke.flame.general.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::Sphere FlameSmoke::GetHitBoxSphere() const
		{
			Donya::Sphere tmp = ParamBullet::Get().Data().smoke.flame.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 FlameSmoke::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().smoke.flame.general;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}

		void IceSmoke::Update( float elapsedTime )
		{
			aliveTime++;
			color = ParamBullet::Get().Data().smoke.ice.general.color;
		}
		void IceSmoke::AttachSelfKind()
		{
			kind	= Kind::IceSmoke;
			element	= Element::Type::Ice;
		}
		bool IceSmoke::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().smoke.ice.general.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::Sphere IceSmoke::GetHitBoxSphere() const
		{
			Donya::Sphere tmp = ParamBullet::Get().Data().smoke.ice.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 IceSmoke::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().smoke.ice.general;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}


		// HACK: The Arrow's behavior almost same as OilBullet.
		Arrow::~Arrow()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Arrow::Uninit()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Arrow::Update( float elapsedTime )
		{
			aliveTime++;

			if ( shouldStay )
			{
				velocity = 0.0f;
			}
			else
			{
				velocity.y -= ParamBullet::Get().Data().arrow.gravity;
				orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), velocity.Unit() );
			}

			if ( !pEffect && element.Has( Element::Type::Flame ) )
			{
				pEffect = std::make_shared<EffectHandle>
				(
					EffectHandle::Generate( EffectAttribute::Flame, pos )
				);
			}
			if ( pEffect )
			{
				pEffect->SetPosition( pos );
			}
		}
		void Arrow::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
		{
			if ( shouldStay ) { return; }
			// else

			const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
			const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );
			if ( raycastResult.raycastResult.wasHit || aabbResult.wasHit )
			{
				shouldStay = true;
			}

			velocity = aabbResult.correctedVector;

			pos += velocity;
		}
		void Arrow::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
		{
			BulletBase::Draw( pRenderer, ParamBullet::Get().Data().arrow.color.Product( color ) );
		}
		void Arrow::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, ParamBullet::Get().Data().arrow.color.Product( color ) );
		}
		void Arrow::AttachSelfKind()
		{
			kind	= Kind::Arrow;
			element	= Element::Type::Nil;
		}
		bool Arrow::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().arrow.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::AABB Arrow::GetHitBoxAABB() const
		{
			Donya::AABB tmp = ParamBullet::Get().Data().arrow.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 Arrow::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().arrow;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}


		Breath::~Breath()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Breath::Uninit()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Breath::Update( float elapsedTime )
		{
			aliveTime++;

			if ( !pEffect && element.Has( Element::Type::Flame ) )
			{
				pEffect = std::make_shared<EffectHandle>
				(
					EffectHandle::Generate( EffectAttribute::Flame, pos )
				);
			}
			if ( pEffect )
			{
				pEffect->SetPosition( pos );
			}
		}
		void Breath::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
		{
			const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
			const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );
			if ( raycastResult.raycastResult.wasHit || aabbResult.wasHit )
			{
				wasHitToObject = true;
				GenerateBurning();
			}

			velocity = aabbResult.correctedVector;
			pos += velocity;
		}
		void Breath::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
		{
			BulletBase::Draw( pRenderer, ParamBullet::Get().Data().breath.color.Product( color ) );
		}
		void Breath::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, ParamBullet::Get().Data().breath.color.Product( color ) );
		}
		void Breath::AttachSelfKind()
		{
			kind	= Kind::Breath;
			element	= Element::Type::Nil;
		}
		void Breath::GenerateBurning() const
		{
			BulletAdmin::FireDesc desc{};
			desc.kind			= Kind::Burning;
			desc.addElement		= Element::Type::Nil;
			desc.speed			= 0.0f;
			desc.direction		= Donya::Vector3::Zero();
			desc.generatePos	= GetPosition();
			BulletAdmin::Get().Append( desc );
		}
		bool Breath::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().breath.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::AABB Breath::GetHitBoxAABB() const
		{
			Donya::AABB tmp = ParamBullet::Get().Data().breath.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 Breath::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().breath;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}

		Burning::~Burning()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Burning::Uninit()
		{
			if ( pEffect )
			{
				pEffect->Stop();
				pEffect.reset();
			}
		}
		void Burning::Update( float elapsedTime )
		{
			aliveTime++;

			if ( !pEffect && element.Has( Element::Type::Flame ) )
			{
				pEffect = std::make_shared<EffectHandle>
				(
					EffectHandle::Generate( EffectAttribute::Flame, pos )
				);
			}
			if ( pEffect )
			{
				pEffect->SetPosition( pos );
			}
		}
		void Burning::PhysicUpdate( const std::vector<Donya::AABB> &solids, const Donya::Model::PolygonGroup *pTerrain, const Donya::Vector4x4 *pTerrainMatrix )
		{
			if ( velocity.IsZero() ) { return; }
			// else

			const auto raycastResult	= CalcCorrectedVector( RECURSION_RAY_COUNT, velocity, pTerrain, pTerrainMatrix );
			const auto aabbResult		= CalcCorrectedVector( raycastResult.correctedVector, solids );
			
			velocity = aabbResult.correctedVector;
			pos += velocity;
		}
		void Burning::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
		{
			BulletBase::Draw( pRenderer, ParamBullet::Get().Data().burning.color.Product( color ) );
		}
		void Burning::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, ParamBullet::Get().Data().burning.color.Product( color ) );
		}
		void Burning::AttachSelfKind()
		{
			kind	= Kind::Burning;
			element	= Element::Type::Nil;
		}
		bool Burning::ShouldRemove() const
		{
			return ( ParamBullet::Get().Data().burning.aliveFrame <= aliveTime || wasHitToObject ) ? true : false;
		}
		Donya::AABB Burning::GetHitBoxAABB() const
		{
			Donya::AABB tmp = ParamBullet::Get().Data().burning.hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 Burning::GetWorldMatrix() const
		{
			const auto data = ParamBullet::Get().Data().burning;
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.MakeRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}
	}
}
