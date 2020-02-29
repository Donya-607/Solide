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
		void UpdateImpl( TitleSentence &target, float elapsedTime, float flushInterval );
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
CEREAL_CLASS_VERSION( TitleSentence, 0 )
