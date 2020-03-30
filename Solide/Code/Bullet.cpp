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
		constexpr const char *MODEL_DIRECTORY = "./Data/Models/Bullets/";
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

	bool LoadBulletsResource()
	{
		bool succeeded = true;

		if ( !LoadModels() ) { succeeded = false; }

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

	struct OilMember
	{
		float		gravity = 1.0f;
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
			ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

			ImGui::TreePop();
		}
	#endif // USE_IMGUI
	};
}
CEREAL_CLASS_VERSION( Bullet::OilMember, 0 )

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


namespace Bullet
{
#if USE_IMGUI
	void UseBulletsImGui()
	{
		ParamOilBullet::Get().UseImGui();
	}
#endif // USE_IMGUI


	void BulletAdmin::Init()
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


	void BulletBase::Init( const BulletAdmin::FireDesc &param )
	{
		AttachSelfKind();
		pos			= param.generatePos;
		velocity	= param.direction * param.speed;
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), param.direction );
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

		Donya::Model::Cube::Constant constant{};
		constant.matWorld		= GetWorldMatrix();
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

		const std::string kindName =
			( kind == Kind::KindCount )
			? "ERROR_KIND"
			: Bullet::MODEL_NAMES[scast<int>( kind )];
		ImGui::Text( u8"種類：%s", kindName.c_str() );

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
			velocity.y += ParamOilBullet::Get().Data().gravity;
		}
		void OilBullet::PhysicUpdate()
		{
			BulletBase::PhysicUpdate();
		}
		void OilBullet::AttachSelfKind() { kind = Kind::Oil; }
		Donya::AABB OilBullet::GetHitBox() const
		{
			Donya::AABB tmp = ParamOilBullet::Get().Data().hitBox;
			tmp.pos += GetPosition();
			return tmp;
		}
	}
}
