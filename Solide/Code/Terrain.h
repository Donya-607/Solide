#pragma once

#include <memory>
#include <string>

#include "Donya/Constant.h"
#include "Donya/StaticMesh.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

class Terrain
{
private:
	std::shared_ptr<Donya::StaticMesh> pCollisionMesh;
	std::shared_ptr<Donya::StaticMesh> pDrawMesh;
	Donya::Vector3 scale{ 1.0f, 1.0f, 1.0f };
	Donya::Vector3 translation;
	Donya::Vector4x4 matWorld;
public:
	Terrain( const std::string &drawMeshName, const std::string &collisionMeshName );
	DELETE_COPY_AND_ASSIGN( Terrain );
public:
	void SetWorldConfig( const Donya::Vector3 &scaling, const Donya::Vector3 &translate );
	void BuildWorldMatrix();
	Donya::Vector4x4 GetWorldMatrix() const { return matWorld; }
	std::shared_ptr<Donya::StaticMesh> GetCollisionMesh() const { return pCollisionMesh; }
public:
	void Draw( const Donya::Vector4 &eyePos, float transNear, float transFar, float transLowerAlpha, const Donya::Vector4x4 &matVP, const Donya::Vector4 &lightDir, const Donya::Vector4 &color );
public:
#if USE_IMGUI
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
