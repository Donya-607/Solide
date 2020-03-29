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
	std::shared_ptr<Donya::Model::StaticModel>	pCollisionModel;
	std::shared_ptr<Donya::Model::Pose>			pCollisionPose;
#endif // DEBUG_MODE
public:
	Terrain( const std::string &drawModelName, const std::string &collisionModelName );
public:
	void SetWorldConfig( const Donya::Vector3 &scaling, const Donya::Vector3 &translate );
	void BuildWorldMatrix();
public:
	const Donya::Vector4x4 &GetWorldMatrix() const { return matWorld; }
	std::shared_ptr<Donya::Model::PolygonGroup> GetCollisionModel() const { return pPolygons; }
public:
	void Draw( RenderingHelper *pRenderer, const Donya::Vector4 &color );
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
