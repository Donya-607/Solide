#include "Bullet.h"

#include <array>

#include "Donya/Loader.h"
#include "Donya/StaticMesh.h"
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

		static std::array<std::shared_ptr<Donya::StaticMesh>, KIND_COUNT> models{};

		bool LoadModels()
		{
			bool result		= true;
			bool succeeded	= true;

			auto Load = []( const std::string &filePath, Donya::StaticMesh *pDest )->bool
			{
				bool result = true;
				Donya::Loader loader{};

				result = loader.Load( filePath, nullptr );
				if ( !result ) { return false; }
				// else

				result = Donya::StaticMesh::Create( loader, *pDest );
				return result;
			};

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

				models[i] = std::make_shared<Donya::StaticMesh>();
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
		std::shared_ptr<Donya::StaticMesh> GetModelPtr( Kind kind )
		{
			if ( IsOutOfRange( kind ) )
			{
				_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
				return nullptr;
			}
			// else

			return models[scast<int>( kind )];
		}
	}

	bool LoadBulletsResource()
	{
		bool result = true;
		bool succeeded = true;

		result = LoadModels();
		if ( !result ) { succeeded = false; }

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


namespace
{
	void DrawHitBox( const Donya::AABB &drawObj, const Donya::Vector4x4 &VP, const Donya::Vector4 &color )
	{
		Section hitBoxDrawer{ drawObj.pos, drawObj };
		hitBoxDrawer.DrawHitBox( VP, color );
	}
}


namespace Bullet
{
	void BulletBase::Init( const Donya::Vector3 &wsPos, float speed, const Donya::Vector3 &direction )
	{
		AttachSelfKind();
		pos			= wsPos;
		velocity	= direction * speed;
		orientation = Donya::Quaternion::LookAt( Donya::Vector3::Front(), direction );
	}
	void BulletBase::PhysicUpdate()
	{
		pos += velocity;
	}



}
