#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Timer.h"

#include "Resources.h"
#include "MovieHap.h"

#include "PerfTracker.h"

using namespace ci;
using namespace ci::app;
using namespace std;

template <typename T> string tostr(const T& t, int p) { ostringstream os; os<<std::setprecision(p)<<std::fixed<<t; return os.str(); }

class HapLoaderApp : public App {
public:
	void prepareSettings( Settings* settings ) override;
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void fileDrop( FileDropEvent event ) override;
	void update() override;
	void draw() override;
	
	void loadMovieFile( const fs::path &path );
	
	gl::TextureRef			mInfoTexture;
	qtime::MovieGlHapRef	mMovie;
	
	PerfTrackerRef			mPerfTracker;
};

void HapLoaderApp::prepareSettings( Settings* settings )
{
	settings->enableHighDensityDisplay();
}

void HapLoaderApp::setup()
{
	mPerfTracker = PerfTracker::create( Area(	0.15f * getWindowWidth(), 10,
												0.85f * getWindowWidth(), 200 ) );
	setFrameRate(60);
	setFpsSampleInterval(0.25);
}

void HapLoaderApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' ) {
		setFullScreen( ! isFullScreen() );
	}
	else if( event.getChar() == 'o' ) {
		fs::path moviePath = getOpenFilePath();
		if( ! moviePath.empty() )
			loadMovieFile( moviePath );
	}
	else if( event.getChar() == 'r' ) {
		mMovie.reset();
	}
}

void HapLoaderApp::loadMovieFile( const fs::path &moviePath )
{
	try {
		mMovie.reset();
		// load up the movie, set it to loop, and begin playing
		mMovie = qtime::MovieGlHap::create( moviePath );
		mMovie->setLoop();
		mMovie->play();
		
		// create a texture for showing some info about the movie
		TextLayout infoText;
		infoText.clear( ColorA( 0.2f, 0.2f, 0.2f, 0.5f ) );
		infoText.setColor( Color::white() );
		infoText.addCenteredLine( moviePath.filename().string() );
		infoText.addLine( toString( mMovie->getWidth() ) + " x " + toString( mMovie->getHeight() ) + " pixels" );
		infoText.addLine( toString( mMovie->getDuration() ) + " seconds" );
		infoText.addLine( toString( mMovie->getNumFrames() ) + " frames" );
		infoText.addLine( toString( mMovie->getFramerate() ) + " fps" );
		infoText.setBorder( 4, 2 );
		mInfoTexture = gl::Texture::create( infoText.render( true ) );
	}
		catch( ... ) {
			console() << "Unable to load the movie." << std::endl;
			mInfoTexture.reset();
	}
	
}

void HapLoaderApp::fileDrop( FileDropEvent event )
{
	loadMovieFile( event.getFile( 0 ) );
}

void HapLoaderApp::update()
{
}

void HapLoaderApp::draw()
{
	gl::clear( Color::black() );
	gl::enableAlphaBlending();
	gl::viewport( toPixels( getWindowSize() ) );
	
	// draw grid
	ivec2 sz = getWindowSize() / ivec2( 8, 6 );
	gl::color( Color::gray( 0.2f ) );
	for (int x = 0 ; x < 8 ; x++ )
		for (int y = (x%2?0:1) ; y < 6 ; y+=2 )
			gl::drawSolidRect( Rectf( 0.0f, 0.0f, sz.x, sz.y ) + sz * ivec2( x, y ) );
	
	
	mPerfTracker->startFrame();
	if ( mMovie ) {
		mMovie->draw();
	}
	mPerfTracker->endFrame();
	
	mPerfTracker->draw();
	
	// draw info
	if( mInfoTexture ) {
		gl::draw( mInfoTexture, ivec2( 20, getWindowHeight() - 20 - mInfoTexture->getHeight() ) );
	}
	
	// draw fps
	TextLayout infoFps;
	infoFps.clear( ColorA( 0.2f, 0.2f, 0.2f, 0.5f ) );
	infoFps.setColor( Color::white() );
	infoFps.addLine( "Movie Framerate: " + tostr( mMovie->getPlaybackFramerate(), 1 ) );
	infoFps.addLine( "App Framerate: " + tostr( this->getAverageFps(), 1 ) );
	infoFps.setBorder( 4, 2 );
	gl::draw( gl::Texture::create( infoFps.render( true ) ), ivec2( 20, 20 ) );
}

CINDER_APP( HapLoaderApp, RendererGl() );