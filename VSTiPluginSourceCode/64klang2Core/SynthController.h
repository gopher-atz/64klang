#ifndef __SYNTHCONTROLLER_H__
#define __SYNTHCONTROLLER_H__

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// vsti specific 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef MY64KLANG2CORE_EXPORTS
#define MY64KLANG2CORE_API __declspec(dllexport)
#else
#define MY64KLANG2CORE_API __declspec(dllimport)
#endif

#include <windows.h>
#include <map>
#include <set>
#include <vector>
#include "SynthNode.h"

#define MAX_NODES			32768

class TiXmlElement;

class MY64KLANG2CORE_API SynthController
{
private:

	struct Connection
	{
		int from;
		int to;
		int index;
	};
	struct Position
	{
		int x,y;
	};

	SynthController();
	~SynthController();
	static SynthController* _instance;

	// additional node information only needed for gui
	class NodeGUIInfo
	{
	public:
		NodeGUIInfo() : Visible(false), Node(0), X(0), Y(0), FixedChannel(-2), IsModAdder(false), IsParameter(false), RecursionFlag(false), IsSelected(false)
		{
			for (int i = 0; i < NODE_MAX_INPUTS; i++) 
				ModAdder[i] = 0; 
		}	
		
		// visible in gui
		bool Visible;
		// referred node
		SynthNode* Node;
		// gui position
		double X, Y;
		// the fixed channel for the node
		int FixedChannel;
		// channels the node is used in (only needed for clearing channels)
		std::set<int> Channels;
		// modadder pointers
		SynthNode* ModAdder[NODE_MAX_INPUTS];
		// dependent nodes
		std::set<SynthNode*> Outputs;
		// node is a modadder itself
		bool IsModAdder;
		// constant is a direct parameter of a node
		bool IsParameter;
		// flag indicating already traversed nodes in a recursion
		bool RecursionFlag;
		// bool flag indicating selection
		bool IsSelected;
		// the node name to be used in display
		std::string Name;
	};

	void RemoveOutput(SynthNode* node, SynthNode* target);

public:	
	static SynthController* instance();
	static HINSTANCE		ModuleInstance;
	static HANDLE			DataAccessMutex;

	bool isInitialized();
	bool checkCPUSupport();

	// node managment
	SynthNode* createNode(DWORD id, DWORD channel, DWORD isGlobal);
	SynthNode* createGUINode(DWORD id, DWORD channel, DWORD isGlobal, double x=0, double y=0);
	void deleteNode(DWORD node);	
	
	SynthNode* constant0() { return _nodes[MAX_NODES-1]; }

	// direct node accessors
	SynthNode* getNode(DWORD node);
	void connectInput(DWORD input, DWORD target, DWORD index);	
	void disconnectInput(DWORD target, DWORD index, bool removeOutput=true);
	void setInputValue(DWORD node, DWORD index, double value1, double value2);
	void setInputMode(DWORD node, DWORD index, DWORD mode, DWORD modemask=0xffffffff);
	double getInputValue(DWORD node, DWORD inputIndex, DWORD channel);
	int getInputMode(DWORD node, DWORD inputIndex);
	void resetEventSignal(DWORD node);
	int getArpStepData(DWORD node, DWORD step);
	int getArpPlayPos(DWORD node);
	void setArpStepData(DWORD node, DWORD step, DWORD value);
	void setX(DWORD node, double x);
	void setY(DWORD node, double y);		
	void setName(DWORD node, std::string name);	
	std::string getName(DWORD node);
	double getNodeSignal(DWORD node, int left, int input);
	void setSAPIText(DWORD node, std::string text);	
	void setFormulaText(DWORD node, std::string text, std::string rpn);
	std::string getSAPIText(DWORD node);
	std::string getFormulaText(DWORD node);
	void clearSelection();
	void setSelected(DWORD node, int selected);
	double getNodeValue(DWORD node, DWORD index, DWORD channel);
	void setNodeProcessingFlags(DWORD node, DWORD flags);

	// indexed accessors for mass gui update
	int		numGUINodes();
	int		numSelectedGUINodes();
	SynthNode* gnNode(DWORD index);
	bool	gnIsVisible(DWORD index);
	int		gnType(DWORD index);
	int		gnID(DWORD index);
	int		gnNodeInputs(DWORD index);
	int		gnNodeReqSignals(DWORD index);
	int		gnNodeMaxSignals(DWORD index);
	double	gnX(DWORD index);
	double	gnY(DWORD index);
	std::string	gnName(DWORD index);
	int		gnChannel(DWORD index);
	int		gnInput(DWORD index, DWORD input);
	double	gnInputValue(DWORD index, DWORD inputIndex, DWORD channel);
	int		gnInputMode(DWORD index, DWORD inputIndex);
	bool	gnIsGlobal(DWORD index);	

	// misc
	int		getNumActiveVoices();	
	int		getNumActiveVoices(DWORD node);
	void	queryCoreProcessingMutex(bool acquire);
	void	killVoices();
	void	panic();
	// deferred synth node deletion
	void DeferredSynthFree();
	void AddDeferredFreeNode(void* node);

	// file managment
	void recursiveAddChannel(SynthNode* node, int channel);
	void saveNode(SynthNode* n, TiXmlElement& parent, int saveChannel=-1);
	void recursiveSaveNode(SynthNode* node, int channel, TiXmlElement& root, bool &interconnected);
	void loadNode(TiXmlElement* child, std::map<int, SynthNode*>& idNodeMap, std::vector<Connection>& connections, int targetChannel = -2, SynthNode* channelRoot = 0);

	bool loadPatch(const std::string& filename);
	void savePatch(const std::string& filename);
	void resetPatch(bool createDefault, bool acquireMutex);

	bool loadChannel(int channel, const std::string& filename);
	bool saveChannel(int channel, const std::string& filename);		
	void resetChannel(int channel, bool createDefault = true);	

	bool loadSelection(const std::string& filename, int refX, int refY);
	bool saveSelection(const std::string& filename);

	void setWaveFileReference(int index, int format, int frequency, const std::string& wfpath);
	int getWaveFileFrequency(int index);
	std::string getWaveFileName(int index);
	int getWaveFileCompressedSize(int index);

	// song recording and export
	void exportSong(const std::string& filename, int timeQuant);
	void doExportSong(const std::string& filename);
	void recursiveCollectUsedNodes(SynthNode* node, std::map<SynthNode*, bool>& usedNodes);
	void exportPatch(const std::string& filename);
	void startRecording();
	void stopRecording();
	bool isRecording();

	// forwaring functions to synth core
	void noteOn		(int channel, int note, int velocity);
	void noteOff	(int channel, int note, int velocity);
	void noteAftertouch(int channel, int note, int pressure);
	void midiSignal	(int channel, int value, int cc);
	void setBPM		(float bpm);
	void tick		(float* left, float* right, int samples);

private:
	// the values blob for all nodes
	SynthNode**							_nodes;
	std::map<DWORD, DWORD>				_freeSlots;
	std::map<DWORD, NodeGUIInfo>		_nodeGUIInfo;

	std::vector<NodeGUIInfo*>			_nodesGUIAccessor;			
	std::vector<void*>					_deferredFreeNodes;

	bool _massDataUpdate;

	bool _initialized;
};

/* input_buf can be equal to output_buf */
template <class T> void downsample( T *input_buf, T *output_buf, int output_count, T &filter_state ) 
{
    int input_idx, input_end, output_idx;
	T output_sam;
    input_idx = output_idx = 0;
    input_end = output_count * 2;
    while( input_idx < input_end ) 
	{
        output_sam = filter_state + ( input_buf[ input_idx++ ] >> 1 );
        filter_state = input_buf[ input_idx++ ] >> 2;
        output_buf[ output_idx++ ] = output_sam + filter_state;
    }
}

#endif