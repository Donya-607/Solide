#pragma once

#include <memory>
#include <string>

#include "Donya/Constant.h"
#include "Donya/Model.h"
#include "Donya/ModelPose.h"
#include "Donya/ModelPolygon.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"

class Terrain
{
private:
	Donya::Vector3		scale;
	Donya::Vector3		translation;
	Donya::Vector4x4	matWorld;
	std::shared_ptr<Donya::Model::StaticModel>	pDrawModel;
	std::shared_ptr<Donya::Model::Pose>			pPose;
	std::shared_ptr<Donya::Model::PolygonGroup>	pPolygons;
#if DEBUG_MODE
	std::shared_ptr<Donya::Model::PolygonGroup>	pDrawingPolygons;
	std::shared_ptr<Donya::Model::StaticModel>	pCollisionModel;
	std::shared_ptr<Donya::Model::Pose>			pCollisionPose;
#endif // DEBUG_MODE
public:
	Terrain( int stageNumber );
public:
	void SetWorldConfig( const Donya::Vector3 &scaling, const Donya::Vector3 &translate );
	void BuildWorldMatrix();
public:
	const Donya::Vector4x4 &GetWorldMatrix() const { return matWorld; }
	std::shared_ptr<Donya::Model::PolygonGroup> GetCollisionModel() const { return pPolygons; }
#if DEBUG_MODE
	std::shared_ptr<Donya::Model::PolygonGroup> GetDrawModel() const { return pDrawingPolygons; }
#endif // DEBUG_MODE
public:
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};


#include "Donya/Serializer.h"
class TerrainDrawStates
{
public:
	struct Constant
	{
		Donya::Vector2	interval{ 1.0f, 1.0f };
		float			darkenAlpha = 0.95f; // 0.0f ~ 1.0f
		float			_padding{};
	};
private:
	Donya::CBuffer<Constant>	cbuffer;
	Donya::VertexShader			VS;
	Donya::PixelShader			PS;
public:
	bool CreateStates();
	void Update( const Constant &constant );
	void ActivateConstant();
	void ActivateShader();
	void DeactivateConstant();
	void DeactivateShader();
private:
	bool CreateShader();
};
template<class Archive>
void serialize( Archive &archive, TerrainDrawStates::Constant &constant, std::uint32_t version )
{
	archive
	(
		cereal::make_nvp( "interval",		constant.interval		),
		cereal::make_nvp( "darkenAlpha",	constant.darkenAlpha	)
	);

	if ( 1 <= version )
	{
		// archive( CEREAL_NVP( x ) );
	}
}
CEREAL_CLASS_VERSION( TerrainDrawStates::Constant, 0 )
