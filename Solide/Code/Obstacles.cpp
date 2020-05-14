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
#include "Renderer.h"

namespace
{
	enum Kind
	{
		Stone,
		Log,
		Tree,
		Table,
		Spray,
		Water,
		Hardened,

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
		"Spray",
		"Water",
		"Hardened",
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

		Donya::Loader loader{};
		auto Load = [&loader]( const std::string &filePath, ModelData *pDest )->bool
		{
			loader.ClearData();

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

			models[i] = std::make_shared<ModelData>();
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
	std::shared_ptr<ModelData> GetModelPtr( Kind kind )
	{
		if ( IsOutOfRange( kind ) )
		{
			_ASSERT_EXPR( 0, L"Error : Passed argument outs of range!" );
			return nullptr;
		}
		// else

		return models[kind];
	}

	void DrawModel( Kind kind, RenderingHelper *pRenderer, const Donya::Vector4x4 &W, const Donya::Vector4 &color )
	{
		if ( !pRenderer ) { return; }
		// else

		const auto pModel = GetModelPtr( kind );
		if ( !pModel ) { return; }
		// else

		Donya::Model::Constants::PerModel::Common constant{};
		constant.drawColor		= color;
		constant.worldMatrix	= W;
		pRenderer->UpdateConstant( constant );
		pRenderer->ActivateConstantModel();

		pRenderer->Render( pModel->model, pModel->pose );

		pRenderer->DeactivateConstantModel();
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
		case Stone:		*pOutput = std::make_shared<::Stone>();			return;
		case Log:		*pOutput = std::make_shared<::Log>();			return;
		case Tree:		*pOutput = std::make_shared<::Tree>();			return;
		case Table:		*pOutput = std::make_shared<::Table>();			return;
		case Spray:		*pOutput = std::make_shared<::Spray>();			return;
		case Water:		*pOutput = std::make_shared<::Water>();			return;
		case Hardened:	*pOutput = std::make_shared<::Hardened>();		return;
		default: _ASSERT_EXPR( 0, L"Error : Unexpected model kind!" );	return;
		}
	}

	struct Member
	{
		std::vector<Donya::AABB> collisions;

		struct
		{
			float submergeAmount	= 0.0f;
			float floatAmount		= 0.0f;
		} hardened;
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
				archive
				(
					CEREAL_NVP( hardened.submergeAmount	),
					CEREAL_NVP( hardened.floatAmount	)
				);
			}
			if ( 2 <= version )
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
CEREAL_CLASS_VERSION( Member, 1 )

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
					Kind kind = scast<Kind>( i );
					caption = GetModelName( kind );
					if ( kind == Kind::Water )
					{
						ImGui::TextDisabled( caption.c_str() );
					}
					else
					{
						ParameterHelper::ShowAABBNode( caption, &data[i] );
					}
				}

				ImGui::TreePop();
			}

			if ( ImGui::TreeNode( u8"固まった足場のパラメータ" ) )
			{
				ImGui::DragFloat( u8"初めに沈む量",		&m.hardened.submergeAmount,	0.01f );
				ImGui::DragFloat( u8"１フレームに浮く量",	&m.hardened.floatAmount,	0.01f );
				m.hardened.submergeAmount	= std::max( 0.0f, m.hardened.submergeAmount	);
				m.hardened.floatAmount		= std::max( 0.0f, m.hardened.floatAmount	);

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
bool ObstacleBase::IsWaterKind( int obstacleKind )
{
	return ( scast<Kind>( obstacleKind ) == Kind::Water );
}
bool ObstacleBase::IsHardenedKind( int obstacleKind )
{
	return ( scast<Kind>( obstacleKind ) == Kind::Hardened );
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

void ObstacleBase::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	if ( !Common::IsShowCollision() ) { return; }
	// else

#if DEBUG_MODE
	Solid::DrawHitBox( pRenderer, matVP, color );
#endif // DEBUG_MODE
}
bool ObstacleBase::ShouldRemove() const
{
#if USE_IMGUI
	if ( wantRemoveByGui ) { return true; }
#endif // USE_IMGUIs
	return false;
}
#if USE_IMGUI
void ObstacleBase::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
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
	
	ImGui::DragFloat3( u8"ワールド座標", &pos.x, 0.1f );
	ParameterHelper::ShowAABBNode( u8"当たり判定", &hitBox );

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI

void Stone::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Stone, ParamObstacle::Get().Data() );
}
void Stone::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Stone, pRenderer, GetWorldMatrix(), color );
}
void Stone::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, { 0.3f, 0.3f, 0.3f, 0.5f } );
}
int Stone::GetKind() const
{
	return scast<int>( Kind::Stone );
}

void Log::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Log, ParamObstacle::Get().Data() );
}
void Log::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Log, pRenderer, GetWorldMatrix(), color );
}
void Log::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, { 0.5f, 0.4f, 0.1f, 0.5f } );
}
int Log::GetKind() const
{
	return scast<int>( Kind::Log );
}

void Tree::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Tree, ParamObstacle::Get().Data() );
}
void Tree::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Tree, pRenderer, GetWorldMatrix(), color );
}
void Tree::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, { 0.4f, 0.5f, 0.0f, 0.5f } );
}
int Tree::GetKind() const
{
	return scast<int>( Kind::Tree );
}

void Table::Update( float elapsedTime )
{
	hitBox = GetModelHitBox( Kind::Table, ParamObstacle::Get().Data() );
}
void Table::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Table, pRenderer, GetWorldMatrix(), color );
}
void Table::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, { 0.4f, 0.5f, 0.0f, 0.5f } );
}
int Table::GetKind() const
{
	return scast<int>( Kind::Table );
}


void Spray::Update( float elapsedTime )
{
	UpdateHitBox();

	UpdateShot( elapsedTime );
}
void Spray::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Spray, pRenderer, GetWorldMatrix(), color );
}
void Spray::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, { 0.4f, 0.5f, 0.0f, 0.5f } );
}
int  Spray::GetKind() const
{
	return scast<int>( Kind::Spray );
}
void Spray::UpdateHitBox()
{
	hitBox = GetModelHitBox( Kind::Spray, ParamObstacle::Get().Data() );
}
void Spray::UpdateShot( float elapsedTime )
{
	if ( startupTimer <= startupFrame )
	{
		startupTimer++;
		nowSpraying = true;
		return;
	}
	// else


	shotTimer++;

	if ( nowSpraying )
	{
		UpdateSpray( elapsedTime );
	}
	else
	{
		UpdateCooldown( elapsedTime );
	}

	if ( ShouldChangeMode() )
	{
		shotTimer	= 0;
		nowSpraying	= !nowSpraying;
	}
}
void Spray::UpdateSpray( float elapsedTime )
{
	const bool generateTiming = ( shotGenInterval < 2 ) ? true : ( shotTimer % shotGenInterval ) == 1;
	if ( generateTiming )
	{
		GenerateShot();
	}
}
void Spray::UpdateCooldown( float elapsedTime )
{
	// No op.
}
void Spray::GenerateShot()
{
	auto desc = shotDesc;
	// The "direction", and "generatePos" are local space.
	desc.direction		=  orientation.RotateVector( desc.direction   );
	desc.generatePos	=  orientation.RotateVector( desc.generatePos );
	desc.generatePos	+= GetPosition();

	Bullet::BulletAdmin::Get().Append( desc );

}
bool Spray::ShouldChangeMode() const
{
	return	( nowSpraying )
			? ( sprayingFrame < shotTimer )
			: ( cooldownFrame < shotTimer );
}
#if USE_IMGUI
void Spray::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ObstacleBase::ShowImGuiNode( nodeCaption + u8"・基底部分", useTreeNode );

	if ( ImGui::TreeNode( std::string{ nodeCaption + u8"・拡張部分" }.c_str() ) )
	{
		auto Clamp = []( int *target, int min, int max )
		{
			*target = std::max( min, std::min( max, *target ) );
		};

		if ( ImGui::TreeNode( u8"調整パラメータ" ) )
		{
			if ( ImGui::TreeNode( u8"姿勢" ) )
			{
				static Donya::Vector3 degrees;
				ImGui::DragFloat3( u8"前方向（degree角）", &degrees.x );
				if ( ImGui::Button( u8"前方向を現在の姿勢に適用" ) )
				{
					Donya::Vector3 radians{};
					radians.x	= ToRadian( degrees.x );
					radians.y	= ToRadian( degrees.y );
					radians.z	= ToRadian( degrees.z );
					orientation	= Donya::Quaternion::Make( radians.x, radians.y, radians.z );
				}
				if ( ImGui::Button( u8"現在の姿勢から前方向を算出" ) )
				{
					Donya::Vector3 radians = orientation.GetEulerAngles();
					degrees.x = ToDegree( radians.x );
					degrees.y = ToDegree( radians.y );
					degrees.z = ToDegree( radians.z );
				}
				ImGui::TreePop();
			}

			ImGui::DragInt( u8"初回起動にかける時間（フレーム）",	&startupFrame		);
			ImGui::DragInt( u8"噴射時間（フレーム）",				&sprayingFrame		);
			ImGui::DragInt( u8"待機時間（フレーム）",				&cooldownFrame		);
			ImGui::DragInt( u8"噴射物生成間隔",					&shotGenInterval	);
			Clamp( &startupFrame,		0, startupFrame		);
			Clamp( &sprayingFrame,		0, sprayingFrame	);
			Clamp( &cooldownFrame,		0, cooldownFrame	);
			Clamp( &shotGenInterval,	0, shotGenInterval	);

			shotDesc.ShowImGuiNode( u8"噴射物" );

			ImGui::TreePop();
		}

		if ( ImGui::TreeNode( u8"現在の状態" ) )
		{
			ImGui::DragInt( u8"ショットタイマ",	&shotTimer		);
			ImGui::DragInt( u8"起動タイマ",		&startupTimer	);
			if ( ImGui::Button( u8"タイマ群をリセット" ) )
			{
				shotTimer		= 0;
				startupTimer	= 0;
			}
			ImGui::Checkbox( u8"今は噴射中か？",	&nowSpraying	);

			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI


void Water::Update( float elapsedTime )
{
	hitBox = hurtBox;
}
void Water::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Water, pRenderer, GetWorldMatrix(), color );
}
void Water::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, color.Product( { 0.0f, 0.0f, 1.0f, 1.0f } ) );
}
Donya::Vector4x4 Water::GetWorldMatrix() const
{
	Donya::Vector4x4 W{};
	W._11 = hurtBox.size.x;
	W._22 = hurtBox.size.y;
	W._33 = hurtBox.size.z;
	const auto pos = GetPosition();
	W._41 = pos.x + hurtBox.pos.x;
	W._42 = pos.y + hurtBox.pos.y;
	W._43 = pos.z + hurtBox.pos.z;
	return W;
}
int Water::GetKind() const
{
	return scast<int>( Kind::Water );
}
#if USE_IMGUI
void Water::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	ImGui::DragFloat3( u8"ワールド座標", &pos.x, 0.1f );
	ParameterHelper::ShowAABBNode( u8"当たり判定", &hurtBox );
	
	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI


void Hardened::Init( const Donya::Vector3 &wsInitialPos )
{
	ObstacleBase::Init( wsInitialPos );

	initialPos		= wsInitialPos;
	submergeAmount	= ParamObstacle::Get().Data().hardened.submergeAmount;
}
void Hardened::Update( float elapsedTime )
{
	const auto data = ParamObstacle::Get().Data();

	hitBox = GetModelHitBox( Kind::Hardened, data );

	submergeAmount -= data.hardened.floatAmount;
	submergeAmount =  std::max( 0.0f, submergeAmount );
	pos = initialPos;
	pos.y -= submergeAmount;
}
void Hardened::Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color )
{
	DrawModel( Kind::Hardened, pRenderer, GetWorldMatrix(), color );
}
void Hardened::DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, const Donya::Vector4 &color )
{
	ObstacleBase::DrawHitBox( pRenderer, matVP, color.Product( { 0.8f, 0.8f, 0.8f, 1.0f } ) );
}
int  Hardened::GetKind() const
{
	return scast<int>( Kind::Hardened );
}
#if USE_IMGUI
void Hardened::ShowImGuiNode( const std::string &nodeCaption, bool useTreeNode )
{
	if ( useTreeNode && !ImGui::TreeNode( nodeCaption.c_str() ) ) { return; }
	// else

	if ( ImGui::TreeNode( u8"調整部分" ) )
	{
		ObstacleBase::ShowImGuiNode( "", /* useTreeNode = */ false );
		ImGui::TreePop();
	}
	if ( ImGui::TreeNode( u8"現在の状態" ) )
	{
		ImGui::DragFloat3( u8"初期位置",		&initialPos.x,		0.01f );
		ImGui::DragFloat ( u8"沈んでいる量",	&submergeAmount,	0.01f );
		submergeAmount = std::max( 0.0f, submergeAmount );

		ImGui::TreePop();
	}

	if ( useTreeNode ) { ImGui::TreePop(); }
}
#endif // USE_IMGUI
