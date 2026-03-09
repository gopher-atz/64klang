#include "NodeConfig.h"
#include "NodeConfigData.h"
#include "tinyxml.h"

#include <cstdlib>

namespace K64GUI {

NodeConfig& NodeConfig::instance()
{
    static NodeConfig s_instance;
    return s_instance;
}

bool NodeConfig::load()
{
    TiXmlDocument doc;
    doc.Parse(k_nodeConfigXml);
    if (doc.Error())
        return false;

    TiXmlElement* root = doc.RootElement();
    if (!root)
        return false;

    // Parse <Nodes>
    TiXmlElement* nodesElem = root->FirstChildElement("Nodes");
    if (nodesElem)
    {
        for (TiXmlElement* nodeElem = nodesElem->FirstChildElement("Node");
             nodeElem; nodeElem = nodeElem->NextSiblingElement("Node"))
        {
            NodeTypeDef def;

            nodeElem->QueryIntAttribute("typeid", &def.id);
            const char* name = nodeElem->Attribute("name");
            if (name) def.name = name;
            nodeElem->QueryIntAttribute("numInputs", &def.numInputs);
            nodeElem->QueryIntAttribute("numReqGUIInputs", &def.numReqGUIInputs);
            nodeElem->QueryIntAttribute("numMaxGUIInputs", &def.numMaxGUIInputs);

            int vi = 0;
            if (nodeElem->QueryIntAttribute("variableInput", &vi) == TIXML_SUCCESS)
                def.variableInput = (vi != 0);

            int asi = 0;
            if (nodeElem->QueryIntAttribute("allowSignalInsertion", &asi) == TIXML_SUCCESS)
                def.allowSignalInsertion = (asi != 0);

            // Parse <NodeInput> children
            for (TiXmlElement* inputElem = nodeElem->FirstChildElement("NodeInput");
                 inputElem; inputElem = inputElem->NextSiblingElement("NodeInput"))
            {
                InputDef input;

                const char* inputName = inputElem->Attribute("name");
                if (inputName) input.name = inputName;

                double minV = 0.0, maxV = 1.0;
                if (inputElem->QueryDoubleAttribute("minValue", &minV) == TIXML_SUCCESS)
                    input.minVal = minV;
                if (inputElem->QueryDoubleAttribute("maxValue", &maxV) == TIXML_SUCCESS)
                    input.maxVal = maxV;

                int mapping = 0;
                if (inputElem->QueryIntAttribute("mapping", &mapping) == TIXML_SUCCESS)
                    input.displayMapping = mapping;

                int si = 0;
                if (inputElem->QueryIntAttribute("singleInput", &si) == TIXML_SUCCESS)
                    input.singleInput = (si != 0);

                int rng = 0;
                if (inputElem->QueryIntAttribute("range", &rng) == TIXML_SUCCESS)
                    input.range = rng;

                // Parse <ModeGroup> children
                for (TiXmlElement* mgElem = inputElem->FirstChildElement("ModeGroup");
                     mgElem; mgElem = mgElem->NextSiblingElement("ModeGroup"))
                {
                    ModeGroup mg;
                    const char* mgName = mgElem->Attribute("name");
                    if (mgName) mg.name = mgName;

                    const char* maskStr = mgElem->Attribute("mask");
                    if (maskStr) mg.mask = (unsigned int)strtoul(maskStr, nullptr, 10);

                    mgElem->QueryIntAttribute("shift", &mg.shift);

                    int hmt = 0;
                    if (mgElem->QueryIntAttribute("hidemodetext", &hmt) == TIXML_SUCCESS)
                        mg.hideModeText = (hmt != 0);

                    for (TiXmlElement* miElem = mgElem->FirstChildElement("ModeItem");
                         miElem; miElem = miElem->NextSiblingElement("ModeItem"))
                    {
                        ModeItem mi;
                        const char* miName = miElem->Attribute("name");
                        if (miName) mi.name = miName;
                        miElem->QueryIntAttribute("value", &mi.value);
                        mg.items.push_back(mi);
                    }

                    input.modeGroups.push_back(std::move(mg));
                }

                // Parse <ModeFlag> children
                for (TiXmlElement* mfElem = inputElem->FirstChildElement("ModeFlag");
                     mfElem; mfElem = mfElem->NextSiblingElement("ModeFlag"))
                {
                    ModeFlag mf;
                    const char* mfName = mfElem->Attribute("name");
                    if (mfName) mf.name = mfName;

                    // Use string parsing — value can be negative (bit 31 = -2147483648)
                    const char* valStr = mfElem->Attribute("value");
                    if (valStr) mf.value = (int)strtol(valStr, nullptr, 10);

                    int vis = 0;
                    if (mfElem->QueryIntAttribute("visibility", &vis) == TIXML_SUCCESS)
                        mf.visible = vis;

                    input.modeFlags.push_back(std::move(mf));
                }

                def.inputs.push_back(std::move(input));
            }

            nodeTypes[def.id] = std::move(def);
        }
    }

    // Parse <Menu> for categories
    TiXmlElement* menuElem = root->FirstChildElement("Menu");
    if (menuElem)
    {
        for (TiXmlElement* catElem = menuElem->FirstChildElement("MenuItem");
             catElem; catElem = catElem->NextSiblingElement("MenuItem"))
        {
            const char* header = catElem->Attribute("Header");
            if (!header) continue;

            std::string catName = header;
            categories.push_back(catName);
            auto& catList = categoryNodes[catName];

            for (TiXmlElement* itemElem = catElem->FirstChildElement("MenuItem");
                 itemElem; itemElem = itemElem->NextSiblingElement("MenuItem"))
            {
                int id = 0;
                if (itemElem->QueryIntAttribute("id", &id) == TIXML_SUCCESS)
                {
                    auto it = nodeTypes.find(id);
                    if (it != nodeTypes.end())
                    {
                        it->second.category = catName;
                        catList.push_back(&it->second);
                    }
                }
            }
        }
    }

    return true;
}

const NodeTypeDef* NodeConfig::getNodeType(int id) const
{
    auto it = nodeTypes.find(id);
    if (it != nodeTypes.end())
        return &it->second;
    return nullptr;
}

const std::vector<std::string>& NodeConfig::getCategories() const
{
    return categories;
}

static const std::vector<const NodeTypeDef*> s_emptyNodeList;

const std::vector<const NodeTypeDef*>& NodeConfig::getNodesInCategory(const std::string& category) const
{
    auto it = categoryNodes.find(category);
    if (it != categoryNodes.end())
        return it->second;
    return s_emptyNodeList;
}

} // namespace K64GUI
