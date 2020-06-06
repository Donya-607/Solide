#pragma once

#include <vector>

#include "Donya/Collision.h"
#include "Donya/Serializer.h"
#include "Donya/UseImGui.h"
#include "Donya/Vector.h"

#include "Renderer.h"
#include "UI.h"

class Tutorial
{
private:
	bool nowActive		= false;
	bool shouldRemove	= false;
private: // Serialize members.
	float			drawScale = 1.0f;
	Donya::Vector2	ssRelatedPos;	// Center
	Donya::Vector2	texPartPos;		// Left-Top
	Donya::Vector2	texPartSize;	// Whole size
	Donya::AABB		wsHitBox;		// The "pos" works world space
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( drawScale		),
			CEREAL_NVP( ssRelatedPos	),
			CEREAL_NVP( texPartPos		),
			CEREAL_NVP( texPartSize		),
			CEREAL_NVP( wsHitBox		)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
public:
	void Init();
	void Uninit();

	void Update( float elapsedTime );

	void Draw( const UIObject &sprite, const Donya::Vector2 &ssBasePos );
	void DrawHitBox( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, float alpha );
public:
	void Start();
	void Remove();
public:
	bool				ShouldRemove()		const;
	bool				IsActive()			const;
	Donya::Vector3		GetPosition()		const;
	Donya::AABB			GetHitBox()			const;
	Donya::Vector4x4	CalcWorldMatrix()	const;
#if USE_IMGUI
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( Tutorial, 0 )


class TutorialContainer
{
private:
	int			stageNo = 0;
	UIObject	sprSentence;
private: // Serializer member.
	float					darkenAlpha = 0.5f;
	UIObject				sprFrame;
	std::vector<Tutorial>	instances;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( darkenAlpha	),
			CEREAL_NVP( sprFrame	),
			CEREAL_NVP( instances	)
		);

		if ( 1 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "Tutorial";
public:
	bool Init( int stageNo );
	void Uninit();

	void Update( float elapsedTime );

	void Draw();
	void DrawHitBoxes( RenderingHelper *pRenderer, const Donya::Vector4x4 &matVP, float alpha );
public:
	size_t			GetTutorialCount() const;
	bool			IsOutOfRange( size_t tutorialIndex ) const;
	const Tutorial	*GetTutorialPtrOrNullptr( size_t tutorialIndex ) const;
private:
	void LoadBin( int stageNo );
	void LoadJson( int stageNo );
#if USE_IMGUI
	void SaveBin( int stageNo );
	void SaveJson( int stageNo );
public:
	void ShowImGuiNode( const std::string &nodeCaption, int stageNo );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( TutorialContainer, 0 )
