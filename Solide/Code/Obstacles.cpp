#include "Obstacles.h"

#include <memory>
#include <vector>

#include "Donya/Constant.h"
#include "Donya/Loader.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/Serializer.h"
#include "Donya/Useful.h"		// MultiByte char -> Wide char

#include "Common.h"
#include "FilePath.h"
#include "Parameter.h"

namespace
{
	enum Kind
	{
		Stone,
		Log,
		Tree,
		Table,

		Goal,

		KindCount
	};
	constexpr size_t KIND_COUNT = scast<size_t>( KindCount );
	constexpr const char *MODEL_DIRECTORY = "./Data/Models/Obstacle/";
	constexpr const char *EXTENSION = ".bin";
	constexpr std::array<const char *, KIND_COUNT> MODEL_NAMES
	{
		"Stone",
		"Log",
		"Tree",
		"Table",

		"Goal",
	};

	struct ModelData
	{
		Donya::Model::StaticModel	model;
		Donya::Model::Pose			pose;
	};
	static std::array<std::shared_ptr<ModelData>, KIND_COUNT> models{};

	bool LoadModels()
	{
		bool result		= true;
		bool succeeded	= true;

		auto Load = []( const std::string &filePath, ModelData *pDest )->bool
		{
			Donya::Loader loader{};

			bool result = loader.Load( filePath );
			if ( !result ) { return false; }
			// else

			const auto &source = loader.GetModelSource();
			pDest->model = Donya::Model::StaticModel::Create( source, loader.GetFileDirectory() );
			pDest->pose.AssignSkeletal( source.skeletal );

			return pDest->model.WasInitializeSucceeded();
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
		return ( kind < 0 || KindCount <= kind ) ? true : false;
	}
	std::string GetModelName( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return "";
		}
		// else

		return std::string{ MODEL_NAMES[kind] };
	}
	std::shared_ptr<Donya::StaticMesh> GetModelPtr( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return nullptr;
		}
		// else

		return models[kind];
	}

	void DrawModel( Kind kind, const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &W, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
	{
		const auto pModel = GetModelPtr( kind );
		if ( !pModel ) { return; }
		// else

		pModel->Render
		(
			eyePos, transNear, transFar, transLowerAlpha,
			nullptr,
			/* useDefaultShading	= */ true,
			/* isEnableFill			= */ true,
			W * VP, W,
			lightDir, color
		);
	}

	// HACK : This method using switch-case statement.
	void AssignModel( Kind kind, std::shared_ptr<ObstacleBase> *pOutput )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			pOutput->reset();
			return;
		}
		// else

		switch ( kind )
		{
		case Stone:	*pOutput = std::make_shared<::Stone>();				return;
		case Log:	*pOutput = std::make_shared<::Log>();				return;
		case Tree:	*pOutput = std::make_shared<::Tree>();				return;
		case Table:	*pOutput = std::make_shared<::Table>();				return;
		case Goal:	*pOutput = std::make_shared<::Goal>();				return;
		default: _ASSERT_EXPR( 0, L"Error : Unexpected model kind!" );	return;
		}
	}

	struct Member
	{
		std::vector<Donya::AABB> collisions;
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
	Donya::AABB GetModelHitBox( Kind kind, const Member &data )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return Donya::AABB::Nil();
		}
		// else

		return data.collisions[kind];
	}
}
CEREAL_CLASS_VERSION( Member, 0 )

class ParamObstacle : public ParameterBase<ParamObstacle>
{
public:
	static constexpr const char *ID = "Obstacle";
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
	}
	Member Data() const { return m; }
private:
	std::string GetSerializeIdentifier()			override { return ID; }
	std::string GetSerializePath( bool isBinary )	override { return GenerateSerializePath( ID, isBinary ); }
public:
#if USE_IMGUI
	void UseImGui() override
	{
		if ( !ImGui::BeginIfAllowed() ) { return; }
		// else

		if ( m.collisions.size() != KIND_COUNT )
		{
			m.collisions.resize( KIND_COUNT );
		}

		if ( ImGui::TreeNode( u8"障害物のパラメータ調整" ) )
		{
			if ( ImGui::TreeNode( u8"当たり判定" ) )
			{
				auto &data = m.collisions;

				std::string  caption{};
				const size_t count = data.size();
				for ( size_t i = 0; i < count; ++i )
				{
					caption = GetModelName( scast<Kind>( i ) );
					ParameterHelper::ShowAABBNode( caption, &data[i] );
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

bool ObstacleBase::LoadModels()
{
	return ::LoadModels();
}
void ObstacleBase::ParameterInit()
{
	ParamObstacle::Get().Init();
}
void ObstacleBase::ParameterUninit()
{
	ParamObstacle::Get().Uninit();
}

int  ObstacleBase::GetModelKindCount()
{
	return scast<int>( KIND_COUNT );
}
std::string ObstacleBase::GetModelName( int modelKind )
{
	return ::GetModelName( scast<Kind>( modelKind ) );
}
void ObstacleBase::AssignDerivedModel( int modelKind, std::shared_ptr<ObstacleBase> *pOutput )
{
	AssignModel( scast<Kind>( modelKind ), pOutput );
}

#if USE_IMGUI
void ObstacleBase::UseImGui()
{
	ParamObstacle::Get().UseImGui();
}
#endif // USE_IMGUI

#if USE_IMGUI
bool ObstacleBase::ShowImGuiNode( const std::string &nodeCaption )
{
	if ( !ImGui::TreeNode( nodeCaption.c_str() ) ) { return false; }
	// else

	bool wantRemove = false;
	if ( ImGui::Button( ( nodeCaption + u8"を削除" ).c_str() ) )
	{
		wantRemove = true;
	}
	
	ImGui::DragFloat3( u8"ワールド座標", &pos.x, 0.1f );
	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

	ImGui::TreePop();
	return wantRemove;
}
#endif // USE_IMGUI

void Stone::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Stone, ParamObstacle::Get().Data() );
}
void Stone::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Stone, eyePos, transNear, transFar, transLowerAlpha, GetWorldMatrix(), VP, lightDir, color );
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		DrawHitBox( VP, { 0.3f, 0.3f, 0.3f, 0.5f } );
	}
#endif // DEBUG_MODE
}
int Stone::GetKind() const
{
	return scast<int>( Kind::Stone );
}

void Log::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Log, ParamObstacle::Get().Data() );
}
void Log::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Log, eyePos, transNear, transFar, transLowerAlpha, GetWorldMatrix(), VP, lightDir, color );
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		DrawHitBox( VP, { 0.5f, 0.4f, 0.1f, 0.5f } );
	}
#endif // DEBUG_MODE
}
int Log::GetKind() const
{
	return scast<int>( Kind::Log );
}

void Tree::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Tree, ParamObstacle::Get().Data() );
}
void Tree::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Tree, eyePos, transNear, transFar, transLowerAlpha, GetWorldMatrix(), VP, lightDir, color );
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		DrawHitBox( VP, { 0.4f, 0.5f, 0.0f, 0.5f } );
	}
#endif // DEBUG_MODE
}
int Tree::GetKind() const
{
	return scast<int>( Kind::Tree );
}

void Table::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Table, ParamObstacle::Get().Data() );
}
void Table::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	DrawModel( Kind::Table, eyePos, transNear, transFar, transLowerAlpha, GetWorldMatrix(), VP, lightDir, color );
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		DrawHitBox( VP, { 0.4f, 0.5f, 0.0f, 0.5f } );
	}
#endif // DEBUG_MODE
}
int Table::GetKind() const
{
	return scast<int>( Kind::Table );
}

void Goal::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Goal, ParamObstacle::Get().Data() );

	constexpr float ROT_ANGLE = 1.5f; // ( 360 / 4 )
	const Donya::Quaternion rotation = Donya::Quaternion::Make( Donya::Vector3::Up(), ToRadian( ROT_ANGLE ) );
	orientation.RotateBy( rotation );
}
void Goal::Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &VP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color )
{
	const Donya::AABB body = GetHitBox();
	Donya::Vector4x4 W{};
	W._11 = body.size.x * 2.0f;
	W._22 = body.size.y * 2.0f;
	W._33 = body.size.z * 2.0f;
	W *= orientation.RequireRotationMatrix();
	W._41 = body.pos.x;
	W._42 = body.pos.y;
	W._43 = body.pos.z;

	DrawModel( Kind::Goal, eyePos, transNear, transFar, transLowerAlpha, W, VP, lightDir, color );
#if DEBUG_MODE
	if ( Common::IsShowCollision() )
	{
		DrawHitBox( VP, { 1.0f, 1.0f, 1.0f, 0.5f } );
	}
#endif // DEBUG_MODE
}
int Goal::GetKind() const
{
	return scast<int>( Kind::Goal );
}
