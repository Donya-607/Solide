#include "Bullet.h"

#include <algorithm>
#include <array>

#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/Useful.h"

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
			"Oil"
		};

		struct StorageBundle
		{
			Donya::Model::StaticModel	model;
			Donya::Model::Pose			pose;
		};
		static std::array<std::shared_ptr<StorageBundle>, KIND_COUNT> models{};

		bool LoadModels()
		{
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
		case Kind::Oil:	return std::make_shared<Impl::OilBullet>();
		default: _ASSERT_EXPR( 0, L"Error : Unexpected bullet kind!" );	break;
		}
		return nullptr;
	}

	struct OilMember
	{
		float		gravity		= 1.0f;	// Absolute value.
		float		drawScale	= 1.0f;
		Donya::AABB	hitBox;
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
				// archive( CEREAL_NVP( x ) );
			}
		}
	public:
	#if USE_IMGUI
		void ShowImGuiNode( const std::string &nodeCaption )
		{
			if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
			// else

			ImGui::DragFloat( u8"重力", &gravity, 0.01f, 0.0f );
			ImGui::DragFloat( u8"描画スケール", &drawScale, 0.01f, 0.0f );

			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::OilMember, 1 )

class ParamOilBullet : public ParameterBase<ParamOilBullet>
{
public:
	static constexpr const char *ID = "OilBullet";
private:
	Bullet::OilMember m;
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
	Bullet::OilMember Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( ImGui::TreeNode( u8"弾・オイル" ) )
		{
			m.ShowImGuiNode( u8"調整" );
			ShowIONode( m );

			ImGui::TreePop();
		}

		ImGui::End();
	}
#endif // USE_IMGUI
};


namespace
{
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

		ParamOilBullet::Get().Init();

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
		ParamOilBullet::Get().UseImGui();
	}

	void BulletAdmin::FireDesc::ShowImGuiNode( const std::string &nodeCaption, bool generatePosIsRelative )
	{
		if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
		// else

		int intKind = scast<int>( kind );
		ImGui::SliderInt( u8"種類の変更", &intKind, 0, KIND_COUNT - 1 );
		kind = scast<Kind>( intKind );
		ImGui::Text( u8"いま：%s", GetKindName( kind ).c_str() );

		ImGui::DragFloat( u8"速度", &speed ); speed = std::max( 0.0f, speed );
		ImGui::SliderFloat3( u8"方向", &direction.x, -1.0f, 1.0f );
		if ( ImGui::Button( u8"方向を正規化" ) ) { direction.Normalize(); }

		std::string genPosCaption{ u8"生成位置" };
		genPosCaption += ( generatePosIsRelative ) ? u8"（相対）" : u8"（絶対）";
		ImGui::DragFloat3( genPosCaption.c_str(), &generatePos.x );


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
		bulletPtrs.erase( result, bulletPtrs.end() );
	}
	void BulletAdmin::PhysicUpdate()
	{
		for ( auto &pIt : bulletPtrs )
		{
			if ( !pIt ) { continue; }
			// else

			pIt->PhysicUpdate();
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


	void BulletBase::Init( const BulletAdmin::FireDesc &param )
	{
		AttachSelfKind();
		pos			= param.generatePos;
		velocity	= param.direction * param.speed;
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), param.direction );
	}
	void BulletBase::Update( float elapsedTime )
	{
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), velocity.Unit() );
	}
	void BulletBase::PhysicUpdate()
	{
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

		const auto &body = GetHitBox();
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
	Donya::Vector4x4 BulletBase::GetWorldMatrix() const
	{
		Donya::Vector4x4 world = orientation.RequireRotationMatrix();
		world._41 = pos.x;
		world._42 = pos.y;
		world._43 = pos.z;
		return world;
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
		void OilBullet::Update( float elapsedTime )
		{
			velocity.y -= ParamOilBullet::Get().Data().gravity;
			orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), velocity.Unit() );
		}
		void OilBullet::PhysicUpdate()
		{
			BulletBase::PhysicUpdate();
		}
		void OilBullet::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
		{
			BulletBase::DrawHitBox( pRenderer, VP, { 0.2f, 0.2f, 0.2f, 0.7f } );
		}
		void OilBullet::AttachSelfKind() { kind = Kind::Oil; }
		bool OilBullet::ShouldRemove() const
		{
		#if DEBUG_MODE
			return !( rand() % 512 );
		#endif // DEBUG_MODE
		}
		Donya::AABB OilBullet::GetHitBox() const
		{
			Donya::AABB tmp = ParamOilBullet::Get().Data().hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
		Donya::Vector4x4 OilBullet::GetWorldMatrix() const
		{
			const auto data = ParamOilBullet::Get().Data();
			Donya::Vector4x4 world{};
			world._11 = data.drawScale;
			world._22 = data.drawScale;
			world._33 = data.drawScale;
			world *= orientation.RequireRotationMatrix();
			world._41 = pos.x;
			world._42 = pos.y;
			world._43 = pos.z;
			return world;
		}
	}
}
