#pragma once

#include <Renderer/RenderContext.hpp>

class UILayer : public RenderLayer
{
public:
    
    struct Attachment;
    
    UILayer(RenderContext& context);
    
    ~UILayer();
    
protected:
    
    virtual RenderNDCScissorRect AsyncUpdate(RenderFrame& frame, RenderNDCScissorRect parentScissor, Float deltaTime) override;
    
    virtual void AsyncProcessEvents(const std::vector<RuntimeInputEvent>& events) override;
    
    void OnAsyncRender(RenderFrame* pFrame);
    
private:
    
    Attachment* _Attachment;
    Ptr<FunctionBase> _RenderCallbackCache;
    Ptr<UILayer> _MyselfProxy;
    
};
