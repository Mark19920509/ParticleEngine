//--------------------------------------------------------------------------------------
// File: Tutorial01.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include <windows.h>
#include <DX11Renderer.h>
#include <DX11Sound.h>

#include "Camera.h"
#include "BaseDemo.h"
#include "Demos/WaterDemo.h"
#include "Demos/TestGrid.h"
#include "Demos/ClothDemo.h"
#include "Demos/GalaxyDemo.h"
#include "Demos/RainDemo.h"
#include "Demos/Animation.h"
#include "Demos/SimulationsTransition.h"
#include "Utility/Timer.h"
#include "Utility/Utility.h"
 


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );


short mouseWheelDelta = 0;
int initMouseX, initMouseY, iMouseX, iMouseY;
bool mouseWheelIsPressed = false;

bool isPaused = false;
bool isStepping = false;
bool leaveProgram = false;

BaseDemo *      baseDemo = NULL;
DX11Renderer    renderer;
DX11Sound       sound;
Camera          camera;

unsigned int __stdcall PlayASound(DX11Sound* sound)
{
    sound->Initialize(g_hWnd);
 
    _endthreadex(0);
    return 0;
}


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    
    // Time
    LARGE_INTEGER startTime;
    QueryPerformanceCounter(&startTime);

    // Set camera before demo not to overide demo camera
    camera.setPointToFollow(slmath::vec3(0.0f, 0.0f, 0.0f));
    camera.setDistanceFromTarget(100.0f);

    // Init demo
    Timer::GetInstance()->StartTimerProfile();
    baseDemo = new WaterDemo;
    baseDemo->SetCamera(&camera);
    baseDemo->Initialize();
    Timer::GetInstance()->StopTimerProfile("Init demo");


    Timer::GetInstance()->StartTimerProfile();
    if( FAILED( renderer.Initialize(g_hWnd) ) )
    {
        renderer.Release();
        return 0;
	}
    renderer.BeginFrame();
    renderer.EndFrame();
    baseDemo->InitializeRenderer(renderer);
    Timer::GetInstance()->StopTimerProfile("Init rendering");

    slmath::mat4 viewMatrix;
    
    slmath::mat4 projection = slmath::perspectiveFovLH( 3.1415926f / 2.0f, 1024.0f / 768.0f, -5.0f, 1.0f );

    // Launch music
    _beginthreadex( NULL, 0, (unsigned int (__stdcall *)(void *))(&PlayASound), &sound, 0, 0);


    // Start a global timer that will be always present
    Timer::GetInstance()->StartTimer();
    // Start timer for FPS and frame duration computation
    Timer::GetInstance()->StartTimer();

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message && leaveProgram == false )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            // Move mouse 
            if (mouseWheelDelta > 0)
            {
                camera.zoomIn(4000.0f * 1/60.0f * float(mouseWheelDelta / 120) * 0.1f);
            }
            else if (mouseWheelDelta < 0)
            {
                camera.zoomOut(4000.0f * 1/60.0f * float(-mouseWheelDelta / 120) * 0.1f);
            }

            if (mouseWheelIsPressed && initMouseX > iMouseX)
            {
                camera.moveRight(1/60.0f * float(initMouseX - iMouseX) * 0.1f );
            }
            else if (mouseWheelIsPressed && initMouseX < iMouseX)
            {
                camera.moveLeft(1/60.0f * float(iMouseX - initMouseX) * 0.1f );
            }

            if (mouseWheelIsPressed && initMouseY > iMouseY)
            {
                camera.moveUp(1/60.0f * float(initMouseY - iMouseY) * 0.1f );
            }
            else if (mouseWheelIsPressed && initMouseY < iMouseY)
            {
                camera.moveDown(1/60.0f * float(iMouseY - initMouseY) * 0.1f );
            }

            camera.move();
            mouseWheelDelta = 0;
            initMouseX = iMouseX;
            initMouseY = iMouseY;
            viewMatrix = camera.getViewMatrix();

            
	        
            if (!isPaused || isStepping)
            {
                // Calculate frame duration 
                float frameDuration = Timer::GetInstance()->StopTimer();

                
                const float fastestFrameDuration = (1.0f / 60.0f);

                float timeToSleep = slmath::max(fastestFrameDuration - frameDuration, 0.0f);
                Timer::GetInstance()->StartTimer();
                if (timeToSleep > 0.0f)
                {
                    Sleep( DWORD(timeToSleep * 1000.0f));
                }
                // float sleepingTime = Timer::GetInstance()->StopTimer();
                // else
                {
                    float fps = 1.0f / frameDuration;
                    DEBUG_OUT("FPS :\t" << fps << "\tFrame duration:\t" << frameDuration /*<<  "Sleeping time: "<< sleepingTime << "Time to sleep : "<< timeToSleep*/ <<"\n");
                }
                // Start loop timer
                Timer::GetInstance()->StartTimer();

                baseDemo->Simulate(frameDuration + timeToSleep);
                isStepping = false;
            }

            if (!isPaused)
            {
                Timer::GetInstance()->StartTimerProfile();
            }
            
            slmath::vec4 vec4CameraPosition = camera.getPosition();
            
            D3DXVECTOR4 cameraPosition = *reinterpret_cast<D3DXVECTOR4*>(&vec4CameraPosition);
            
            slmath::vec4 lightPosition = baseDemo->GetLightPosition();
            D3DXVECTOR4 dxLightPosition = *reinterpret_cast<D3DXVECTOR4*>(&lightPosition);

            renderer.SetViewAndProjMatrix(*(D3DXMATRIX*)&viewMatrix, *(D3DXMATRIX*)&projection, cameraPosition, dxLightPosition);
            renderer.BeginFrame();
            baseDemo->Draw(renderer);
            renderer.EndFrame();
            if (!isPaused)
            {
                Timer::GetInstance()->StopTimerProfile("Renderer");
            }

        }
    }

    renderer.Release();
    sound.Shutdown();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"ClassName";
    wcex.hIconSm = NULL;
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 1024, 768};
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"ClassName", L"Particle Engine", WS_EX_TOPMOST | WS_POPUP/*WS_OVERLAPPEDWINDOW*/,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

void InitNewDemo(BaseDemo *newDemo)
{
    
    baseDemo->Release();
    delete baseDemo;
    baseDemo = newDemo;
    Timer::GetInstance()->StartTimerProfile();
    baseDemo->SetCamera(&camera);
    baseDemo->Initialize();
    Timer::GetInstance()->StopTimerProfile("Init demo");
    Timer::GetInstance()->StartTimerProfile();
    renderer.Release();
    if( FAILED( renderer.Initialize(g_hWnd) ) )
    {
        renderer.Release();
        assert(0);
	}
    baseDemo->InitializeRenderer(renderer);
    baseDemo->SetCamera(&camera);
    Timer::GetInstance()->StopTimerProfile("Init rendering");
}

void IputKey(WPARAM wParam)
{
    switch (wParam)
    {
    case 0x52: // R
        baseDemo->Release();
        baseDemo->Initialize();
        baseDemo->InitializeOpenClData();
        break;

    case 0x50: // P
        isPaused  = ! isPaused;
        break;

    case 0x53: // S
        isStepping = true;
        break;

    case 0x54: // T
        renderer.SetIsDisplayingParticles(!renderer.IsDisplayingParticles());
        break;

    case 0x43: // C
        camera.setPointToFollow(slmath::vec3(0.0f, 0.0f, 0.0f));
        camera.setDistanceFromTarget(300.0f);
        camera.setAngleHeight(0.0f);
        camera.setAngleHorizon(0.0f);
        break;

    case VK_NUMPAD1:
        InitNewDemo(new WaterDemo);
        break;
    case VK_NUMPAD2:
        InitNewDemo(new TestGrid);
        break;
    case VK_NUMPAD3:
        InitNewDemo(new ClothDemo);
        break;
    case VK_NUMPAD4:
        InitNewDemo(new GalaxyDemo);
        break;
    case VK_NUMPAD5:
        InitNewDemo(new RainDemo);
        break;
    case VK_NUMPAD6:
        InitNewDemo(new Animation);
        break;
    case VK_NUMPAD7:
        InitNewDemo(new SimulationsTransition);
        break;
    case VK_ESCAPE:
        leaveProgram = true;
        break;
    }

    baseDemo->IputKey(wParam);
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        case WM_MOUSEWHEEL:
            // Update member var state
            mouseWheelDelta += ( short )HIWORD( wParam );
            break;

        case WM_MBUTTONUP:
           // ReleaseCapture();
            mouseWheelIsPressed = false;
            break;

        case WM_MBUTTONDOWN:
            mouseWheelIsPressed = true;
            break;
        case WM_MOUSEMOVE:
            iMouseX = ( int )( short )LOWORD( lParam );
            iMouseY = ( int )( short )HIWORD( lParam );
        break;

        case WM_KEYDOWN:
            IputKey(wParam);
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}
