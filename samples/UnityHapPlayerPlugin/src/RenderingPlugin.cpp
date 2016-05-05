// Example low level rendering Unity plugin
// http://docs.unity3d.com/Manual/CommandLineArguments.html
#include "RenderingPlugin.h"
#include "Unity/IUnityGraphics.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include <string>

#include "MovieHap.h"

// --------------------------------------------------------------------------
// Include headers for the graphics APIs we support

#if SUPPORT_D3D9
#	include <d3d9.h>
#	include "Unity/IUnityGraphicsD3D9.h"
#endif
#if SUPPORT_D3D11
#	include <d3d11.h>
#	include "Unity/IUnityGraphicsD3D11.h"
#endif
#if SUPPORT_D3D12
#	include <d3d12.h>
#	include "Unity/IUnityGraphicsD3D12.h"
#endif

#if SUPPORT_OPENGL_LEGACY
  #include <cinder/gl/gl.h>
  #include <cinder/gl/platform.h>
  //using namespace ci;
#endif
#if SUPPORT_OPENGL_UNIFIED
#	if UNITY_IPHONE
#		include <OpenGLES/ES2/gl.h>
#	elif UNITY_ANDROID
#		include <GLES2/gl2.h>
#	else
  #include <cinder/gl/gl.h>
  #include <cinder/gl/platform.h>
  //using namespace ci;
#	endif
#endif


// --------------------------------------------------------------------------
// Helper utilities


// Prints a string
static void DebugLog (const char* str)
{
	#if UNITY_WIN
	OutputDebugStringA (str);
	#else
	printf ("%s", str);
	#endif
}

// COM-like Release macro
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(a) if (a) { a->Release(); a = NULL; }
#endif



// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity (float t) { g_Time = t; }


// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of the scripts.

static void* g_TexturePointer = nullptr;
#ifdef SUPPORT_OPENGL_UNIFIED
static int   g_TexWidth = 0;
static int   g_TexHeight = 0;
#endif

//#if SUPPORT_OPENGL_UNIFIED
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieSetTextureFromUnity(void* texturePtr, int w, int h)
//#else
//extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTextureFromUnity(void* texturePtr)
//#endif
{
  // A script calls this at initialization time; just remember the texture pointer here.
  // Will update texture pixels each frame from the plugin rendering event (texture update
  // needs to happen on the rendering thread).
  g_TexturePointer = texturePtr;
#if SUPPORT_OPENGL_UNIFIED
  g_TexWidth = w;
  g_TexHeight = h;
#endif
}

// --------------------------------------------------------------------------
// HapMovieLoadFromUnity, an example function we export which is called by one of the scripts.

static std::string g_MoviePath; 
static ci::qtime::MovieGlHapRef g_Movie;

// No std::string, see http://stackoverflow.com/a/874575
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieLoadFromUnity(const char* path)
{
  try
  {
    g_MoviePath = path;
    g_Movie.reset();
    g_Movie = ci::qtime::MovieGlHap::create(g_MoviePath);
    auto width = g_Movie->getWidth(), height = g_Movie->getHeight();
    
    return true;
  }
  catch (...)
  {
    g_MoviePath.clear();
    g_Movie.reset();
    return false;
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieResetFromUnity(const char* path)
{
  g_MoviePath = "";
  g_Movie.reset();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieGetResolutionFromUnity(int32_t& width, int32_t& height)
{
  if (g_Movie)
  {
    width = g_Movie->getWidth();
    height = g_Movie->getHeight();
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieSetLoopFromUnity(bool isLoop, bool isPalindrome)
{
  if (g_Movie)
  {
    g_Movie->setLoop(isLoop, isPalindrome);
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieSetVolumeFromUnity(float volume)
{
  if (g_Movie)
  {
    g_Movie->setVolume(volume);
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMoviePlayFromUnity()
{
  if (g_Movie)
  {
    g_Movie->play();
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieStopFromUnity()
{
  if (g_Movie)
  {
    g_Movie->stop();
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieSetPlaybackRateFromUnity(float rate)
{
  if (g_Movie)
  {
    g_Movie->setRate(rate);
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieSeekToFrameFromUnity(int frame)
{
  if (g_Movie)
  {
    if (frame < 0 || frame >= g_Movie->getNumFrames())
      return;

    g_Movie->seekToFrame(frame);
  }
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieGetNumFramesFromUnity()
{
  if (g_Movie)
  {
    return g_Movie->getNumFrames();
  }
  return 0;
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieGetFramerateFromUnity()
{
  if (g_Movie)
  {
    return g_Movie->getFramerate();
  }
  return 0.0f;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieStepForwardFromUnity()
{
  if (g_Movie)
  {
    g_Movie->stepForward();
  }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieStepBackwardFromUnity()
{
  if (g_Movie)
  {
    g_Movie->stepBackward();
  }
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieGetDurationFromUnity()
{
  if (g_Movie)
  {
    return g_Movie->getDuration();
  }
  return 0.0f;
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API HapMovieGetCurrentTimeFromUnity()
{
  if (g_Movie)
  {
    return g_Movie->getCurrentTime();
  }
  return 0.0f;
}


enum
{
	ATTRIB_POSITION = 0,
	ATTRIB_COLOR = 1
};

// --------------------------------------------------------------------------
// SetUnityStreamingAssetsPath, an example function we export which is called by one of the scripts.

static std::string s_UnityStreamingAssetsPath;
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetUnityStreamingAssetsPath(const char* path)
{
	s_UnityStreamingAssetsPath = path;
}



// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	
	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}



// --------------------------------------------------------------------------
// GraphicsDeviceEvent

// Actual setup/teardown functions defined below
#if SUPPORT_D3D9
static void DoEventGraphicsDeviceD3D9(UnityGfxDeviceEventType eventType);
#endif
#if SUPPORT_D3D11
static void DoEventGraphicsDeviceD3D11(UnityGfxDeviceEventType eventType);
#endif
#if SUPPORT_D3D12
static void DoEventGraphicsDeviceD3D12(UnityGfxDeviceEventType eventType);
#endif
#if SUPPORT_OPENGL_UNIFIED
static void DoEventGraphicsDeviceGLUnified(UnityGfxDeviceEventType eventType);
#endif

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	UnityGfxRenderer currentDeviceType = s_DeviceType;

	switch (eventType)
	{
	case kUnityGfxDeviceEventInitialize:
		{
			DebugLog("OnGraphicsDeviceEvent(Initialize).\n");
			s_DeviceType = s_Graphics->GetRenderer();
			currentDeviceType = s_DeviceType;
			break;
		}

	case kUnityGfxDeviceEventShutdown:
		{
			DebugLog("OnGraphicsDeviceEvent(Shutdown).\n");
			s_DeviceType = kUnityGfxRendererNull;
			g_TexturePointer = NULL;
			break;
		}

	case kUnityGfxDeviceEventBeforeReset:
		{
			DebugLog("OnGraphicsDeviceEvent(BeforeReset).\n");
			break;
		}

	case kUnityGfxDeviceEventAfterReset:
		{
			DebugLog("OnGraphicsDeviceEvent(AfterReset).\n");
			break;
		}
	};

	#if SUPPORT_D3D9
	if (currentDeviceType == kUnityGfxRendererD3D9)
		DoEventGraphicsDeviceD3D9(eventType);
	#endif

	#if SUPPORT_D3D11
	if (currentDeviceType == kUnityGfxRendererD3D11)
		DoEventGraphicsDeviceD3D11(eventType);
	#endif

	#if SUPPORT_D3D12
	if (currentDeviceType == kUnityGfxRendererD3D12)
		DoEventGraphicsDeviceD3D12(eventType);
	#endif
	
	#if SUPPORT_OPENGL_UNIFIED
	if (currentDeviceType == kUnityGfxRendererOpenGLES20 ||
		currentDeviceType == kUnityGfxRendererOpenGLES30 ||
		currentDeviceType == kUnityGfxRendererOpenGLCore)
		DoEventGraphicsDeviceGLUnified(eventType);
	#endif
}



// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.


struct MyVertex {
	float x, y, z;
	unsigned int color;
};
static void SetDefaultGraphicsState ();
static void DoRendering (const float* worldMatrix, const float* identityMatrix, float* projectionMatrix, const MyVertex* verts);

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	// Unknown graphics device type? Do nothing.
	if (s_DeviceType == kUnityGfxRendererNull)
		return;


	// A colored triangle. Note that colors will come out differently
	// in D3D9/11 and OpenGL, for example, since they expect color bytes
	// in different ordering.
	MyVertex verts[3] = {
		{ -0.5f, -0.25f,  0, 0xFFff0000 },
		{  0.5f, -0.25f,  0, 0xFF00ff00 },
		{  0,     0.5f ,  0, 0xFF0000ff },
	};


	// Some transformation matrices: rotate around Z axis for world
	// matrix, identity view matrix, and identity projection matrix.

	float phi = g_Time;
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	float worldMatrix[16] = {
		cosPhi,-sinPhi,0,0,
		sinPhi,cosPhi,0,0,
		0,0,1,0,
		0,0,0.7f,1,
	};
	float identityMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};
	float projectionMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};

	// Actual functions defined below
	SetDefaultGraphicsState ();
	DoRendering (worldMatrix, identityMatrix, projectionMatrix, verts);
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}



// -------------------------------------------------------------------
// Shared code

#if SUPPORT_D3D11
typedef std::vector<unsigned char> Buffer;
bool LoadFileIntoBuffer(const std::string& fileName, Buffer& data)
{
	FILE* fp;
	fopen_s(&fp, fileName.c_str(), "rb");
	if (fp)
	{
		fseek (fp, 0, SEEK_END);
		int size = ftell (fp);
		fseek (fp, 0, SEEK_SET);
		data.resize(size);

		fread(&data[0], size, 1, fp);

		fclose(fp);

		return true;
	}
	else
	{
		std::string errorMessage = "Failed to find ";
		errorMessage += fileName;
		DebugLog(errorMessage.c_str());
		return false;
	}
}
#endif


// -------------------------------------------------------------------
//  Direct3D 9 setup/teardown code


#if SUPPORT_D3D9

static IDirect3DDevice9* g_D3D9Device;

// A dynamic vertex buffer just to demonstrate how to handle D3D9 device resets.
static IDirect3DVertexBuffer9* g_D3D9DynamicVB;

static void DoEventGraphicsDeviceD3D9(UnityGfxDeviceEventType eventType)
{
	// Create or release a small dynamic vertex buffer depending on the event type.
	switch (eventType) {
	case kUnityGfxDeviceEventInitialize:
		{
			IUnityGraphicsD3D9* d3d9 = s_UnityInterfaces->Get<IUnityGraphicsD3D9>();
			g_D3D9Device = d3d9->GetDevice();
		}
	case kUnityGfxDeviceEventAfterReset:
		// After device is initialized or was just reset, create the VB.
		if (!g_D3D9DynamicVB)
			g_D3D9Device->CreateVertexBuffer (1024, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &g_D3D9DynamicVB, NULL);
		break;
	case kUnityGfxDeviceEventBeforeReset:
	case kUnityGfxDeviceEventShutdown:
		// Before device is reset or being shut down, release the VB.
		SAFE_RELEASE(g_D3D9DynamicVB);
		break;
	}
}

#endif // #if SUPPORT_D3D9



// -------------------------------------------------------------------
//  Direct3D 11 setup/teardown code


#if SUPPORT_D3D11

static ID3D11Device* g_D3D11Device = NULL;
static ID3D11Buffer* g_D3D11VB = NULL; // vertex buffer
static ID3D11Buffer* g_D3D11CB = NULL; // constant buffer
static ID3D11VertexShader* g_D3D11VertexShader = NULL;
static ID3D11PixelShader* g_D3D11PixelShader = NULL;
static ID3D11InputLayout* g_D3D11InputLayout = NULL;
static ID3D11RasterizerState* g_D3D11RasterState = NULL;
static ID3D11BlendState* g_D3D11BlendState = NULL;
static ID3D11DepthStencilState* g_D3D11DepthState = NULL;

static D3D11_INPUT_ELEMENT_DESC s_DX11InputElementDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static bool EnsureD3D11ResourcesAreCreated()
{
	//if (g_D3D11VertexShader)
	//	return true;

	//// D3D11 has to load resources. Wait for Unity to provide the streaming assets path first.
	//if (s_UnityStreamingAssetsPath.empty())
	//	return false;

	//D3D11_BUFFER_DESC desc;
	//memset (&desc, 0, sizeof(desc));

	//// vertex buffer
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.ByteWidth = 1024;
	//desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//g_D3D11Device->CreateBuffer (&desc, NULL, &g_D3D11VB);

	//// constant buffer
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.ByteWidth = 64; // hold 1 matrix
	//desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//desc.CPUAccessFlags = 0;
	//g_D3D11Device->CreateBuffer (&desc, NULL, &g_D3D11CB);


	//HRESULT hr = -1;
	//Buffer vertexShader;
	//Buffer pixelShader;
	//std::string vertexShaderPath(s_UnityStreamingAssetsPath);
	//vertexShaderPath += "/Shaders/DX11_9_1/SimpleVertexShader.cso";
	//std::string fragmentShaderPath(s_UnityStreamingAssetsPath);
	//fragmentShaderPath += "/Shaders/DX11_9_1/SimplePixelShader.cso";
	//LoadFileIntoBuffer(vertexShaderPath, vertexShader);
	//LoadFileIntoBuffer(fragmentShaderPath, pixelShader);

	//if (vertexShader.size() > 0 && pixelShader.size() > 0)
	//{
	//	hr = g_D3D11Device->CreateVertexShader(&vertexShader[0], vertexShader.size(), nullptr, &g_D3D11VertexShader);
	//	if (FAILED(hr)) DebugLog("Failed to create vertex shader.\n");
	//	hr = g_D3D11Device->CreatePixelShader(&pixelShader[0], pixelShader.size(), nullptr, &g_D3D11PixelShader);
	//	if (FAILED(hr)) DebugLog("Failed to create pixel shader.\n");
	//}
	//else
	//{
	//	DebugLog("Failed to load vertex or pixel shader.\n");
	//}
	//// input layout
	//if (g_D3D11VertexShader && vertexShader.size() > 0)
	//{
	//	g_D3D11Device->CreateInputLayout (s_DX11InputElementDesc, 2, &vertexShader[0], vertexShader.size(), &g_D3D11InputLayout);
	//}	//// D3D11 has to load resources. Wait for Unity to provide the streaming assets path first.
	//if (s_UnityStreamingAssetsPath.empty())
	//	return false;

	//D3D11_BUFFER_DESC desc;
	//memset (&desc, 0, sizeof(desc));

	//// vertex buffer
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.ByteWidth = 1024;
	//desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//g_D3D11Device->CreateBuffer (&desc, NULL, &g_D3D11VB);

	//// constant buffer
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.ByteWidth = 64; // hold 1 matrix
	//desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//desc.CPUAccessFlags = 0;
	//g_D3D11Device->CreateBuffer (&desc, NULL, &g_D3D11CB);


	//HRESULT hr = -1;
	//Buffer vertexShader;
	//Buffer pixelShader;
	//std::string vertexShaderPath(s_UnityStreamingAssetsPath);
	//vertexShaderPath += "/Shaders/DX11_9_1/SimpleVertexShader.cso";
	//std::string fragmentShaderPath(s_UnityStreamingAssetsPath);
	//fragmentShaderPath += "/Shaders/DX11_9_1/SimplePixelShader.cso";
	//LoadFileIntoBuffer(vertexShaderPath, vertexShader);
	//LoadFileIntoBuffer(fragmentShaderPath, pixelShader);

	//if (vertexShader.size() > 0 && pixelShader.size() > 0)
	//{
	//	hr = g_D3D11Device->CreateVertexShader(&vertexShader[0], vertexShader.size(), nullptr, &g_D3D11VertexShader);
	//	if (FAILED(hr)) DebugLog("Failed to create vertex shader.\n");
	//	hr = g_D3D11Device->CreatePixelShader(&pixelShader[0], pixelShader.size(), nullptr, &g_D3D11PixelShader);
	//	if (FAILED(hr)) DebugLog("Failed to create pixel shader.\n");
	//}
	//else
	//{
	//	DebugLog("Failed to load vertex or pixel shader.\n");
	//}
	//// input layout
	//if (g_D3D11VertexShader && vertexShader.size() > 0)
	//{
	//	g_D3D11Device->CreateInputLayout (s_DX11InputElementDesc, 2, &vertexShader[0], vertexShader.size(), &g_D3D11InputLayout);
	//}

	// render states
	D3D11_RASTERIZER_DESC rsdesc;
	memset (&rsdesc, 0, sizeof(rsdesc));
	rsdesc.FillMode = D3D11_FILL_SOLID;
	rsdesc.CullMode = D3D11_CULL_NONE;
	rsdesc.DepthClipEnable = TRUE;
	g_D3D11Device->CreateRasterizerState (&rsdesc, &g_D3D11RasterState);

	D3D11_DEPTH_STENCIL_DESC dsdesc;
	memset (&dsdesc, 0, sizeof(dsdesc));
	dsdesc.DepthEnable = TRUE;
	dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	g_D3D11Device->CreateDepthStencilState (&dsdesc, &g_D3D11DepthState);

	D3D11_BLEND_DESC bdesc;
	memset (&bdesc, 0, sizeof(bdesc));
	bdesc.RenderTarget[0].BlendEnable = FALSE;
	bdesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
	g_D3D11Device->CreateBlendState (&bdesc, &g_D3D11BlendState);

	return true;
}

static void ReleaseD3D11Resources()
{
	SAFE_RELEASE(g_D3D11VB);
	SAFE_RELEASE(g_D3D11CB);
	SAFE_RELEASE(g_D3D11VertexShader);
	SAFE_RELEASE(g_D3D11PixelShader);
	SAFE_RELEASE(g_D3D11InputLayout);
	SAFE_RELEASE(g_D3D11RasterState);
	SAFE_RELEASE(g_D3D11BlendState);
	SAFE_RELEASE(g_D3D11DepthState);
}

static void DoEventGraphicsDeviceD3D11(UnityGfxDeviceEventType eventType)
{
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		IUnityGraphicsD3D11* d3d11 = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
		g_D3D11Device = d3d11->GetDevice();
		
		EnsureD3D11ResourcesAreCreated();
	}
	else if (eventType == kUnityGfxDeviceEventShutdown)
	{
		ReleaseD3D11Resources();
	}
}

#endif // #if SUPPORT_D3D11



// -------------------------------------------------------------------
// Direct3D 12 setup/teardown code


#if SUPPORT_D3D12
const UINT kNodeMask = 0;

static IUnityGraphicsD3D12* s_D3D12 = NULL;
static ID3D12Resource* s_D3D12Upload = NULL;

static ID3D12CommandAllocator* s_D3D12CmdAlloc = NULL;
static ID3D12GraphicsCommandList* s_D3D12CmdList = NULL;

static ID3D12Fence* s_D3D12Fence = NULL;
static UINT64 s_D3D12FenceValue = 1;
static HANDLE s_D3D12Event = NULL;

ID3D12Resource* GetD3D12UploadResource(UINT64 size)
{
	if (s_D3D12Upload)
	{
		D3D12_RESOURCE_DESC desc = s_D3D12Upload->GetDesc();
		if (desc.Width == size)
			return s_D3D12Upload;
		else
			s_D3D12Upload->Release();
	}

	// Texture upload buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = kNodeMask;
	heapProps.VisibleNodeMask = kNodeMask;

	D3D12_RESOURCE_DESC heapDesc = {};
	heapDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	heapDesc.Alignment = 0;
	heapDesc.Width = size;
	heapDesc.Height = 1;
	heapDesc.DepthOrArraySize = 1;
	heapDesc.MipLevels = 1;
	heapDesc.Format = DXGI_FORMAT_UNKNOWN;
	heapDesc.SampleDesc.Count = 1;
	heapDesc.SampleDesc.Quality = 0;
	heapDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	heapDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Device* device = s_D3D12->GetDevice();
	HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&heapDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&s_D3D12Upload));
	if (FAILED(hr)) DebugLog("Failed to CreateCommittedResource.\n");

	return s_D3D12Upload;
}

static void CreateD3D12Resources()
{
	ID3D12Device* device = s_D3D12->GetDevice();

	HRESULT hr = E_FAIL;

	// Command list
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_D3D12CmdAlloc));
	if (FAILED(hr)) DebugLog("Failed to CreateCommandAllocator.\n");
	hr = device->CreateCommandList(kNodeMask, D3D12_COMMAND_LIST_TYPE_DIRECT, s_D3D12CmdAlloc, nullptr, IID_PPV_ARGS(&s_D3D12CmdList));
	if (FAILED(hr)) DebugLog("Failed to CreateCommandList.\n");
	s_D3D12CmdList->Close();

	// Fence
	s_D3D12FenceValue = 1;
	device->CreateFence(s_D3D12FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_D3D12Fence));
	if (FAILED(hr)) DebugLog("Failed to CreateFence.\n");
	s_D3D12Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

static void ReleaseD3D12Resources()
{
	SAFE_RELEASE(s_D3D12Upload);
	
	if (s_D3D12Event)
		CloseHandle(s_D3D12Event);

	SAFE_RELEASE(s_D3D12Fence);
	SAFE_RELEASE(s_D3D12CmdList);
	SAFE_RELEASE(s_D3D12CmdAlloc);
}

static void DoEventGraphicsDeviceD3D12(UnityGfxDeviceEventType eventType)
{
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		s_D3D12 = s_UnityInterfaces->Get<IUnityGraphicsD3D12>();
		CreateD3D12Resources();
	}
	else if (eventType == kUnityGfxDeviceEventShutdown)
	{
		ReleaseD3D12Resources();
	}
}
#endif // #if SUPPORT_D3D12



// -------------------------------------------------------------------
// OpenGL ES / Core setup/teardown code


#if SUPPORT_OPENGL_UNIFIED

static void DoEventGraphicsDeviceGLUnified(UnityGfxDeviceEventType eventType)
{
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
    if (s_DeviceType == kUnityGfxRendererOpenGLES20 ||
        s_DeviceType == kUnityGfxRendererOpenGLES30)
    {
      ci::gl::Environment::setCore();
      ci::gl::env()->initializeFunctionPointers();
    }

#if SUPPORT_OPENGL_CORE
    if (s_DeviceType == kUnityGfxRendererOpenGLCore)
    {
      ci::gl::Environment::setCore();
      ci::gl::env()->initializeFunctionPointers();
    }
#endif

		assert(glGetError() == GL_NO_ERROR);
	}
	else if (eventType == kUnityGfxDeviceEventShutdown)
	{
		
	}
}
#endif



// --------------------------------------------------------------------------
// SetDefaultGraphicsState
//
// Helper function to setup some "sane" graphics state. Rendering state
// upon call into our plugin can be almost completely arbitrary depending
// on what was rendered in Unity before.
// Before calling into the plugin, Unity will set shaders to null,
// and will unbind most of "current" objects (e.g. VBOs in OpenGL case).
//
// Here, we set culling off, lighting off, alpha blend & test off, Z
// comparison to less equal, and Z writes off.

static void SetDefaultGraphicsState ()
{
	#if SUPPORT_D3D9
	// D3D9 case
	if (s_DeviceType == kUnityGfxRendererD3D9)
	{
		g_D3D9Device->SetRenderState (D3DRS_CULLMODE, D3DCULL_NONE);
		g_D3D9Device->SetRenderState (D3DRS_LIGHTING, FALSE);
		g_D3D9Device->SetRenderState (D3DRS_ALPHABLENDENABLE, FALSE);
		g_D3D9Device->SetRenderState (D3DRS_ALPHATESTENABLE, FALSE);
		g_D3D9Device->SetRenderState (D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		g_D3D9Device->SetRenderState (D3DRS_ZWRITEENABLE, FALSE);
	}
	#endif


	#if SUPPORT_D3D11
	// D3D11 case
	if (s_DeviceType == kUnityGfxRendererD3D11)
	{
		ID3D11DeviceContext* ctx = NULL;
		g_D3D11Device->GetImmediateContext (&ctx);
		ctx->OMSetDepthStencilState (g_D3D11DepthState, 0);
		ctx->RSSetState (g_D3D11RasterState);
		ctx->OMSetBlendState (g_D3D11BlendState, NULL, 0xFFFFFFFF);
		ctx->Release();
	}
	#endif


	#if SUPPORT_D3D12
	// D3D12 case
	if (s_DeviceType == kUnityGfxRendererD3D12)
	{
		// Stateless. Nothing to do.
	}
	#endif


	#if SUPPORT_OPENGL_LEGACY
	// OpenGL 2 legacy case (deprecated)
	if (s_DeviceType == kUnityGfxRendererOpenGL)
	{
		glDisable (GL_CULL_FACE);
		glDisable (GL_LIGHTING);
		glDisable (GL_BLEND);
		glDisable (GL_ALPHA_TEST);
		glDepthFunc (GL_LEQUAL);
		glEnable (GL_DEPTH_TEST);
		glDepthMask (GL_FALSE);
	}
	#endif
	
	
	#if SUPPORT_OPENGL_UNIFIED
	// OpenGL ES / core case
	if (s_DeviceType == kUnityGfxRendererOpenGLES20 ||
		  s_DeviceType == kUnityGfxRendererOpenGLES30 ||
		  s_DeviceType == kUnityGfxRendererOpenGLCore)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		assert(glGetError() == GL_NO_ERROR);
	}
	#endif
}


static void FillTextureFromCode (int width, int height, int stride, unsigned char* dst)
{
	const float t = g_Time * 4.0f;

	for (int y = 0; y < height; ++y)
	{
		unsigned char* ptr = dst;
		for (int x = 0; x < width; ++x)
		{
			// Simple oldskool "plasma effect", a bunch of combined sine waves
			int vv = int(
				(127.0f + (127.0f * sinf(x/7.0f+t))) +
				(127.0f + (127.0f * sinf(y/5.0f-t))) +
				(127.0f + (127.0f * sinf((x+y)/6.0f-t))) +
				(127.0f + (127.0f * sinf(sqrtf(float(x*x + y*y))/4.0f-t)))
				) / 4;

			// Write the texture pixel
			ptr[0] = vv;
			ptr[1] = vv;
			ptr[2] = vv;
			ptr[3] = vv;

			// To next pixel (our pixels are 4 bpp)
			ptr += 4;
		}

		// To next image row
		dst += stride;
	}
}


static void DoRendering (const float* worldMatrix, const float* identityMatrix, float* projectionMatrix, const MyVertex* verts)
{
	// Does actual rendering

  if (!g_Movie)
  {
    return;
  }

	#if SUPPORT_D3D9
	// D3D9 case
	if (s_DeviceType == kUnityGfxRendererD3D9)
	{
		// Transformation matrices
		g_D3D9Device->SetTransform (D3DTS_WORLD, (const D3DMATRIX*)worldMatrix);
		g_D3D9Device->SetTransform (D3DTS_VIEW, (const D3DMATRIX*)identityMatrix);
		g_D3D9Device->SetTransform (D3DTS_PROJECTION, (const D3DMATRIX*)projectionMatrix);

		// Vertex layout
		g_D3D9Device->SetFVF (D3DFVF_XYZ|D3DFVF_DIFFUSE);

		// Texture stage states to output vertex color
		g_D3D9Device->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		g_D3D9Device->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_CURRENT);
		g_D3D9Device->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		g_D3D9Device->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
		g_D3D9Device->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		g_D3D9Device->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		// Copy vertex data into our small dynamic vertex buffer. We could have used
		// DrawPrimitiveUP just fine as well.
		void* vbPtr;
		g_D3D9DynamicVB->Lock (0, 0, &vbPtr, D3DLOCK_DISCARD);
		memcpy (vbPtr, verts, sizeof(verts[0])*3);
		g_D3D9DynamicVB->Unlock ();
		g_D3D9Device->SetStreamSource (0, g_D3D9DynamicVB, 0, sizeof(MyVertex));

		// Draw!
		g_D3D9Device->DrawPrimitive (D3DPT_TRIANGLELIST, 0, 1);

		// Update native texture from code
		if (g_TexturePointer)
		{
			IDirect3DTexture9* d3dtex = (IDirect3DTexture9*)g_TexturePointer;
			D3DSURFACE_DESC desc;
			d3dtex->GetLevelDesc (0, &desc);
			D3DLOCKED_RECT lr;
			d3dtex->LockRect (0, &lr, NULL, 0);
			FillTextureFromCode (desc.Width, desc.Height, lr.Pitch, (unsigned char*)lr.pBits);
			d3dtex->UnlockRect (0);
		}
	}
	#endif


	#if SUPPORT_D3D11
	// D3D11 case
	if (s_DeviceType == kUnityGfxRendererD3D11 && EnsureD3D11ResourcesAreCreated())
	{
		ID3D11DeviceContext* ctx = NULL;
		g_D3D11Device->GetImmediateContext (&ctx);

		//// update constant buffer - just the world matrix in our case
		//ctx->UpdateSubresource (g_D3D11CB, 0, NULL, worldMatrix, 64, 0);

		//// set shaders
		//ctx->VSSetConstantBuffers (0, 1, &g_D3D11CB);
		//ctx->VSSetShader (g_D3D11VertexShader, NULL, 0);
		//ctx->PSSetShader (g_D3D11PixelShader, NULL, 0);

		//// update vertex buffer
		//ctx->UpdateSubresource (g_D3D11VB, 0, NULL, verts, sizeof(verts[0])*3, 0);

		//// set input assembler data and draw
		//ctx->IASetInputLayout (g_D3D11InputLayout);
		//ctx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//UINT stride = sizeof(MyVertex);
		//UINT offset = 0;
		//ctx->IASetVertexBuffers (0, 1, &g_D3D11VB, &stride, &offset);
		//ctx->Draw (3, 0);

		// update native texture from code
		if (g_TexturePointer)
		{
      auto textureUpdateFunc = [&](uint32_t width, uint32_t height, uint32_t dataLength, void* baseAddress)
      {
        ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)g_TexturePointer;
        D3D11_TEXTURE2D_DESC desc;
        d3dtex->GetDesc(&desc);

        if (width > desc.Width || height > desc.Height)
          return;

        D3D11_BOX destRegion;
        destRegion.left = 0;
        destRegion.right = width;
        destRegion.top = 0;
        destRegion.bottom = height;
        destRegion.front = 0;
        destRegion.back = 1;

        // FIXME: what is logical explanation for this value?
        auto rowPitch = width * 2;

        // Update subresource is bad!!!
        // ftp://download.nvidia.com/developer/cuda/seminar/TDCI_DX10perf_DX11preview.pdf
        // https://eatplayhate.me/2013/09/29/d3d11-texture-update-costs/

        ctx->UpdateSubresource(d3dtex, 0, &destRegion, baseAddress, rowPitch, 0);
      };

      // Update the movie texture
      g_Movie->updateTextureIfNeeded(textureUpdateFunc);
		}

		ctx->Release();
	}
	#endif



	#if SUPPORT_D3D12
	// D3D12 case
	if (s_DeviceType == kUnityGfxRendererD3D12)
	{
		ID3D12Device* device = s_D3D12->GetDevice();
		ID3D12CommandQueue* queue = s_D3D12->GetCommandQueue();

		// Wait on the previous job (example only - simplifies resource management)
		if (s_D3D12Fence->GetCompletedValue() < s_D3D12FenceValue)
		{
			s_D3D12Fence->SetEventOnCompletion(s_D3D12FenceValue, s_D3D12Event);
			WaitForSingleObject(s_D3D12Event, INFINITE);
		}
		
		// Update native texture from code
		if (g_TexturePointer)
		{
			ID3D12Resource* resource = (ID3D12Resource*)g_TexturePointer;
			D3D12_RESOURCE_DESC desc = resource->GetDesc();

			// Begin a command list
			s_D3D12CmdAlloc->Reset();
			s_D3D12CmdList->Reset(s_D3D12CmdAlloc, nullptr);

			// Fill data
			const UINT64 kDataSize = desc.Width*desc.Height*4;
			ID3D12Resource* upload = GetD3D12UploadResource(kDataSize);
			void* mapped = NULL;
			upload->Map(0, NULL, &mapped);
			FillTextureFromCode(desc.Width, desc.Height, desc.Width*4, (unsigned char*)mapped);
			upload->Unmap(0, NULL);

			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.pResource = upload;
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			device->GetCopyableFootprints(&desc, 0, 1, 0, &srcLoc.PlacedFootprint, nullptr, nullptr, nullptr);

			D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
			dstLoc.pResource = resource;
			dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLoc.SubresourceIndex = 0;

			// Queue data upload
			const D3D12_RESOURCE_STATES kDesiredState = D3D12_RESOURCE_STATE_COPY_DEST;
			D3D12_RESOURCE_STATES resourceState = kDesiredState;
			s_D3D12->GetResourceState(resource, &resourceState); // Get resource state as it will be after all command lists Unity queued before this plugin call execute.
			if (resourceState != kDesiredState)
			{
				D3D12_RESOURCE_BARRIER barrierDesc = {};
				barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierDesc.Transition.pResource = resource;
				barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				barrierDesc.Transition.StateBefore = resourceState;
				barrierDesc.Transition.StateAfter = kDesiredState;
				s_D3D12CmdList->ResourceBarrier(1, &barrierDesc);
				s_D3D12->SetResourceState(resource, kDesiredState); // Set resource state as it will be after this command list is executed.
			}

			s_D3D12CmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

			// Execute the command list
			s_D3D12CmdList->Close();
			ID3D12CommandList* lists[1] = { s_D3D12CmdList };
			queue->ExecuteCommandLists(1, lists);
		}

		// Insert fence
		++s_D3D12FenceValue;
		queue->Signal(s_D3D12Fence, s_D3D12FenceValue);
	}
	#endif


	#if SUPPORT_OPENGL_LEGACY
	// OpenGL 2 legacy case (deprecated)
  if (s_DeviceType == kUnityGfxRendererOpenGL)
  {
  }
	#endif
	
	
	#if SUPPORT_OPENGL_UNIFIED
	// OpenGL ES / core case
	if (s_DeviceType == kUnityGfxRendererOpenGLES20 ||
		  s_DeviceType == kUnityGfxRendererOpenGLES30 ||
		  s_DeviceType == kUnityGfxRendererOpenGLCore)
	{
		assert(glGetError() == GL_NO_ERROR); // Make sure no OpenGL error happen before starting rendering

    auto textureUpdateFunc = [&](int width, int height, int dataLength, void* baseAddress)
    {
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

      GLuint textureId = (GLuint)(size_t)(g_TexturePointer);
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
                                width,
                                height,
                                GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                                dataLength,
                                baseAddress);
      error = getErrorString(glGetError());

      //glBindTexture(GL_TEXTURE_2D, 0);
      //glDisable(GL_TEXTURE_2D);
    };

    g_Movie->updateTextureIfNeeded(textureUpdateFunc);

		assert(glGetError() == GL_NO_ERROR);
	}
	#endif
}
