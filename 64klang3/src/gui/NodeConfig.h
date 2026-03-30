#pragma once

#include <string>
#include <vector>
#include <map>

// Parses 64klang2Config.xml into in-memory structs for driving the ImGui GUI.

namespace K64GUI {

struct ModeItem
{
    std::string name;
    int value = 0;
};

struct ModeGroup
{
    std::string name;
    unsigned int mask = 0;
    int shift = 0;
    bool hideModeText = false;
    int showFor = -1;  // if >= 0, only show this group when display mode (bits[3:0]) equals this value
    std::vector<ModeItem> items;
};

struct ModeFlag
{
    std::string name;
    int value = 0;
    int visible = 0; // visibility: 0=always, 1=hide for global, 2=hide for voice
};

struct InputDef
{
    std::string name;
    double minVal = 0.0;
    double maxVal = 1.0;
    int displayMapping = 0;  // mapping attribute (0-15)
    bool singleInput = false;
    int range = 0;

    // Mode input children (only populated for "Mode" inputs)
    std::vector<ModeGroup> modeGroups;
    std::vector<ModeFlag> modeFlags;

    bool isMode() const { return !modeGroups.empty() || !modeFlags.empty(); }
};

struct NodeTypeDef
{
    int id = 0;
    std::string name;
    std::string category;
    int numInputs = 0;
    int numReqGUIInputs = 0;
    int numMaxGUIInputs = 0;
    bool variableInput = false;
    bool allowSignalInsertion = false;
    std::vector<InputDef> inputs;
};

class NodeConfig
{
public:
    static NodeConfig& instance();

    bool load();

    const NodeTypeDef* getNodeType(int id) const;
    const std::vector<std::string>& getCategories() const;
    const std::vector<const NodeTypeDef*>& getNodesInCategory(const std::string& category) const;

private:
    NodeConfig() = default;
    std::map<int, NodeTypeDef> nodeTypes;
    std::vector<std::string> categories;
    std::map<std::string, std::vector<const NodeTypeDef*>> categoryNodes;
};

} // namespace K64GUI
