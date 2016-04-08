#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class UnityHapPlayerPluginApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void UnityHapPlayerPluginApp::setup()
{
}

void UnityHapPlayerPluginApp::mouseDown( MouseEvent event )
{
}

void UnityHapPlayerPluginApp::update()
{
}

void UnityHapPlayerPluginApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( UnityHapPlayerPluginApp, RendererGl )
