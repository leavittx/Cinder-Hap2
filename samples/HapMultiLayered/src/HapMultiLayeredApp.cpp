#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Resources.h"
#include "MovieHap.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class HapMultiLayeredApp : public App {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();

	qtime::MovieGlHapRef	mMovieBg, mMovieFront;
};

void HapMultiLayeredApp::setup()
{
	setFrameRate( 60 );
	setFpsSampleInterval( 0.25 );

	mMovieBg = qtime::MovieGlHap::create( loadAsset( "SampleHapQ.mov" ) );
	mMovieFront = qtime::MovieGlHap::create( loadAsset( "SampleHapAlpha.mov" ) );

	mMovieBg->setLoop();
	mMovieBg->play();

	mMovieFront->setLoop();
	mMovieFront->play();

	gl::enableAlphaBlending();
}

void HapMultiLayeredApp::mouseDown( MouseEvent event )
{
}

void HapMultiLayeredApp::update()
{
}

void HapMultiLayeredApp::draw()
{
	gl::clear( Color::black() );
	gl::viewport( toPixels( getWindowSize() ) );

	if( mMovieBg ) {
		mMovieBg->draw();
	}

	if( mMovieFront ) {
		mMovieFront->draw();
	}
}

CINDER_APP( HapMultiLayeredApp, RendererGl )
