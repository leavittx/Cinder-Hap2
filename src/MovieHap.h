/*
 *  MovieHap.h
 *
 *  Created by Roger Sodre on 14/05/10.
 *  Copyright 2010 Studio Avante. All rights reserved.
 *
 */
#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Batch.h"

#if defined( CINDER_MSW )
#define _STDINT_H
#define __FP__
#endif
#include "cinder/qtime/QuicktimeGl.h"

namespace cinder { namespace qtime {
	
	typedef std::shared_ptr<class MovieGlHap> MovieGlHapRef;
	
	class MovieGlHap : public MovieBase {
	public:
		enum class Codec { HAP, HAP_A, HAP_Q, UNSUPPORTED };
		
		~MovieGlHap();
		MovieGlHap( const fs::path &path );
		MovieGlHap( const class MovieLoader &loader );
		MovieGlHap( const void *data, size_t dataSize, const std::string &fileNameHint, const std::string &mimeTypeHint = "" );
		MovieGlHap( DataSourceRef dataSource, const std::string mimeTypeHint = "" );
		
    bool setUnityTexture(void* unityTexture, int w, int h);
    void updateUnityTexture();

		gl::Texture2dRef getTexture();
		gl::GlslProgRef getGlsl() const;
		void draw();
    //void draw(const ci::);
		
		bool			isHap() const { return mCodec == Codec::HAP; }
		bool			isHapA() const { return mCodec == Codec::HAP_A; }
		bool			isHapQ() const { return mCodec == Codec::HAP_Q; }
		
		const Codec&	getCodecName() const { return mCodec; }
		float			getPlaybackFramerate() const;
		
		static MovieGlHapRef create( const fs::path &path ) { return MovieGlHapRef( new MovieGlHap( path ) ); }
		static MovieGlHapRef create( const MovieLoaderRef &loader );
		static MovieGlHapRef create( const void *data, size_t dataSize, const std::string &fileNameHint, const std::string &mimeTypeHint = "" )
		{ return MovieGlHapRef( new MovieGlHap( data, dataSize, fileNameHint, mimeTypeHint ) ); }
		static MovieGlHapRef create( DataSourceRef dataSource, const std::string mimeTypeHint = "" )
		{ return MovieGlHapRef( new MovieGlHap( dataSource, mimeTypeHint ) ); }
	protected:
		
		void allocateVisualContext();

    enum class UnityMode
    {
      OpenGl,
      D3D11
    };

		struct Obj : public MovieBase::Obj {
			Obj();
			~Obj();
		  void		releaseFrame() override;
		  void		newFrame( CVImageBufferRef cvImage ) override;
      gl::Texture2dRef	mTexture;
      void*             mUnityTexture;
      UnityMode         mUnityMode;
			gl::GlslProgRef		mDefaultShader;
			static gl::GlslProgRef	sHapQShader;
      gl::BatchRef      mFullscreenQuadHapQBatch;
      gl::BatchRef      mFullscreenQuadDefaultBatch;
			std::once_flag			mHapQOnceFlag;
		};

		std::unique_ptr<Obj>		mObj;
	  MovieBase::Obj* getObj() const override;

	  Codec						mCodec;
	};

} } //namespace cinder::qtime
