// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MY64KLANG2GUI_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MY64KLANG2GUI_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MY64KLANG2WRAPPER_EXPORTS
#define MY64KLANG2WRAPPER_API __declspec(dllexport)
#else
#define MY64KLANG2WRAPPER_API __declspec(dllimport)
#endif

#include <string>

// This class is exported from the 64klang2Wrapper.dll
class MY64KLANG2WRAPPER_API SynthWrapper 
{
public:	
	static SynthWrapper* instance();

	void invalidateGUI();
	void loadPatchAndUpdateGUI(std::string filename);
	bool openWindow();
	void closeWindow();

private:
	static SynthWrapper* _instance;
	SynthWrapper(void);
	
	int _windowOpen;	
	bool _rebuildNodes;
};
