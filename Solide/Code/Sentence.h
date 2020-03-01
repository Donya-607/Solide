#pragma once

#include <memory>

#include "UI.h"

class TitleSentence
{
private:
	class FlusherBase
	{
	public:
		virtual void Init( TitleSentence &target );
		virtual void Update( TitleSentence &target, float elapsedTime ) = 0;
	protected:
		void UpdateImpl( TitleSentence &target, float elapsedTime, float flushInterval, float flushRange );
	};
	class LateFlusher : public FlusherBase
	{
	public:
		void Update( TitleSentence &target, float elapsedTime ) override;
	};
	class FastFlusher : public FlusherBase
	{
	public:
		void Update( TitleSentence &target, float elapsedTime ) override;
	};
private:
	float	flushTimer		= 0.0f;
	float	drawAlpha		= 1.0f;
	std::unique_ptr<FlusherBase> pFlusher{};
private: // Serialize members.
	UIObject uiLogo;
	UIObject uiPrompt;
	float lowestAlpha		= 0.0f;
	float flushIntervalLate	= 0.5f;	 // Seconds.
	float flushIntervalFast	= 0.25f; // Seconds.
	float flushRangeLate	= 0.0f;
	float flushRangeFast	= 0.0f;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( uiLogo ),
			CEREAL_NVP( uiPrompt ),
			CEREAL_NVP( lowestAlpha ),
			CEREAL_NVP( flushIntervalLate ),
			CEREAL_NVP( flushIntervalFast )
		);

		if ( 1 <= version )
		{
			archive
			(
				CEREAL_NVP( flushRangeLate ),
				CEREAL_NVP( flushRangeFast )
			);
		}
		if ( 2 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "TitleSentence";
public:
	/// <summary>
	/// The loaded sprite will not changes.
	/// </summary>
	void Init();
	bool LoadSprites( const std::wstring &logoFileName, const std::wstring &promptFileName );

	void Update( float elapsedTime );

	void Draw( float elapsedTime ) const;
public:
	void AdvanceState();
private:
	template<class Flusher>
	void ResetFlusher()
	{
		pFlusher = std::make_unique<Flusher>();
		pFlusher->Init( *this );
	}
private:
	void LoadBin ();
	void LoadJson();
#if USE_IMGUI
	void SaveBin ();
	void SaveJson();
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( TitleSentence, 1 )


class TutorialSentence
{
public:
	struct WhatEasing
	{
		int		easeKind = 0;
		int		easeType = 0;
		float	easeSeconds = 1.0f;
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize( Archive &archive, std::uint32_t version )
		{
			archive
			(
				CEREAL_NVP( easeKind ),
				CEREAL_NVP( easeType ),
				CEREAL_NVP( easeSeconds )
			);

			if ( 1 <= version )
			{
				// archive( CEREAL_NVP( x ) );
			}
		}
	};
private:
	class PerformerBase
	{
	public:
		void Init( TutorialSentence &target ) const;
		virtual void Update( TutorialSentence &target, float elapsedTime ) = 0;
		float CalcEasing( TutorialSentence &target, const WhatEasing &param ) const;
	};
	class AppearPerformer : public PerformerBase
	{
	public:
		void Update( TutorialSentence &target, float elapsedTime ) override;
	};
	class SlidePerformer : public PerformerBase
	{
	public:
		void Update( TutorialSentence &target, float elapsedTime ) override;
	};
private:
	float			easingTimer	= 0.0f; // 0.0f ~ 1.0f
	float			drawScale	= 0.0f;
	Donya::Vector2	drawPos;
	std::unique_ptr<PerformerBase> pPerformer;
private: // Serialize members.
	UIObject uiTutorial;

	WhatEasing		appearEasing;
	float			appearScale = 1.0f;
	Donya::Vector2	appearPos;
	WhatEasing		slideEasing;
	float			slideScale  = 1.0f;
	Donya::Vector2	slidePos;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( uiTutorial ),
			CEREAL_NVP( appearEasing ),
			CEREAL_NVP( appearScale ),
			CEREAL_NVP( appearPos ),
			CEREAL_NVP( slideEasing ),
			CEREAL_NVP( slideScale ),
			CEREAL_NVP( slidePos )
		);

		if ( 0 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "TutorialSentence";
public:
	void Init();
	bool LoadSprite( const std::wstring &tutorialFileName );

	void Update( float elapsedTime );

	void Draw( float elapsedTime ) const;
public:
	void Appear();
	void StartSliding();
private:
	template<class Performer>
	void ResetPerformer()
	{
		pPerformer = std::make_unique<Performer>();
		pPerformer->Init( *this );
	}
private:
	void LoadBin();
	void LoadJson();
#if USE_IMGUI
	void SaveBin();
	void SaveJson();
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( TutorialSentence, 0 )
CEREAL_CLASS_VERSION( TutorialSentence::WhatEasing, 0 )


class ClearSentence
{
private:
	float	easingTimer = 0.0f; // 0.0f ~ 1.0f
	float	drawScale   = 0.0f;
	bool	nowHidden	= true;
private: // Serialize members.
	UIObject uiClear;
	int		easeKind = 0;
	int		easeType = 0;
	float	easeSeconds = 1.0f;
	float	scalingSize = 1.0f;
private:
	friend class cereal::access;
	template<class Archive>
	void serialize( Archive &archive, std::uint32_t version )
	{
		archive
		(
			CEREAL_NVP( uiClear ),
			CEREAL_NVP( easeKind ),
			CEREAL_NVP( easeType ),
			CEREAL_NVP( easeSeconds ),
			CEREAL_NVP( scalingSize )
		);

		if ( 0 <= version )
		{
			// archive( CEREAL_NVP( x ) );
		}
	}
	static constexpr const char *ID = "ClearSentence";
public:
	void Init();
	bool LoadSprite( const std::wstring &clearFileName );

	void Update( float elapsedTime );

	void Draw( float elapsedTime ) const;
public:
	/// <summary>
	/// Will reset a performance state.
	/// </summary>
	void Hide();
	/// <summary>
	/// Will start a performance.
	/// </summary>
	void Appear();
private:
	void LoadBin();
	void LoadJson();
#if USE_IMGUI
	void SaveBin();
	void SaveJson();
public:
	void ShowImGuiNode( const std::string &nodeCaption );
#endif // USE_IMGUI
};
CEREAL_CLASS_VERSION( ClearSentence, 0 )
