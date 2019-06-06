
#include "DX11Renderer.h"
#include "D3Dcompiler.h"
#include <slmath/slmath.h>


DX11Renderer::DX11Renderer():	m_driverType		( D3D_DRIVER_TYPE_NULL  ),
								m_featureLevel		( D3D_FEATURE_LEVEL_11_0),
								m_pd3dDevice		( NULL					),
								m_pImmediateContext ( NULL					),
								m_pSwapChain		( NULL					),
								m_pRenderTargetView ( NULL					),
                                m_RasterState       ( NULL					),
                                m_RasterStateDoubleFaces( NULL				),
								// Shaders
                                m_pRenderParticlesVS( NULL					),
								m_pRenderParticlesGS( NULL					),
								m_pRenderParticlesPS( NULL					),
                                m_pRenderSphereVS   ( NULL					),
                                m_pRenderSphereGS   ( NULL					),
                                m_pRenderSpherePS   ( NULL					),
                                m_pRenderMeshVS     ( NULL					),
                                m_pRenderMeshHS     ( NULL					),
                                m_pRenderMeshDS     ( NULL					),
                                m_pRenderMeshPS     ( NULL					),
                                m_pRenderVS         ( NULL					),
                                m_pRenderPS         ( NULL					),
								// Layout and textures
                                m_pParticleVertexLayout( NULL				),
                                m_pPatchLayout      ( NULL				),
                                m_pNormalLayout     ( NULL				),
                                m_pParticlePositionBuffer( NULL					),
                                m_pSphereBuffer          ( NULL					),
                                m_pParticleNormalBuffer( NULL					),
                                m_pIndexBuffer      ( NULL					),
								m_pConstantBuffer(NULL),
								m_pParticleTexRV(NULL),
                                m_pSphereTexRV(NULL),
								m_pSampleStateLinear(NULL),
								
                                m_pDepthStencilState(NULL),
                                m_pDepthStencilStateParticle(NULL),
                                m_pDepthStencilView(NULL),
                                m_pBlendingStateParticle(NULL),
                                
                                m_Particles(NULL),
                                m_Spheres(NULL),
                                m_PositionNormals(NULL),
                                m_ParticlesCount(0),
                                m_ParticlesAllocatedCount(0),
                                m_SpheresCount(0),
                                m_SpheresAllocatedCount(0),
                                m_ClothSide(0),
                                m_IndicesCount(0),
                                m_DisplayParticles(true),
                                m_IsUsingInteroperability(true)

{
}

DX11Renderer::~DX11Renderer()
{
}

HRESULT DX11Renderer::Initialize(const HWND &m_hWnd)
{
	HRESULT hr = InitDevice(m_hWnd);
    // Initialize the data in the buffers
	hr &= InitParticleShaders();
	return hr;
}

void DX11Renderer::InitializeParticles(int particlesCount, int spheresCount, int clothParticlesCount)
{
    m_ParticlesAllocatedCount = m_ParticlesCount = particlesCount;

    m_SpheresAllocatedCount = m_SpheresCount = spheresCount;
   
    m_ClothParticlesAllocatedCount = m_ClothParticlesCount = clothParticlesCount;


    // Create Particles
    HRESULT hr = CreateBuffers();
    CheckResult(hr);
}

void DX11Renderer::SetIsDisplayingParticles(bool displayParticles)
{
    m_DisplayParticles = displayParticles;
}

bool DX11Renderer::IsDisplayingParticles()const
{
    return m_DisplayParticles;
}

void DX11Renderer::SetIsUsingInteroperability(bool isUsingInteroperability)
{
    m_IsUsingInteroperability = isUsingInteroperability;
}

bool DX11Renderer::IsUsingInteroperability() const
{
    return m_IsUsingInteroperability;
}


ID3D11Device* DX11Renderer::GetID3D11Device()const
{
    return m_pd3dDevice;
}

ID3D11Buffer* DX11Renderer::GetID3D11Buffer() const
{
    return m_pParticlePositionBuffer;
}


// Create Direct3D device and swap chain
HRESULT DX11Renderer::InitDevice(const HWND &m_hWnd)
{

    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( m_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
//#ifdef DEBUG
//    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = m_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

   m_pImmediateContext->OMSetRenderTargets( 1, &m_pRenderTargetView, NULL );
   
    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    ID3D11Texture2D*                    depthStencil;
    hr = m_pd3dDevice->CreateTexture2D( &descDepth, NULL, &depthStencil );
    if( FAILED( hr ) )
        return hr;

     // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView( depthStencil, &descDSV, &m_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    m_pImmediateContext->OMSetRenderTargets( 1, &m_pRenderTargetView, m_pDepthStencilView );
    depthStencil->Release();


    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pImmediateContext->RSSetViewports( 1, &vp );

   D3D11_RASTERIZER_DESC rasterDesc;

   // Setup the raster description for one face displayed
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode =  D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID; // ;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	hr = m_pd3dDevice->CreateRasterizerState(&rasterDesc, &m_RasterState);
	if(FAILED(hr))
	{
		return hr;
	}

    D3D11_RASTERIZER_DESC rasterDescDoubleFaces;

   // Setup the raster description for two faces displayed
	rasterDescDoubleFaces.AntialiasedLineEnable = false;
	rasterDescDoubleFaces.CullMode =  D3D11_CULL_NONE;
	rasterDescDoubleFaces.DepthBias = 0;
	rasterDescDoubleFaces.DepthBiasClamp = 0.0f;
	rasterDescDoubleFaces.DepthClipEnable = true;
	rasterDescDoubleFaces.FillMode = D3D11_FILL_SOLID; //D3D11_FILL_WIREFRAME;
	rasterDescDoubleFaces.FrontCounterClockwise = false;
	rasterDescDoubleFaces.MultisampleEnable = false;
	rasterDescDoubleFaces.ScissorEnable = false;
	rasterDescDoubleFaces.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	hr = m_pd3dDevice->CreateRasterizerState(&rasterDescDoubleFaces, &m_RasterStateDoubleFaces);
	if(FAILED(hr))
	{
		return hr;
	}

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT DX11Renderer::CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines /*= NULL*/ )
{
    HRESULT hr = S_OK;
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, pDefines, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
	    //CheckResult(hr);
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
		{
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
			pErrorBlob->Release();
		}
        return hr;
    }
    pErrorBlob->Release();

    return hr;
}

HRESULT DX11Renderer::InitParticleShaders()
{

    // Particles
	ID3DBlob* pBlobRenderParticlesVS = NULL;
    ID3DBlob* pBlobRenderParticlesGS = NULL;
    ID3DBlob* pBlobRenderParticlesPS = NULL;

	HRESULT hr = S_OK;

	hr = CompileShaderFromFile( L"shaders\\ParticleDraw.hlsl", "VSParticleDraw", "vs_4_0", &pBlobRenderParticlesVS );
	CheckResult(hr);
	hr = CompileShaderFromFile( L"shaders\\ParticleDraw.hlsl", "GSParticleDraw", "gs_4_0", &pBlobRenderParticlesGS );
	CheckResult(hr);
	hr = CompileShaderFromFile( L"shaders\\ParticleDraw.hlsl", "PSParticleDraw", "ps_4_0", &pBlobRenderParticlesPS );
	CheckResult(hr);

    hr = m_pd3dDevice->CreateVertexShader( pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), NULL, &m_pRenderParticlesVS );
	CheckResult(hr);
	hr = m_pd3dDevice->CreateGeometryShader( pBlobRenderParticlesGS->GetBufferPointer(), pBlobRenderParticlesGS->GetBufferSize(), NULL, &m_pRenderParticlesGS );
	CheckResult(hr);
	hr = m_pd3dDevice->CreatePixelShader( pBlobRenderParticlesPS->GetBufferPointer(), pBlobRenderParticlesPS->GetBufferSize(), NULL, &m_pRenderParticlesPS );
	CheckResult(hr);

    // Sphere
    ID3DBlob* pBlobRenderSphereVS = NULL;
    ID3DBlob* pBlobRenderSphereGS = NULL;
    ID3DBlob* pBlobRenderSpherePS = NULL;

    hr = CompileShaderFromFile( L"shaders\\SphereDraw.hlsl", "VSSphereDraw", "vs_4_0", &pBlobRenderSphereVS );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\SphereDraw.hlsl", "GSSphereDraw", "gs_4_0", &pBlobRenderSphereGS );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\SphereDraw.hlsl", "PSSphereDraw", "ps_4_0", &pBlobRenderSpherePS );
	CheckResult(hr);

    hr = m_pd3dDevice->CreateVertexShader( pBlobRenderSphereVS->GetBufferPointer(), pBlobRenderSphereVS->GetBufferSize(), NULL, &m_pRenderSphereVS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreateGeometryShader( pBlobRenderSphereGS->GetBufferPointer(), pBlobRenderSphereGS->GetBufferSize(), NULL, &m_pRenderSphereGS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreatePixelShader( pBlobRenderSpherePS->GetBufferPointer(), pBlobRenderSpherePS->GetBufferSize(), NULL, &m_pRenderSpherePS );
	CheckResult(hr);

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
    hr = m_pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
        pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &m_pParticleVertexLayout );
	CheckResult(hr);

    hr = m_pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
        pBlobRenderSphereVS->GetBufferPointer(), pBlobRenderSphereVS->GetBufferSize(), &m_pParticleVertexLayout );
	CheckResult(hr);

	pBlobRenderParticlesVS->Release();
	pBlobRenderParticlesGS->Release();
	pBlobRenderParticlesPS->Release();

    pBlobRenderSphereVS->Release();
    pBlobRenderSphereGS->Release();
    pBlobRenderSpherePS->Release();

    // Mesh on hull and domain shader
    ID3DBlob* pBlobRenderMeshVS = NULL;
    ID3DBlob* pBlobRenderMeshHS = NULL;
    ID3DBlob* pBlobRenderMeshDS = NULL;
    ID3DBlob* pBlobRenderMeshPS = NULL;

    D3D_SHADER_MACRO integerPartitioning[] = { { "BEZIER_HS_PARTITION", "\"integer\"" }, { 0 } };

    hr = CompileShaderFromFile( L"shaders\\MeshGenerate.hlsl", "TesselateVS", "vs_5_0", &pBlobRenderMeshVS );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\MeshGenerate.hlsl", "TesselateHS", "hs_5_0", &pBlobRenderMeshHS, integerPartitioning );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\MeshGenerate.hlsl", "TesselateDS", "ds_5_0", &pBlobRenderMeshDS );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\MeshGenerate.hlsl", "TesselatePS", "ps_5_0", &pBlobRenderMeshPS );
	CheckResult(hr);

    hr = m_pd3dDevice->CreateVertexShader( pBlobRenderMeshVS->GetBufferPointer(), pBlobRenderMeshVS->GetBufferSize(), NULL, &m_pRenderMeshVS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreateHullShader( pBlobRenderMeshHS->GetBufferPointer(), pBlobRenderMeshHS->GetBufferSize(), NULL, &m_pRenderMeshHS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreateDomainShader( pBlobRenderMeshDS->GetBufferPointer(), pBlobRenderMeshDS->GetBufferSize(), NULL, &m_pRenderMeshDS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreatePixelShader( pBlobRenderMeshPS->GetBufferPointer(), pBlobRenderMeshPS->GetBufferSize(), NULL, &m_pRenderMeshPS );
	CheckResult(hr);

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_pd3dDevice->CreateInputLayout( patchlayout, ARRAYSIZE( patchlayout ), pBlobRenderMeshVS->GetBufferPointer(),
                                             pBlobRenderMeshVS->GetBufferSize(), &m_pPatchLayout );
	CheckResult(hr);


    pBlobRenderMeshVS->Release();
    pBlobRenderMeshHS->Release();
    pBlobRenderMeshDS->Release();
    pBlobRenderMeshPS->Release();

    // Draw any simple mesh

    ID3DBlob* pBlobRenderVS = NULL;
    ID3DBlob* pBlobRenderPS = NULL;

     hr = CompileShaderFromFile( L"shaders\\Draw.hlsl", "VSDraw", "vs_4_0", &pBlobRenderVS );
	CheckResult(hr);
    hr = CompileShaderFromFile( L"shaders\\Draw.hlsl", "PSDraw", "ps_4_0", &pBlobRenderPS );
	CheckResult(hr);

     hr = m_pd3dDevice->CreateVertexShader( pBlobRenderVS->GetBufferPointer(), pBlobRenderVS->GetBufferSize(), NULL, &m_pRenderVS );
	CheckResult(hr);
    hr = m_pd3dDevice->CreatePixelShader( pBlobRenderPS->GetBufferPointer(), pBlobRenderPS->GetBufferSize(), NULL, &m_pRenderPS );
	CheckResult(hr);

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC normalLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = m_pd3dDevice->CreateInputLayout( normalLayout, ARRAYSIZE( normalLayout ), pBlobRenderVS->GetBufferPointer(),
                                             pBlobRenderVS->GetBufferSize(), &m_pNormalLayout );
	CheckResult(hr);

    pBlobRenderPS->Release();
    pBlobRenderVS->Release();


	// Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( ConstantBufferData );
    hr = m_pd3dDevice->CreateBuffer( &Desc, NULL, &m_pConstantBuffer );
	CheckResult(hr);


	// Load the Particle Texture
    WCHAR *str1 = L"Asset\\particleGrey.dds";
    hr = D3DX11CreateShaderResourceViewFromFile( m_pd3dDevice, str1, NULL, NULL, &m_pParticleTexRV, NULL );
    CheckResult(hr);

    WCHAR *str2 = L"Asset\\planet.dds";
    hr = D3DX11CreateShaderResourceViewFromFile( m_pd3dDevice, str2, NULL, NULL, &m_pSphereTexRV, NULL );
    CheckResult(hr);

    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    hr = m_pd3dDevice->CreateSamplerState( &SamplerDesc, &m_pSampleStateLinear );
	CheckResult(hr);


    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
    BlendStateDesc.AlphaToCoverageEnable = FALSE;
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
    hr = m_pd3dDevice->CreateBlendState( &BlendStateDesc, &m_pBlendingStateParticle );
	CheckResult(hr);

    // Depth Stencil for current objects
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory( &depthStencilDesc, sizeof(depthStencilDesc) );

    // Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    hr = m_pd3dDevice->CreateDepthStencilState( &depthStencilDesc, &m_pDepthStencilState );


    // Depth Stencil for particles
    ZeroMemory( &depthStencilDesc, sizeof(depthStencilDesc) );

    // Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    hr = m_pd3dDevice->CreateDepthStencilState( &depthStencilDesc, &m_pDepthStencilStateParticle );

	CheckResult(hr);
    

    return hr;
}

HRESULT DX11Renderer::CreateBuffers()
{
    HRESULT hr = S_OK;
    if (m_ParticlesAllocatedCount > 0)
    {
        delete [] m_Particles;
        m_Particles = new Position[m_ParticlesAllocatedCount];

        hr = CreateParticleBuffer();
    }
    if (m_SpheresAllocatedCount > 0)
    {
        delete [] m_Spheres;
        m_Spheres = new Position[m_SpheresAllocatedCount];

        hr = CreateSpheresBuffer();
    }
    if (m_ClothParticlesAllocatedCount > 0)
    {
        delete [] m_PositionNormals;
        m_PositionNormals = new PositionNormal[m_ClothParticlesAllocatedCount];
        hr = CreateMeshBuffer();
    }
    return hr;
}

HRESULT DX11Renderer::CreateParticleBuffer()
{
    HRESULT hr = S_OK;
 
    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.Usage = D3D11_USAGE_DEFAULT;

    if (m_IsUsingInteroperability)
    {
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    }
    else
    {
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.MiscFlags = 0;
    }
    desc.CPUAccessFlags = 0;
    desc.ByteWidth =  m_ParticlesAllocatedCount *  sizeof(Position);
    
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = m_Particles;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;


    hr = m_pd3dDevice->CreateBuffer( &desc, NULL, &m_pParticlePositionBuffer );
    CheckResult(hr);

    return hr;
}

HRESULT DX11Renderer::CreateSpheresBuffer()
{
    HRESULT hr = S_OK;
 
    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.ByteWidth =  m_SpheresAllocatedCount *  sizeof(Position);
    
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = m_Spheres;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;


    hr = m_pd3dDevice->CreateBuffer( &desc, NULL, &m_pSphereBuffer );
    CheckResult(hr);

    return hr;
}

HRESULT DX11Renderer::CreateMeshBuffer()
{
    HRESULT hr = S_OK;
    D3D11_BUFFER_DESC descNormal;
    ZeroMemory( &descNormal, sizeof(descNormal) );
    descNormal.Usage = D3D11_USAGE_DEFAULT;
    descNormal.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    descNormal.CPUAccessFlags = 0;
    descNormal.ByteWidth =  m_ClothParticlesAllocatedCount *  sizeof(PositionNormal);
    descNormal.MiscFlags = 0;
    descNormal.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA InitDataNormal;
   InitDataNormal.pSysMem = m_PositionNormals;
   InitDataNormal.SysMemPitch = 0;
   InitDataNormal.SysMemSlicePitch = 0;


    hr = m_pd3dDevice->CreateBuffer( &descNormal, &InitDataNormal, &m_pParticleNormalBuffer );
    CheckResult(hr);

    return hr;
}


void DX11Renderer::CreateIndexBuffersForCloth(unsigned int clothSide)
{
    assert(clothSide > 0);

    m_ClothSide  = clothSide;
    m_IndicesCount = 6 * (m_ClothSide - 1) * (m_ClothSide - 1);

    // Create indices.    
    unsigned int *indices = new unsigned int[m_IndicesCount];
    int index = 0;
    for (unsigned int i = 0; i < m_ClothSide - 1; i++)
    {
        for (unsigned int j = 0; j < m_ClothSide - 1; j++)
        {
            // First triangle
            indices[index++] = i + j * m_ClothSide;
            indices[index++] = i + (j + 1) * m_ClothSide;
            indices[index++] = i + 1 + (j + 1) * m_ClothSide;

            // Second one
            indices[index++] = i + j * m_ClothSide;
            indices[index++] = i + 1 + (j + 1) * m_ClothSide;
            indices[index++] = i + 1 + j * m_ClothSide;
        }
    }


    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.ByteWidth =  m_IndicesCount *  sizeof(unsigned int);
    desc.MiscFlags = 0;
    desc.StructureByteStride = sizeof(unsigned int);

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = indices;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;


    hr = m_pd3dDevice->CreateBuffer( &desc, &InitData, &m_pIndexBuffer );
    CheckResult(hr);

    // Set the buffer.
    m_pImmediateContext->IASetIndexBuffer( m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

    delete[] indices;

    CheckResult(hr);
}

void DX11Renderer::BeginFrame()
{
    // Just clear the backbuffer
    float ClearColor[4] = { 0.0f, 0.0f, 0.1f, 1.0f }; //red,green,blue,alpha
    m_pImmediateContext->ClearRenderTargetView( m_pRenderTargetView, ClearColor );
    m_pImmediateContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
}

void DX11Renderer::EndFrame()
{
    m_pSwapChain->Present( 0, 0 );
}

void DX11Renderer::CopyParticlesButHardCodeW(slmath::vec4 * particlePositions, int particlesCount)
{
    assert(particlesCount <= m_ParticlesAllocatedCount);
    m_ParticlesCount = particlesCount;
    
    // Copy particles in the buffer
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_Particles[i].m_Position = D3DXVECTOR4(particlePositions[i].x, particlePositions[i].y, particlePositions[i].z, 0.506f);
    }
}

void DX11Renderer::CopyParticles(slmath::vec4 * particlePositions, int particlesCount)
{
    assert(particlesCount <= m_ParticlesAllocatedCount);
    m_ParticlesCount = particlesCount;
    
    // Copy particles in the buffer
    for (int i = 0; i < m_ParticlesCount; i++)
    {
        m_Particles[i].m_Position = D3DXVECTOR4(particlePositions[i].x, particlePositions[i].y, particlePositions[i].z, particlePositions[i].w);
    }
}

void DX11Renderer::CopySpheres(slmath::vec4 * particlePositions, int particlesCount)
{
    assert(particlesCount <= m_SpheresAllocatedCount);
    m_SpheresCount = particlesCount;
    
    // Copy spheres in the buffer
    for (int i = 0; i < m_SpheresCount; i++)
    {
        m_Spheres[i].m_Position = D3DXVECTOR4(particlePositions[i].x, particlePositions[i].y, particlePositions[i].z, particlePositions[i].w);
    }
}

// Render Particles
void DX11Renderer::Render(slmath::vec4 * particlePositions, int particlesCount, bool useDefaultColor /*= false*/ )
{
    if (! m_IsUsingInteroperability)
    {
        if (useDefaultColor)
        {
            CopyParticlesButHardCodeW(particlePositions, particlesCount);
        }
        else
        {
            CopyParticles(particlePositions, particlesCount);
        }
    }

   // Render the particles
    if (m_DisplayParticles)
    {
        RenderParticles();
    }
    else
    {
        RenderMeshOnGPU();
    }
}

void DX11Renderer::RenderSphere(slmath::vec4 * particlePositions, int particlesCount)
{
    CopySpheres(particlePositions, particlesCount);
    const bool isRenderingSphere = true;
    RenderMeshOnGPU(isRenderingSphere);
}


void DX11Renderer::RenderCloth(const slmath::vec4 * particlePositions, int particlesCount)
{

    assert(particlesCount <= m_ClothParticlesAllocatedCount);
    m_ClothParticlesCount = particlesCount;

    
    for (int i = 0; i < m_ClothParticlesCount; i++)
    {
        m_PositionNormals[i].m_Position = D3DXVECTOR4(particlePositions[i].x, particlePositions[i].y, particlePositions[i].z, 0.0f);
        m_PositionNormals[i].m_Normal = D3DXVECTOR4(0.0f, 0.0f, -1.0f, 0.0f);
    }

    for (unsigned int i = 0; i < m_ClothSide - 1; i++)
    {
        for (unsigned int j = 0; j < m_ClothSide - 1; j++)
        {
            int indexVertex = i + j * m_ClothSide;

            // First triangle
            int index1 = i + j * m_ClothSide;
            int index2 = i + (j + 1) * m_ClothSide;
            int index3 = i + 1 + j * m_ClothSide;


            D3DXVECTOR4 v1 = m_PositionNormals[index2].m_Position - m_PositionNormals[index1].m_Position;
            D3DXVECTOR4 v2 = m_PositionNormals[index3].m_Position - m_PositionNormals[index1].m_Position;
             
            D3DXVec3Cross(  (D3DXVECTOR3*)&m_PositionNormals[indexVertex].m_Normal,
                            (D3DXVECTOR3*)&v1,
                            (D3DXVECTOR3*)&v2);

            if (i == m_ClothSide - 2)
            {
                int indexVertexToEdit = (i + 1) + j * m_ClothSide;
                m_PositionNormals[indexVertexToEdit].m_Normal = m_PositionNormals[indexVertex].m_Normal;
            }
            if (j == m_ClothSide - 2)
            {
                int indexVertexToEdit = i + (j + 1) * m_ClothSide;
                m_PositionNormals[indexVertexToEdit].m_Normal = m_PositionNormals[indexVertex].m_Normal;
            }

            if (i == m_ClothSide - 2 && j == m_ClothSide - 2)
            {
                int indexVertexToEdit = i + 1+ (j + 1) * m_ClothSide;
                m_PositionNormals[indexVertexToEdit].m_Normal = m_PositionNormals[indexVertex].m_Normal;
            }
        }
    }

  RenderDynamicMesh();
}

void DX11Renderer::SetViewAndProjMatrix(const D3DXMATRIX& view, const D3DXMATRIX& projection, const D3DXVECTOR4 &cameraPosition)
{
    // Set the two matrix for in constant buffer
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    m_pImmediateContext->Map( m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    m_ConstantBuffer = ( ConstantBufferData* )MappedResource.pData; 
    D3DXMatrixMultiply( &m_ConstantBuffer->m_WorldViewProj, &view, &projection );
    D3DXMatrixInverse( &m_ConstantBuffer->m_InvView, NULL, &view );
    m_ConstantBuffer->m_CameraPosition = cameraPosition;
    m_pImmediateContext->Unmap( m_pConstantBuffer, 0 ); 
}

bool DX11Renderer::RenderParticles()
{
    // Now set the rasterizer state.
	m_pImmediateContext->RSSetState(m_RasterState);

    // Vertex Shader
    m_pImmediateContext->VSSetShader( m_pRenderParticlesVS, NULL, 0 );
    m_pImmediateContext->IASetInputLayout( m_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { m_pParticlePositionBuffer };
    UINT stride[1] = { sizeof( Position ) };
    UINT offset[1] = { 0 };
    m_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    

    if (! m_IsUsingInteroperability)
    {
        // Update buffer with new value for the Vertex shader 
        m_pImmediateContext->UpdateSubresource(m_pParticlePositionBuffer, 0, NULL, &m_Particles[0], 0, 0);
    }

  
   m_pImmediateContext->HSSetShader( NULL, NULL, 0 );
   m_pImmediateContext->DSSetShader( NULL, NULL, 0 );


   // Geometry shader
    m_pImmediateContext->GSSetShader( m_pRenderParticlesGS, NULL, 0 );
    m_pImmediateContext->GSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
   
    
    // Pixel shader
    m_pImmediateContext->PSSetShader( m_pRenderParticlesPS, NULL, 0 );
    // Texture for the pixel shader
    m_pImmediateContext->PSSetShaderResources( 0, 1, &m_pParticleTexRV );
    m_pImmediateContext->PSSetSamplers( 0, 1, &m_pSampleStateLinear );

    m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateParticle, 0 );
    m_pImmediateContext->OMSetBlendState( m_pBlendingStateParticle, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    
    m_pImmediateContext->Draw( m_ParticlesCount, 0 );

    return true;
}

bool DX11Renderer::RenderSpheresOnGPU()
{
    // Now set the rasterizer state.
	m_pImmediateContext->RSSetState(m_RasterState);

    // Vertex Shader
    m_pImmediateContext->VSSetShader( m_pRenderSphereVS, NULL, 0 );
    m_pImmediateContext->IASetInputLayout( m_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { m_pParticlePositionBuffer };
    UINT stride[1] = { sizeof( Position ) };
    UINT offset[1] = { 0 };
    m_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    // Update buffer with new value for the Vertex shader 
    m_pImmediateContext->UpdateSubresource(m_pParticlePositionBuffer, 0, NULL, &m_Particles[0], 0, 0); 
   
   m_pImmediateContext->HSSetShader( NULL, NULL, 0 );
   m_pImmediateContext->DSSetShader( NULL, NULL, 0 );

   // Geometry shader
   m_pImmediateContext->GSSetShader( m_pRenderSphereGS, NULL, 0 );
    // Set the two matrix for the Geometry shaders
    m_pImmediateContext->GSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
   
    // Pixel shader
    m_pImmediateContext->PSSetShader( m_pRenderSpherePS, NULL, 0 );
    // Texture for the pixel shader
    m_pImmediateContext->PSSetShaderResources( 0, 1, &m_pSphereTexRV );
    m_pImmediateContext->PSSetSamplers( 0, 1, &m_pSampleStateLinear );

     m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
    m_pImmediateContext->OMSetBlendState( NULL, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    
    m_pImmediateContext->Draw( m_ParticlesCount, 0 );

    return true;
}

bool DX11Renderer::RenderMeshOnGPU(bool isRenderingSphere /* = false */)
{
    // Now set the rasterizer state.
	m_pImmediateContext->RSSetState(m_RasterStateDoubleFaces);

    // Vertex Shader
    m_pImmediateContext->VSSetShader( m_pRenderMeshVS, NULL, 0 );
    m_pImmediateContext->IASetInputLayout( m_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1];
    
    if (isRenderingSphere)
    {
        pBuffers[0] = m_pSphereBuffer;
    }
    else
    {
        pBuffers[0] = m_pParticlePositionBuffer;
    }

    UINT stride[1] = { sizeof( Position ) };
    UINT offset[1] = { 0 };
    m_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST );

    if (isRenderingSphere)
    {
         // Update buffer with new value for the Vertex shader 
        m_pImmediateContext->UpdateSubresource(m_pSphereBuffer, 0, NULL, &m_Spheres[0], 0, 0);
    }
    else if (! m_IsUsingInteroperability)
    {
        // Update buffer with new value for the Vertex shader 
        m_pImmediateContext->UpdateSubresource(m_pParticlePositionBuffer, 0, NULL, &m_Particles[0], 0, 0);
    }
   
   // Hull shader and domain
   m_pImmediateContext->GSSetShader( NULL, NULL, 0 );
   m_pImmediateContext->HSSetShader( m_pRenderMeshHS, NULL, 0 );
   m_pImmediateContext->DSSetShader( m_pRenderMeshDS, NULL, 0 );

    // Set the constant buffer for Vertex and domain shaders
    m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
    m_pImmediateContext->DSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
   
    // Pixel shader
    m_pImmediateContext->PSSetShader( m_pRenderMeshPS, NULL, 0 );
    
    m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
    m_pImmediateContext->OMSetBlendState( NULL, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    

    if (isRenderingSphere)
    {
        m_pImmediateContext->Draw( m_SpheresCount, 0 );
    }
    else
    {
        m_pImmediateContext->Draw( m_ParticlesCount, 0 );
    }


    return true;
}

bool DX11Renderer::RenderDynamicMesh()
{
    // Now set the rasterizer state.
	m_pImmediateContext->RSSetState(m_RasterStateDoubleFaces);

    // Vertex Shader
    m_pImmediateContext->VSSetShader( m_pRenderVS, NULL, 0 );
    m_pImmediateContext->IASetInputLayout( m_pNormalLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { m_pParticleNormalBuffer };
    UINT stride[1] = { sizeof( PositionNormal ) };
    UINT offset[1] = { 0 };
    m_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
        
    // Update buffer with new value for the Vertex shader 
    m_pImmediateContext->UpdateSubresource(m_pParticleNormalBuffer, 0, NULL, &m_PositionNormals[0], 0, 0); 

    m_pImmediateContext->HSSetShader( NULL, NULL, 0 );
    m_pImmediateContext->DSSetShader( NULL, NULL, 0 );
    m_pImmediateContext->GSSetShader( NULL, NULL, 0 );

    
    ID3D11Buffer* pBuffersIndex = m_pIndexBuffer;
    UINT offsetIndex = 0;
    m_pImmediateContext->IASetIndexBuffer( pBuffersIndex, DXGI_FORMAT_R32_UINT, offsetIndex );

    m_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    

    // Set the two matrix for the Vertex shaders
    m_pImmediateContext->VSSetConstantBuffers( 0, 1, &m_pConstantBuffer );
   
    // Pixel shader
    m_pImmediateContext->PSSetShader( m_pRenderPS, NULL, 0 );
    

     m_pImmediateContext->OMSetDepthStencilState( m_pDepthStencilState, 0 );
    m_pImmediateContext->OMSetBlendState( NULL, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    
    m_pImmediateContext->DrawIndexed( m_IndicesCount, 0 ,0 );

    return true;
}



void DX11Renderer::Release()
{
    delete []m_Particles;
    m_Particles = NULL;
    delete []m_PositionNormals;
    m_PositionNormals = NULL;
    CleanupDevice();
}


// Clean up the objects we've created
void DX11Renderer::CleanupDevice()
{
    // Release shaders
    if (m_pRenderParticlesVS )
    {
        m_pRenderParticlesVS->Release();
    }
    if (m_pRenderParticlesGS )
    {
        m_pRenderParticlesGS->Release();
    }
    if (m_pRenderParticlesPS )
    {
        m_pRenderParticlesPS->Release();
    }

    if (m_pRenderSphereVS )
    {
        m_pRenderSphereVS->Release();
    }
    if (m_pRenderSphereGS )
    {
        m_pRenderSphereGS->Release();
    }
    if (m_pRenderSpherePS )
    {
        m_pRenderSpherePS->Release();
    }

    if (m_pRenderMeshVS )
    {
        m_pRenderMeshVS->Release();
    }
    if (m_pRenderMeshHS )
    {
        m_pRenderMeshHS->Release();
    }
    if (m_pRenderMeshDS )
    {
        m_pRenderMeshDS->Release();
    }
    if (m_pRenderMeshPS )
    {
        m_pRenderMeshPS->Release();
    }

    if (m_pSampleStateLinear )
    {
        m_pSampleStateLinear->Release();
    }
    if (m_pBlendingStateParticle )
    {
        m_pBlendingStateParticle->Release();
    }
    if (m_pDepthStencilState )
    {
        m_pDepthStencilState->Release();
    }
    if (m_pDepthStencilStateParticle )
    {
        m_pDepthStencilStateParticle->Release();
    }    
    if (m_pDepthStencilView )
    {
        m_pDepthStencilView->Release();
    }    

    // Release shader buffers and ressources
    if( m_pParticlePositionBuffer ) 
    {
        m_pParticlePositionBuffer->Release();
    }
    if( m_pParticleNormalBuffer ) 
    {
        m_pParticleNormalBuffer->Release();
    }
    if (m_pConstantBuffer )
    {
        m_pConstantBuffer->Release();
    }
    if (m_pParticleTexRV )
    {
        m_pParticleTexRV->Release();
    }
    if (m_pParticleVertexLayout )
    {
        m_pParticleVertexLayout->Release();
    }  
    if (m_pPatchLayout )
    {
        m_pPatchLayout->Release();
    }  
    if (m_pNormalLayout )
    {
        m_pNormalLayout->Release();
    }  

    //
    if( m_pImmediateContext )
    {
        m_pImmediateContext->ClearState();
    }
    
    if( m_pRenderTargetView ) 
    {
        m_pRenderTargetView->Release();
    }
    if( m_pSwapChain )
    {
        m_pSwapChain->Release();
    }
    if( m_pImmediateContext ) 
    {
        m_pImmediateContext->Release();
    }
    if( m_pd3dDevice ) 
    {
        m_pd3dDevice->Release();
    }
}

void DX11Renderer::CheckResult(HRESULT hr)
{
	if(FAILED(hr) )
    {
		assert(false);
	}
}
