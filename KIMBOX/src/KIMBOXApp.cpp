#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
// Includes
//#include "cinder/app/AppBasic.h"
#include "cinder/Camera.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"
#include "Kinect.h"

#include <iostream>
#include <cstdlib>
#include "RtMidi.h"

#if defined(__WINDOWS_MM__)
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#endif


using namespace ci;
using namespace ci::app;
using namespace std;

class KIMBOXApp : public AppNative {
 public:

	// Cinder callbacks
	void	draw();
	void	keyDown( ci::app::KeyEvent event );
	void	prepareSettings( ci::app::AppBasic::Settings *settings );
	void	setup();
	void	shutdown();
	void	update();

private:

	// Kinect
	uint32_t							mCallbackId;
	KinectSdk::KinectRef				mKinect;
	std::vector<KinectSdk::Skeleton>	mSkeletons;
	void								onSkeletonData( std::vector<KinectSdk::Skeleton> skeletons, 
		const KinectSdk::DeviceOptions &deviceOptions );

	// Camera
	ci::CameraPersp						mCamera;

	// Save screenshot
	void								screenShot();

	//MIDI
	RtMidiOut							*rtmidi;

};

// Imports
using namespace ci;
using namespace ci::app;
using namespace KinectSdk;
using namespace std;

// Render
void KIMBOXApp::draw()
{

	// Clear window
	gl::setViewport( getWindowBounds() );
	gl::clear( Colorf::gray( 0.1f ) );

	// We're capturing
	if ( mKinect->isCapturing() ) {

		// Set up 3D view
		gl::setMatrices( mCamera );

		// Iterate through skeletons
		uint32_t i = 0;
		for ( vector<Skeleton>::const_iterator skeletonIt = mSkeletons.cbegin(); skeletonIt != mSkeletons.cend(); ++skeletonIt, i++ ) {

			// Set color
			Colorf color = mKinect->getUserColor( i );

			// Iterate through joints
			for ( Skeleton::const_iterator boneIt = skeletonIt->cbegin(); boneIt != skeletonIt->cend(); ++boneIt ) {

				// Set user color
				gl::color( color );

				// Get position and rotation
				const Bone& bone	= boneIt->second;
				Vec3f position		= bone.getPosition();
				Matrix44f transform	= bone.getAbsoluteRotationMatrix();
				Vec3f direction		= transform.transformPoint( position ).normalized();
				direction			*= 0.05f;
				position.z			*= -1.0f;

				// Draw bone
				glLineWidth( 2.0f );
				JointName startJoint = bone.getStartJoint();
				if ( skeletonIt->find( startJoint ) != skeletonIt->end() ) {
					Vec3f destination	= skeletonIt->find( startJoint )->second.getPosition();
					destination.z		*= -1.0f;
					gl::drawLine( position, destination );
					
					gl::drawString( "HELLO!", Vec2i::zero() );
				}

				// Draw joint
				gl::drawSphere( position, 0.025f, 16 );

				// Draw joint orientation
				glLineWidth( 0.5f );
				gl::color( ColorAf::white() );
				gl::drawVector( position, position + direction, 0.05f, 0.01f );

				

			}

		}

	}

}

// Handles key press
void KIMBOXApp::keyDown( KeyEvent event )
{

	// Quit, toggle fullscreen
	switch ( event.getCode() ) {
	case KeyEvent::KEY_q:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_SPACE:
		screenShot();
		break;
	}

}

// Receives skeleton data
void KIMBOXApp::onSkeletonData( vector<Skeleton> skeletons, const DeviceOptions &deviceOptions )
{
	mSkeletons = skeletons;
}

// Prepare window
void KIMBOXApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
	settings->setFrameRate( 60.0f );
}

// Take screen shot
void KIMBOXApp::screenShot()
{
	writeImage( getAppPath() / ( "frame" + toString( getElapsedFrames() ) + ".png" ), copyWindowSurface() );
}

// Set up
void KIMBOXApp::setup()
{

	// Start Kinect
	mKinect = Kinect::create();
	mKinect->start( DeviceOptions().enableDepth( true ).enableColor( true ) );
	mKinect->removeBackground();

	// Set the skeleton smoothing to remove jitters. Better smoothing means
	// less jitters, but a slower response time.
	mKinect->setTransform( Kinect::TRANSFORM_SMOOTH );
	
	// Add callback to receive skeleton data
	mCallbackId = mKinect->addSkeletonTrackingCallback( &KIMBOXApp::onSkeletonData, this );

	// Set up camera
	mCamera.lookAt( Vec3f( 0.0f, 0.0f, 2.0f ), Vec3f::zero() );
	mCamera.setPerspective( 45.0f, getWindowAspectRatio(), 0.01f, 1000.0f );

	//Set up MIDI
	rtmidi = new RtMidiOut(RtMidi::Api::WINDOWS_MM);
	if(rtmidi->getPortCount() > 0)
	{
		std::string portName;
		portName = rtmidi->getPortName(1); //hacer configurable
		rtmidi->openPort(1);
	}

	
	 //std::vector<unsigned char> message;

	  // RtMidiOut constructor
	 // try {
		//midiout = new RtMidiOut();
	 // }
	 // catch ( RtError &error ) {
		////error.printMessage();
		////exit( EXIT_FAILURE );
	 // }

}

// Called on exit
void KIMBOXApp::shutdown()
{

	// Stop input
	mKinect->removeCallback( mCallbackId );
	mKinect->stop();

}

// Runs update logic
void KIMBOXApp::update()
{

	if ( mKinect->isCapturing() ) {
		mKinect->update();
	} else {
		// If Kinect initialization failed, try again every 90 frames
		if ( getElapsedFrames() % 90 == 0 ) {
			mKinect->start();
		}
	}
}

CINDER_APP_NATIVE( KIMBOXApp, RendererGl )
