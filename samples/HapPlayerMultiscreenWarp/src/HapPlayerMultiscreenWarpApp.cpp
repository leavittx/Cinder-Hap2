// TODO: 
// 1. savable per-warp uv settings
// 2. use fast mov reader (vux)
// 3. play sequence of dxt compressed files

#include "boost/optional.hpp"

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/Timer.h"
#include "cinder/Xml.h"

#include "Resources.h"

// Cinder blocks
#include "MovieHap.h"
#include "Warp.h"

#include "PerfTracker.h"
#include <cinder/Rand.h>

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using boost::optional;

template <typename T> string tostr(const T& t, int p) { ostringstream os; os << std::setprecision(p) << std::fixed << t; return os.str(); }

struct AppSettings
{
  ivec2 mWindowPos;
  ivec2 mWindowSize;
};

class HapPlayerMultiscreenWarpApp : public App
{
public:
  HapPlayerMultiscreenWarpApp();

  static void prepareSettings(Settings* settings);
  void setup() override;
  void cleanup() override;
  void update() override;
  void draw() override;

  void resize() override;

  void mouseMove(MouseEvent event) override;
  void mouseDown(MouseEvent event) override;
  void mouseDrag(MouseEvent event) override;
  void mouseUp(MouseEvent event) override;

  void keyDown(KeyEvent event) override;
  void keyUp(KeyEvent event) override;

  void fileDrop(FileDropEvent event) override;

private:
  void loadMovieFile(const fs::path &path);
  void drawMovie();

  static optional<AppSettings> readSettings(const ci::DataSourceRef &source);
  static void writeSettings(const AppSettings& appSettings, const ci::DataTargetRef &target);
  AppSettings getCurrentAppSettings() const;
  void applyAppSettings(const AppSettings& appSettings);
  void createPerspectiveWarp();

private:
  // Hap video playback
  gl::TextureRef mInfoTexture;
  qtime::MovieGlHapRef mMovie;

  // Performance tracker
  PerfTrackerRef mPerfTracker;
  bool mPerfTrackerVisible;

  // Warps
  bool mUseBeginEnd;
  fs::path mWarpSettingsPath;
  WarpList mWarps;
  gl::TextureRef mHelpImage;

  // Settings
  fs::path mAppSettingsPath;
};

HapPlayerMultiscreenWarpApp::HapPlayerMultiscreenWarpApp()
  : mPerfTrackerVisible(false)
  , mUseBeginEnd(false)
{
}

void HapPlayerMultiscreenWarpApp::prepareSettings(Settings* settings)
{
  settings->setHighDensityDisplayEnabled(true);
}

void HapPlayerMultiscreenWarpApp::setup()
{
  getWindow()->setTitle("Hap playback app by Lev Panov, 2016");
  getWindow()->setBorderless(false);

  mPerfTracker = PerfTracker::create(Area(0.15f * getWindowWidth(), 10,
                                          0.85f * getWindowWidth(), 200));
  setFrameRate(60);
  setFpsSampleInterval(0.25);
  //disableFrameRate();

  // Set app settings path
  mAppSettingsPath = getAssetPath("") / "app.xml";

  // Setup warps
  mUseBeginEnd = false;
  // Initialize warps
  mWarpSettingsPath = getAssetPath("") / "warps.xml";
  // Otherwise create a single warp from scratch
  createPerspectiveWarp();

  // Load help image
  try 
  {
    mHelpImage = gl::Texture::create(loadImage(loadAsset("help.png")),
      gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));

    //mSrcArea = mHelpImage->getBounds();

    // adjust the content size of the warps
    Warp::setSize(mWarps, mHelpImage->getSize());
  }
  catch (const exception &ex) 
  {
    console() << ex.what() << std::endl;
  }
}

void HapPlayerMultiscreenWarpApp::cleanup()
{
  //// Save warp settings
  //Warp::writeSettings(mWarps, writeFile(mWarpSettingsPath));
}

void HapPlayerMultiscreenWarpApp::update()
{
}

void HapPlayerMultiscreenWarpApp::draw()
{
  gl::clear(Color::black());
  gl::enableAlphaBlending();
  gl::viewport(toPixels(getWindowSize()));

  // draw grid
  ivec2 sz = getWindowSize() / ivec2(8, 6);
  gl::color(Color::gray(0.2f));
  for (int x = 0; x < 8; x++)
    for (int y = (x % 2 ? 0 : 1); y < 6; y += 2)
      gl::drawSolidRect(Rectf(0.0f, 0.0f, sz.x, sz.y) + sz * ivec2(x, y));

  mPerfTracker->startFrame();
  drawMovie();
  mPerfTracker->endFrame();

  // draw performance tracker
  if (mPerfTrackerVisible)
  {
    mPerfTracker->draw();
  }

  // draw info
  gl::color(1.0f, 1.0f, 1.0f);
  if (mInfoTexture) 
  {
    gl::draw(mInfoTexture, ivec2(20, getWindowHeight() - 20 - mInfoTexture->getHeight()));
  }

  // draw fps
  TextLayout infoFps;
  infoFps.clear(ColorA(0.2f, 0.2f, 0.2f, 0.5f));
  infoFps.setColor(Color::white());
  infoFps.addLine("Movie Framerate: " + tostr(mMovie->getPlaybackFramerate(), 1));
  infoFps.addLine("App Framerate: " + tostr(this->getAverageFps(), 1));
  if (mMovie)
    infoFps.addLine(mMovie->isPlaying() ? "Playing" : "Not playing");
  infoFps.setBorder(4, 2);
  gl::draw(gl::Texture::create(infoFps.render(true)), ivec2(20, 20));
}

void HapPlayerMultiscreenWarpApp::resize()
{
  // tell the warps our window has been resized, so they properly scale up or down
  Warp::handleResize(mWarps);
}

void HapPlayerMultiscreenWarpApp::mouseMove(MouseEvent event)
{
  // pass this mouse event to the warp editor first
  if (!Warp::handleMouseMove(mWarps, event)) {
    // let your application perform its mouseMove handling here
  }
}

void HapPlayerMultiscreenWarpApp::mouseDown(MouseEvent event)
{
  // pass this mouse event to the warp editor first
  if (!Warp::handleMouseDown(mWarps, event)) {
    // let your application perform its mouseDown handling here
  }
}

void HapPlayerMultiscreenWarpApp::mouseDrag(MouseEvent event)
{
  // pass this mouse event to the warp editor first
  if (!Warp::handleMouseDrag(mWarps, event)) {
    // let your application perform its mouseDrag handling here
  }
}

void HapPlayerMultiscreenWarpApp::mouseUp(MouseEvent event)
{
  // pass this mouse event to the warp editor first
  if (!Warp::handleMouseUp(mWarps, event)) {
    // let your application perform its mouseUp handling here
  }
}

void HapPlayerMultiscreenWarpApp::keyDown(KeyEvent event)
{
  // pass this key event to the warp editor first
  if (!Warp::handleKeyDown(mWarps, event)) {
    // warp editor did not handle the key, so handle it here
    switch (event.getCode()) {
    case KeyEvent::KEY_ESCAPE:
      // quit the application
      quit();
      break;
    case KeyEvent::KEY_f:
      // toggle full screen
      setFullScreen(!isFullScreen());
      break;
    case KeyEvent::KEY_b:
      // toggle borderless mode
      getWindow()->setBorderless(!getWindow()->isBorderless());
      break;
    case KeyEvent::KEY_p:
      mPerfTrackerVisible = !mPerfTrackerVisible;
      break;
    case KeyEvent::KEY_s:
      // save app settings
      writeSettings(getCurrentAppSettings(), writeFile(mAppSettingsPath));
      // Save warp settings
      Warp::writeSettings(mWarps, writeFile(mWarpSettingsPath));
      break;
    case KeyEvent::KEY_l:
      // load app settings from file if one exists
      if (fs::exists(mAppSettingsPath))
      {
        auto optAppSettings = readSettings(loadFile(mAppSettingsPath));
        if (optAppSettings)
        {
          applyAppSettings(optAppSettings.get());
        }
      }
      // load warp settings from file if one exists
      if (fs::exists(mWarpSettingsPath))
      {
        mWarps = Warp::readSettings(loadFile(mWarpSettingsPath));
        getWindow()->emitResize();
      }
      break;
    case KeyEvent::KEY_o:
    {
      // open a movie from user-selected path
      fs::path moviePath = getOpenFilePath();
      if (!moviePath.empty())
        loadMovieFile(moviePath);
      break;
    }
    case KeyEvent::KEY_r:
      // reset the movie
      mMovie.reset();
      break;
    case KeyEvent::KEY_v:
      // toggle vertical sync
      gl::enableVerticalSync(!gl::isVerticalSyncEnabled());
      break;
    case KeyEvent::KEY_w:
      // toggle warp edit mode
      Warp::enableEditMode(!Warp::isEditModeEnabled());
      break;
    case KeyEvent::KEY_RETURN:
      createPerspectiveWarp();
      break;
    //case KeyEvent::KEY_a:
    //  // toggle drawing a random region of the image
    //  if (mSrcArea.getWidth() != mImage->getWidth() || mSrcArea.getHeight() != mImage->getHeight())
    //    mSrcArea = mImage->getBounds();
    //  else {
    //    int x1 = Rand::randInt(0, mImage->getWidth() - 150);
    //    int y1 = Rand::randInt(0, mImage->getHeight() - 150);
    //    int x2 = Rand::randInt(x1 + 150, mImage->getWidth());
    //    int y2 = Rand::randInt(y1 + 150, mImage->getHeight());
    //    mSrcArea = Area(x1, y1, x2, y2);
    //  }
    //  break;
    case KeyEvent::KEY_SPACE:
      // toggle drawing mode
      mUseBeginEnd = !mUseBeginEnd;
      //updateWindowTitle();
      break;
    }
  }
}

void HapPlayerMultiscreenWarpApp::keyUp(KeyEvent event)
{
  // pass this key event to the warp editor first
  if (!Warp::handleKeyUp(mWarps, event)) {
    // let your application perform its keyUp handling here
  }
}

void HapPlayerMultiscreenWarpApp::fileDrop(FileDropEvent event)
{
  loadMovieFile(event.getFile(0));
}

void HapPlayerMultiscreenWarpApp::loadMovieFile(const fs::path &moviePath)
{
  try
  {
    mMovie.reset();
    // load up the movie, set it to loop, and begin playing
    mMovie = qtime::MovieGlHap::create(moviePath);
    mMovie->setLoop();
    mMovie->play();

    // create a texture for showing some info about the movie
    TextLayout infoText;
    infoText.clear(ColorA(0.2f, 0.2f, 0.2f, 0.5f));
    infoText.setColor(Color::white());
    infoText.addCenteredLine(moviePath.filename().string());
    infoText.addLine(toString(mMovie->getWidth()) + " x " + toString(mMovie->getHeight()) + " pixels");
    infoText.addLine(toString(mMovie->getDuration()) + " seconds");
    infoText.addLine(toString(mMovie->getNumFrames()) + " frames");
    infoText.addLine(toString(mMovie->getFramerate()) + " fps");
    infoText.addLine(toString(mMovie->isHapQ() ? "HapQ" : "Hap"));
    infoText.setBorder(4, 2);
    mInfoTexture = gl::Texture::create(infoText.render(true));
  }
  catch (...) 
  {
    console() << "Unable to load the movie." << endl;
    mInfoTexture.reset();

    quit();
  }
}

void HapPlayerMultiscreenWarpApp::drawMovie()
{
#if 0
  if (mMovie)
  {
    mMovie->draw();
  }
#endif

  // iterate over the warps and draw their content
  for (auto& warp : mWarps) 
  {
    if (mMovie)
    {
      auto movieTex = mMovie->getTexture();
      if (!movieTex)
        return;

      Rectf centeredRect = Rectf(mMovie->getBounds()).getCenteredFit(app::getWindowBounds(), true);

      float cw = movieTex->getActualWidth();
      float ch = movieTex->getActualHeight();
      float w = movieTex->getWidth();
      float h = movieTex->getHeight();

      //auto srcArea = Area(vec2(0, 0), vec2(w / cw, h / ch));
      auto srcArea = Area(vec2(0, 0), vec2(w, h));

      warp->draw(movieTex, srcArea);
    }
    else if (mHelpImage)
    {
      auto srcArea = mHelpImage->getBounds();
      warp->draw(mHelpImage, srcArea);
    }
  }


#if 0
  if (mHelpImage) {
    // iterate over the warps and draw their content
    for (auto &warp : mWarps) {
      // there are two ways you can use the warps:
      if (mUseBeginEnd) {
        // a) issue your draw commands between begin() and end() statements
        warp->begin();

        // in this demo, we want to draw a specific area of our image,
        // but if you want to draw the whole image, you can simply use: gl::draw( mImage );
        gl::draw(mHelpImage, srcArea, warp->getBounds());

        warp->end();
      }
      else {
        // b) simply draw a texture on them (ideal for video)

        // in this demo, we want to draw a specific area of our image,
        // but if you want to draw the whole image, you can simply use: warp->draw( mImage );
        warp->draw(mHelpImage, srcArea);
      }
    }
  }
#endif
}

optional<AppSettings> HapPlayerMultiscreenWarpApp::readSettings(const ci::DataSourceRef& source)
{
  XmlTree  doc;
  AppSettings result;

  // try to load the specified xml file
  try {
    doc = XmlTree(source);
  }
  catch (...) {
    return optional<AppSettings>();
  }

  // check if this is a valid file
  bool isAppConfig = doc.hasChild("appconfig");
  if (!isAppConfig)
    return optional<AppSettings>();

  // get first window
  XmlTree windowXml = doc.getChild("appconfig/window");

  XmlTree positionXml = windowXml.getChild("position");
  XmlTree sizeXml = windowXml.getChild("size");

  result.mWindowPos.x = positionXml.getAttributeValue<int>("x", 0);
  result.mWindowPos.y = positionXml.getAttributeValue<int>("y", 0);

  result.mWindowSize.x = sizeXml.getAttributeValue<int>("w", 0);
  result.mWindowSize.y = sizeXml.getAttributeValue<int>("h", 0);

  return result;
}

void HapPlayerMultiscreenWarpApp::writeSettings(const AppSettings& appSettings, const ci::DataTargetRef& target)
{
  XmlTree window;
  window.setTag("window");

  XmlTree pos;
  pos.setTag("position");
  pos.setAttribute("x", appSettings.mWindowPos.x);
  pos.setAttribute("y", appSettings.mWindowPos.y);

  XmlTree res;
  res.setTag("size");
  res.setAttribute("w", appSettings.mWindowSize.x);
  res.setAttribute("h", appSettings.mWindowSize.y);

  window.push_back(pos);
  window.push_back(res);

  // create config document and root <warpconfig>
  XmlTree doc;
  doc.setTag("appconfig");
  doc.setAttribute("version", "1.0");

  // add profile to root
  doc.push_back(window);

  // write file
  doc.write(target);
}

AppSettings HapPlayerMultiscreenWarpApp::getCurrentAppSettings() const
{
  AppSettings result;
  result.mWindowPos = getWindow()->getPos();
  result.mWindowSize = getWindow()->getSize();
  return result;
}

void HapPlayerMultiscreenWarpApp::applyAppSettings(const AppSettings& appSettings)
{
  getWindow()->setPos(appSettings.mWindowPos);
  getWindow()->setSize(appSettings.mWindowSize);
  getWindow()->emitResize();
}

void HapPlayerMultiscreenWarpApp::createPerspectiveWarp()
{
  //mWarps.push_back(WarpBilinear::create());
  //mWarps.push_back(WarpPerspectiveBilinear::create());

  WarpPerspectiveRef warpNew = nullptr;
  //for (int i = mWarps.size() - 1; i >= 0; --i)
  for (int i = 0; i < mWarps.size(); ++i)
  {
    if (auto* warpPerspective = dynamic_cast<WarpPerspective*>(mWarps[i].get()))
    {
      warpNew = WarpPerspective::create(*warpPerspective);
    }
  }

  if (warpNew == nullptr)
  {
    warpNew = WarpPerspective::create();
  }

  warpNew->resize();

  mWarps.push_back(warpNew);
}

CINDER_APP(HapPlayerMultiscreenWarpApp,
           RendererGl(RendererGl::Options().msaa(8)),
           &HapPlayerMultiscreenWarpApp::prepareSettings)
