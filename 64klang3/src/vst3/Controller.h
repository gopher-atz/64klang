#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg {
namespace Vst {

class K64Controller : public EditController
{
public:
    static FUnknown* createInstance(void* /*context*/)
    {
        return static_cast<IEditController*>(new K64Controller());
    }

    // EditController overrides
    tresult PLUGIN_API initialize(FUnknown* context) override;
    IPlugView* PLUGIN_API createView(FIDString name) override;
};

} // namespace Vst
} // namespace Steinberg
