/*
 *  MovieGlHap.cpp
 *
 *  Created by Roger Sodre on 14/05/10.
 *  Copyright 2010 Studio Avante. All rights reserved.
 *
 */

#include "MovieHap.h"

extern "C" {
#include "HapSupport.h"
}

#include "Resources.h"

#include "cinder/CinderAssert.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/Color.h"
#include "cinder/gl/Context.h"
#include "cinder/GeomIo.h"


#if defined( CINDER_MAC )
	#include <QTKit/QTKit.h>
	#include <QTKit/QTMovie.h>
	#include <CoreVideo/CoreVideo.h>
	#include <CoreVideo/CVBase.h>
#else
	#pragma push_macro( "__STDC_CONSTANT_MACROS" )
	#pragma push_macro( "_STDINT_H" )
	#undef __STDC_CONSTANT_MACROS

	#include <QTML.h>
	#include <CVPixelBuffer.h>
	#include <ImageCompression.h>
	#include <Movies.h>
	#include <GXMath.h>
	#pragma pop_macro( "_STDINT_H" )
	#pragma pop_macro( "__STDC_CONSTANT_MACROS" )

	// this call is improperly defined as Mac-only in the headers
	extern "C" {
		EXTERN_API_C( OSStatus ) QTPixelBufferContextCreate( CFAllocatorRef, CFDictionaryRef, QTVisualContextRef* );
	}


	enum CVPixelBufferLockFlags {
		kCVPixelBufferLock_ReadOnly = 0x00000001,
	};

#endif

namespace cinder { namespace qtime {

	// Playback Framerate
	uint32_t _FrameCount = 0;
	uint32_t _FpsLastSampleFrame = 0;
	double _FpsLastSampleTime = 0;
	float _AverageFps = 0;
	
	void updateMovieFPS( long time, void *ptr )
	{
    // FIXME: crashes
 		//double now = app::getElapsedSeconds();
		//if( now > _FpsLastSampleTime + app::App::get()->getFpsSampleInterval() ) {
		//	//calculate average Fps over sample interval
		//	uint32_t framesPassed = _FrameCount - _FpsLastSampleFrame;
		//	_AverageFps = (float)(framesPassed / (now - _FpsLastSampleTime));
		//	_FpsLastSampleTime = now;
		//	_FpsLastSampleFrame = _FrameCount;
		//}
		//_FrameCount++;
	}
	
	float MovieGlHap::getPlaybackFramerate() const
	{
		return _AverageFps;
	}
	
	gl::GlslProgRef MovieGlHap::Obj::sHapQShader = nullptr;
	
	MovieGlHap::Obj::Obj()
	: MovieBase::Obj()
  //, mDefaultShader( gl::getStockShader( gl::ShaderDef().texture() ) )
  , mUnityTexture(nullptr)
  , mUnityMode(UnityMode::OpenGl)
	{
		//std::call_once( mHapQOnceFlag, []() {
		//	MovieGlHap::Obj::sHapQShader = gl::GlslProg::create( app::loadResource(RES_HAP_VERT),  app::loadResource(RES_HAP_FRAG) );
		//} );

    //// Just make sure that TexCoord is in there
    //auto rectAttribSet = geom::Rect().getAvailableAttribs();
    //if (rectAttribSet.count(geom::TEX_COORD_0) == 0)
    //{
    //  //ExitProcess(0);
    //}

    //Rectf fsQuad(-1.0f, -1.0f, 1.0f, 1.0f);
    //mFullscreenQuadDefaultBatch = gl::Batch::create(geom::Rect(fsQuad), mDefaultShader);
    //mFullscreenQuadHapQBatch = gl::Batch::create(geom::Rect(fsQuad), MovieGlHap::Obj::sHapQShader);
	}
	
	MovieGlHap::Obj::~Obj()
	{
		// see note on prepareForDestruction()
		prepareForDestruction();
		if (mTexture)
      mTexture.reset();
	}
	
	
	MovieGlHap::MovieGlHap( const MovieLoader &loader )
	: MovieBase(), mObj( new Obj() )
	{
		MovieBase::initFromLoader( loader );
		allocateVisualContext();
	}
	
	MovieGlHap::MovieGlHap( const fs::path &path )
	: MovieBase(), mObj( new Obj() )
	{
		MovieBase::initFromPath( path );
		allocateVisualContext();
	}
	
	MovieGlHap::MovieGlHap( const void *data, size_t dataSize, const std::string &fileNameHint, const std::string &mimeTypeHint )
	: MovieBase(), mObj( new Obj() )
	{
		MovieBase::initFromMemory( data, dataSize, fileNameHint, mimeTypeHint );
		allocateVisualContext();
	}
	
	MovieGlHap::MovieGlHap( DataSourceRef dataSource, const std::string mimeTypeHint )
	: MovieBase(), mObj( new Obj() )
	{
		MovieBase::initFromDataSource( dataSource, mimeTypeHint );
		allocateVisualContext();
	}

  bool MovieGlHap::setUnityTexture(void* unityTexture, int w, int h)
  {
    // FIXME: check size

    assert(unityTexture != nullptr);
    mObj->mUnityTexture = unityTexture;

    return true;
  }

  void MovieGlHap::updateUnityTexture()
  {
    updateFrame();
  }

  MovieGlHap::~MovieGlHap()
	{
		//CI_LOG_I( "Detroying movie hap." );
	}

	void MovieGlHap::allocateVisualContext()
	{
		// Load HAP Movie
		if( HapQTQuickTimeMovieHasHapTrackPlayable( getObj()->mMovie ) )
		{
			// QT Visual Context attributes
			OSStatus err = noErr;
			QTVisualContextRef * visualContext = (QTVisualContextRef*)&getObj()->mVisualContext;
			CFDictionaryRef pixelBufferOptions = HapQTCreateCVPixelBufferOptionsDictionary();
			
			const CFStringRef keys[] = { kQTVisualContextPixelBufferAttributesKey };
			CFDictionaryRef visualContextOptions = ::CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys, (const void**)&pixelBufferOptions, sizeof(keys)/sizeof(keys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			err = QTPixelBufferContextCreate( kCFAllocatorDefault, visualContextOptions, visualContext );
			
			::CFRelease( pixelBufferOptions );
			::CFRelease( visualContextOptions );
			
			if( err != noErr ) {
				CI_LOG_E( "HAP ERROR :: " << err << " couldnt create visual context." );
				return;
			}
			// Set the movie's visual context
			err = SetMovieVisualContext( getObj()->mMovie, *visualContext );
			if( err != noErr ) {
				CI_LOG_E( "HAP ERROR :: " << err << " SetMovieVisualContext." );
				return;
			}
		}
		
		// Get codec name
		for (long i = 1; i <= GetMovieTrackCount(getObj()->mMovie); i++) {
            Track track = GetMovieIndTrack(getObj()->mMovie, i);
            Media media = GetTrackMedia(track);
            OSType mediaType;
            GetMediaHandlerDescription(media, &mediaType, NULL, NULL);
            if (mediaType == VideoMediaType)
            {
                // Get the codec-type of this track
                ImageDescriptionHandle imageDescription = (ImageDescriptionHandle)NewHandle(0); // GetMediaSampleDescription will resize it
                GetMediaSampleDescription(media, 1, (SampleDescriptionHandle)imageDescription);
                OSType codecType = (*imageDescription)->cType;
                DisposeHandle((Handle)imageDescription);
                
                switch (codecType) {
                    case 'Hap1': mCodec = Codec::HAP; break;
                    case 'Hap5': mCodec = Codec::HAP_A; break;
                    case 'HapY': mCodec = Codec::HAP_Q; break;
                    default: mCodec = Codec::UNSUPPORTED; break;
				}
            }
        }

		// Set framerate callback
		this->setNewFrameCallback( updateMovieFPS, (void*)this );

    // Goes here, no errors
	}
	
//#if defined( CINDER_MAC )
//	static void CVOpenGLTextureDealloc( gl::Texture *texture, void *refcon )
//	{
//		CVOpenGLTextureRelease( (CVImageBufferRef)(refcon) );
//		delete texture;
//	}
//#endif // defined( CINDER_MAC )
	
	void MovieGlHap::Obj::releaseFrame()
	{
	}
	
	void MovieGlHap::Obj::newFrame( CVImageBufferRef cvImage )
	{
    //ExitProcess(0);

		::CVPixelBufferLockBaseAddress( cvImage, kCVPixelBufferLock_ReadOnly );
		// Load HAP frame
		if( ::CFGetTypeID( cvImage ) == ::CVPixelBufferGetTypeID() ) {
			GLuint width = ::CVPixelBufferGetWidth( cvImage );
			GLuint height = ::CVPixelBufferGetHeight( cvImage );
			
			CI_ASSERT( cvImage != NULL );
			
			// Check the buffer padding
			size_t extraRight, extraBottom;
			::CVPixelBufferGetExtendedPixels( cvImage, NULL, &extraRight, NULL, &extraBottom );
			GLuint roundedWidth = width + extraRight;
			GLuint roundedHeight = height + extraBottom;
			
			// Valid DXT will be a multiple of 4 wide and high
			CI_ASSERT( !(roundedWidth % 4 != 0 || roundedHeight % 4 != 0) );
			OSType newPixelFormat = ::CVPixelBufferGetPixelFormatType( cvImage );
			GLenum internalFormat;
			unsigned int bitsPerPixel;
			switch (newPixelFormat) {
				case kHapPixelFormatTypeRGB_DXT1:
					internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
					bitsPerPixel = 4;
					break;
				case kHapPixelFormatTypeRGBA_DXT5:
				case kHapPixelFormatTypeYCoCg_DXT5:
					internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					bitsPerPixel = 8;
					break;
				default:
					CI_ASSERT_MSG( false, "We don't support non-DXT pixel buffers." );
					return;
					break;
			}
			
			// Ignore the value for CVPixelBufferGetBytesPerRow()
			size_t	bytesPerRow = (roundedWidth * bitsPerPixel) / 8;
			GLsizei	dataLength = bytesPerRow * roundedHeight; // usually not the full length of the buffer
			size_t	actualBufferSize = ::CVPixelBufferGetDataSize( cvImage );
			
			// Check the buffer is as large as we expect it to be
			CI_ASSERT( dataLength < actualBufferSize );
			
			GLvoid *baseAddress = ::CVPixelBufferGetBaseAddress( cvImage );

      if (mUnityTexture)
      {
        if (mUnityMode == UnityMode::OpenGl)
        {
          GLuint textureId = (GLuint)(size_t)(mUnityTexture);

          auto getErrorString = [](GLenum err) -> std::string
          {
            switch (err) {
            case GL_NO_ERROR:
              return "GL_NO_ERROR";
            case GL_INVALID_ENUM:
              return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:
              return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:
              return "GL_INVALID_OPERATION";
            case GL_INVALID_FRAMEBUFFER_OPERATION:
              return "GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY:
              return "GL_OUT_OF_MEMORY";
            default:
              return "";
            }
          };

          // FIXME: crashes
          //gl::ScopedTextureBind bind(GL_TEXTURE_2D, textureId);

          //glEnable(GL_TEXTURE_2D);
          auto error = getErrorString(glGetError());

          glBindTexture(GL_TEXTURE_2D, textureId);
          error = getErrorString(glGetError());

          GLint mInternalFormat;
          glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &mInternalFormat);
          assert(mInternalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
          error = getErrorString(glGetError());
          
          GLint mCompressed;
          glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &mCompressed);
          assert(mCompressed == 1);
          error = getErrorString(glGetError());

          glCompressedTexSubImage2D(GL_TEXTURE_2D,
                                    0,
                                    0,
                                    0,
                                    roundedWidth,
                                    roundedHeight,
                                    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                                    dataLength,
                                    baseAddress);
          error = getErrorString(glGetError());
          //glBindTexture(GL_TEXTURE_2D, 0);
          //glDisable(GL_TEXTURE_2D);
        }
        else if (mUnityMode == UnityMode::D3D11)
        {
          // TODO
        }
      }
			else 
      {
        if (!mTexture)
        {
          // On NVIDIA hardware there is a massive slowdown if DXT textures aren't POT-dimensioned, so we use POT-dimensioned backing
          GLuint backingWidth = 1;
          while (backingWidth < roundedWidth) backingWidth <<= 1;

          GLuint backingHeight = 1;
          while (backingHeight < roundedHeight) backingHeight <<= 1;

          // We allocate the texture with no pixel data, then use CompressedTexSubImage to update the content region
          gl::Texture2d::Format format;
          format.wrap(GL_CLAMP_TO_EDGE).magFilter(GL_LINEAR).minFilter(GL_LINEAR).internalFormat(internalFormat).dataType(GL_UNSIGNED_INT_8_8_8_8_REV).immutableStorage();// .pixelDataFormat( GL_BGRA );
          mTexture = gl::Texture2d::create(backingWidth, backingHeight, format);
          mTexture->setCleanBounds(Area(0, 0, width, height));

          //CI_LOG_I("Created texture.");

#if defined( CINDER_MAC )
          /// There is no default format GL_TEXTURE_STORAGE_HINT_APPLE param so we fill it manually
          gl::ScopedTextureBind bind( mTexture->getTarget(), mTexture->getId() );
          glTexParameteri( mTexture->getTarget(), GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE );
#endif
        }

        gl::ScopedTextureBind bind( mTexture );
#if defined( CINDER_MAC )
			  glTextureRangeAPPLE( mTexture->getTarget(), dataLength, baseAddress );
			  /* WARNING: Even though it is present here:
			   * https://github.com/Vidvox/hap-quicktime-playback-demo/blob/master/HapQuickTimePlayback/HapPixelBufferTexture.m#L186
			   * the following call does not appear necessary. Furthermore, it corrupts display
			   * when movies are loaded more than once
			   */
  //			glPixelStorei( GL_UNPACK_CLIENT_STORAGE_APPLE, 1 );
#endif
        auto internalFormat = mTexture->getInternalFormat();
			  glCompressedTexSubImage2D(mTexture->getTarget(),
									                0,
									                0,
									                0,
									                roundedWidth,
									                roundedHeight,
									                mTexture->getInternalFormat(),
									                dataLength,
									                baseAddress);
        }
		}
		
		::CVPixelBufferUnlockBaseAddress( cvImage, kCVPixelBufferLock_ReadOnly );
		::CVPixelBufferRelease(cvImage);
	}

  MovieBase::Obj* MovieGlHap::getObj() const
	{
	  return mObj.get();
	}

  gl::TextureRef MovieGlHap::getTexture()
	{
		updateFrame();
		
		mObj->lock();
		auto texture = mObj->mTexture;
		mObj->unlock();
		
		return texture;
	}
	
	gl::GlslProgRef MovieGlHap::getGlsl() const
	{
		return isHapQ() ? MovieGlHap::Obj::sHapQShader : mObj->mDefaultShader;
	}
	
	void MovieGlHap::draw()
	{
		updateFrame();
		
    //gl::disableAlphaBlending();

    //gl::color(Color(0.0f, 1.0f, 0.0f));;
    //gl::drawSolidRect(Rectf(0, 0, getWidth(), getHeight()), vec2(0, 0));
    //return;

		mObj->lock();
		if (mObj->mTexture)
    {
			Rectf centeredRect = Rectf(mObj->mTexture->getBounds()).getCenteredFit(app::getWindowBounds(), true);
			gl::color(Color::white());
			
			auto drawRect = [&]()
      {
				gl::ScopedTextureBind tex(mObj->mTexture);
				float cw = mObj->mTexture->getActualWidth();
				float ch = mObj->mTexture->getActualHeight();
				float w = mObj->mTexture->getWidth();
				float h = mObj->mTexture->getHeight();
				//gl::drawSolidRect(centeredRect , vec2(0, 0), vec2(cw / w, ch / h) );

        // No tex coords here?
				gl::drawSolidRect(centeredRect, vec2(0, 0), vec2(w / cw, h / ch));
        //gl::drawSolidRect(Rectf(0, 0, getWidth(), getHeight()), vec2(0, 0));
			};
			
			if (isHapQ()) 
      {
        //mObj->mFullscreenQuadHapQBatch->draw();

				gl::ScopedGlslProg bind(MovieGlHap::Obj::sHapQShader);
				drawRect();
			}
      else
      {
        //gl::draw(mObj->mTexture, Rectf(0, 0, getWidth(), getHeight()));
        
        //gl::ScopedTextureBind tex(mObj->mTexture);
        //mObj->mFullscreenQuadDefaultBatch->draw();

				gl::ScopedGlslProg bind(mObj->mDefaultShader);
				drawRect();
			}
		}
    else
    {
      // Indicate that something is wrong
      gl::color(Color(0.0f, 1.0f, 0.0f));;
      gl::drawSolidRect(Rectf(0, 0, getWidth(), getHeight()), vec2(0, 0));
    }
		mObj->unlock();
	}
} } //namespace cinder::qtime
