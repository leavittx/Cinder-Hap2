#pragma once

#include "cinder/gl/Query.h"
#include "cinder/Timer.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Batch.h"
#include "cinder/Area.h"
#include "cinder/Color.h"

#include <boost/noncopyable.hpp>

typedef std::shared_ptr<class PerfTracker> PerfTrackerRef;

#include <deque>

class PerfTracker : public boost::noncopyable {
public:
	class Signal {
	public:
		Signal( std::string name );
		
		virtual void startFrame() = 0;
		virtual void endFrame() = 0;
		
		std::string					mName;
		std::deque<double>			mFrameTimes;
		
		ci::Color					mColor;
		ci::gl::VboRef				mPositionVbo;
		ci::gl::BatchRef			mLine;
		std::pair<double, double>	mMinMax;
	};
	
	class CpuSignal : public Signal {
	public:
		CpuSignal( std::string name );
		void startFrame() override;
		void endFrame() override { }
	protected:
		ci::Timer mTimer;
	};
	
	class GpuSignal : public Signal {
	public:
		GpuSignal( std::string name );
		void startFrame() override;
		void endFrame() override;
	protected:
		ci::gl::QueryTimeSwappedRef mQuery;
	};
	
	static PerfTrackerRef create( const ci::Area& area ) { return PerfTrackerRef( new PerfTracker( area ) ); }
	
	void startFrame();
	void endFrame();
	
	void update();
	void draw();
private:
	PerfTracker( const ci::Area& area );
	
	ci::Area			mArea;
	static const size_t	sMaxFrames;
	std::vector<std::unique_ptr<Signal>> mSignals;
};
