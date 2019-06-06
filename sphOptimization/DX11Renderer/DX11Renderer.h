// DX11Renderer.h

#pragma once

#ifndef DX11_RENDERER
#define DX11_RENDERER

#include <d3d11.h>
#include <d3dx11.h>
//#include <D3dx11effect.h>
#include <d3dx9math.h>

#include <Windows.h>


#ifdef EXPORT_DLL
    #define MYPROJECT_API __declspec(dllexport)
#else
    #define MYPROJECT_API __declspec(dllimport)
#endif

namespace slmath
{
    class vec4;
}


class MYPROJECT_API DX11Renderer
{
public:
	DX11Renderer();
	~DX11Renderer();


	HRESULT Initialize(const HWND &m_hWnd);
    void InitializeParticles(int particlesCount, int spheresCount, int clothParticlesCount);
    void CreateIndexBuffersForCloth(unsigned int clothSide);

    void BeginFrame();
    void EndFrame();

	void Render(slmath::vec4 * particlePositions, int particlesCount, bool useDefaultColor = false);
    void RenderSphere(slmath::vec4 * particlePositions, int particlesCount);
    void RenderCloth(const slmath::vec4 * particlePositions, int particlesCount);
    void Release();

    void SetIsDisplayingParticles(bool displayParticles);
    bool IsDisplayingParticles()const;

    void SetIsUsingInteroperability(bool isUsingInteroperability);
    bool IsUsingInteroperability() const;

    void SetViewAndProjMatrix(const D3DXMATRIX& view, const D3DXMATRIX& projection, const D3DXVECTOR4 &cameraPosition);
	
    ID3D11Device* GetID3D11Device() const;
    ID3D11Buffer* GetID3D11Buffer() const;

private:
	HRESULT InitDevice(const HWND &m_hWnd);
    HRESULT CreateBuffers();
    HRESULT CreateParticleBuffer();
    HRESULT CreateSpheresBuffer();
    HRESULT CreateMeshBuffer();
	HRESULT InitParticleShaders();
    HRESULT CompileShaderFromFile(  WCHAR* szFileName, LPCSTR szEntryPoint, 
                                    LPCSTR szShaderModel, ID3DBlob** ppBlobOut, 
                                    D3D_SHADER_MACRO* pDefines = NULL);

    void CopyParticles(slmath::vec4 * particlePositions, int particlesCount);
    void CopyParticlesButHardCodeW(slmath::vec4 * particlePositions, int particlesCount);
    void CopySpheres(slmath::vec4 * particlePositions, int particlesCount);
    bool RenderParticles();
    bool RenderSpheresOnGPU();
    bool RenderMeshOnGPU(bool isRenderingSphere = false);
    bool RenderDynamicMesh();
	void CleanupDevice();
    
	void CheckResult(HRESULT hr);

private:
	D3D_DRIVER_TYPE         m_driverType;
	D3D_FEATURE_LEVEL       m_featureLevel;
	ID3D11Device*           m_pd3dDevice;
	ID3D11DeviceContext*    m_pImmediateContext;
	IDXGISwapChain*         m_pSwapChain;
	ID3D11RenderTargetView* m_pRenderTargetView;

    ID3D11RasterizerState* m_RasterState;
    ID3D11RasterizerState* m_RasterStateDoubleFaces;
	
	// Shaders
	ID3D11VertexShader*                 m_pRenderParticlesVS;
	ID3D11GeometryShader*               m_pRenderParticlesGS;
	ID3D11PixelShader*                  m_pRenderParticlesPS;

    ID3D11VertexShader*                 m_pRenderSphereVS;
    ID3D11GeometryShader*               m_pRenderSphereGS;
    ID3D11PixelShader*                  m_pRenderSpherePS;

    ID3D11VertexShader*                 m_pRenderMeshVS;
    ID3D11HullShader*                   m_pRenderMeshHS;
    ID3D11DomainShader*                 m_pRenderMeshDS;
    ID3D11PixelShader*                  m_pRenderMeshPS;

    ID3D11VertexShader*                 m_pRenderVS;
    ID3D11PixelShader*                  m_pRenderPS;

	ID3D11InputLayout*                  m_pParticleVertexLayout;
    ID3D11InputLayout*                  m_pPatchLayout;
    ID3D11InputLayout*                  m_pNormalLayout;

    // Buffer containg particules data to send to the vertex shader 
    ID3D11Buffer*                       m_pParticlePositionBuffer;
    ID3D11Buffer*                       m_pSphereBuffer;
    ID3D11Buffer*                       m_pParticleNormalBuffer;
    ID3D11Buffer*                       m_pIndexBuffer;

    // Constant buffer to send matrix information to the shader
	ID3D11Buffer*                       m_pConstantBuffer;

    // Texture to send to the pixel shader 
	ID3D11ShaderResourceView*           m_pParticleTexRV;
    ID3D11ShaderResourceView*           m_pSphereTexRV;

	
	

	ID3D11SamplerState*                 m_pSampleStateLinear;
	ID3D11DepthStencilState*            m_pDepthStencilState;
    ID3D11DepthStencilState*            m_pDepthStencilStateParticle;
	ID3D11BlendState*                   m_pBlendingStateParticle;

    ID3D11DepthStencilView*             m_pDepthStencilView;

    // Matrix 
    struct ConstantBufferData
    {
        D3DXMATRIX m_WorldViewProj;
        D3DXMATRIX m_InvView;
        D3DXVECTOR4 m_CameraPosition; 

    };
    ConstantBufferData *m_ConstantBuffer;

    // Data position
    struct Position
    {
        D3DXVECTOR4 m_Position;
    };

    struct PositionNormal
    {
        D3DXVECTOR4 m_Position;
        D3DXVECTOR4 m_Normal;
    };

   Position             *m_Particles;
   Position             *m_Spheres;
   PositionNormal       *m_PositionNormals;
   int m_ParticlesCount;
   int m_ParticlesAllocatedCount;
   int m_SpheresCount;
   int m_SpheresAllocatedCount;
   int m_ClothParticlesAllocatedCount;
   int m_ClothParticlesCount;
   unsigned int m_ClothSide;
   unsigned int m_IndicesCount;
   bool m_DisplayParticles;
   bool m_IsUsingInteroperability;
   

};

#endif // DX11_RENDERER