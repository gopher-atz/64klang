#include "Controller.h"
#include "PluginView.h"
#include "pluginterfaces/base/fstrdefs.h"

namespace Steinberg {
namespace Vst {

tresult PLUGIN_API K64Controller::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
        return result;

    // No exposed parameters for now — all editing via the ImGui GUI
    return kResultOk;
}

IPlugView* PLUGIN_API K64Controller::createView(FIDString name)
{
    if (FIDStringsEqual(name, ViewType::kEditor))
    {
        return new K64PluginView();
    }
    return nullptr;
}

} // namespace Vst
} // namespace Steinberg
