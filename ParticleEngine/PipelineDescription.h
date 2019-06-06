#ifndef PIPELINE_DESCRIPTION
#define PIPELINE_DESCRIPTION

struct PipelineDescription
{

    // Exposed simulation parameter, cf accesors
    bool m_IsCreatingGridOnGPU;
    bool m_SPHAndIntegrateOnGPU;
    bool m_CollisionOnGPU;
    bool m_SpringOnGPU;
    bool m_AcceleratorOnGPU;
    bool m_IsUsingAnimation;

    PipelineDescription() : m_IsCreatingGridOnGPU(false),
                            m_SPHAndIntegrateOnGPU(false),
                            m_CollisionOnGPU(false),
                            m_SpringOnGPU(false),
                            m_AcceleratorOnGPU(false),
                            m_IsUsingAnimation(false)
    {
    }
};

#endif // PIPELINE_DESCRIPTION