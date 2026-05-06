// 64klang2GUI.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "64klang2Wrapper.h"
#include "../64klang2Core/SynthController.h"
#include "windows.h"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::IO;
using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Windows::Media;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// helper singleton for loading the 64klangGUI assembly
// more specifically for setting up assembly load path so that the gui assembly is found by the core dll
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public ref class DotNet
{
public:

	static DotNet^ Instance()
	{
		if (instance == nullptr)
		{
			instance = gcnew DotNet();
			instance->Init();
		}
		return instance;
	}

private:
		
	static DotNet^ instance = nullptr;
	bool initialized;

	// -------------------------------------------------------------------------------------------

	DotNet() 
	{			
		initialized = false;
	}

	// -------------------------------------------------------------------------------------------

	void Init()
	{
		if (!initialized)
		{
//			MessageBox::Show(L"Init called", L"Aha", MessageBoxButton::OK);
			AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(this, &DotNet::ResolveAssembly);
			initialized = true;
		}
	}

	// -------------------------------------------------------------------------------------------

	Assembly^ ResolveAssembly(Object^ sender, ResolveEventArgs^ args)
	{
		try
		{
//			MessageBox::Show(L"ResolveAssembly entered", L"Aha1", MessageBoxButton::OK);
			String^ folderPath = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
//			MessageBox::Show(folderPath, L"Aha2", MessageBoxButton::OK);
			AssemblyName^ assemblyName = gcnew AssemblyName(args->Name);
//			MessageBox::Show(assemblyName->Name, L"Aha3", MessageBoxButton::OK);
			String^ fileName = assemblyName->Name + ".dll";
			String^ assemblyPath = Path::Combine(folderPath, fileName);
//			MessageBox::Show(assemblyPath, L"Aha4", MessageBoxButton::OK);
			if (File::Exists(assemblyPath) == false) 
				return nullptr;
			//Assembly^ assembly = Assembly::LoadFrom(assemblyPath);
			Assembly^ assembly = Assembly::UnsafeLoadFrom(assemblyPath);
			if (assembly == nullptr)
			{
				MessageBox::Show(assemblyPath, L"ResolveAssembly failed", MessageBoxButton::OK);
			}
			else
			{
				//	MessageBox::Show(assemblyPath, L"ResolveAssembly successful", MessageBoxButton::OK);
			}
//			MessageBox::Show(L"ResolveAssembly successful", L"Aha6", MessageBoxButton::OK);
			return assembly;
		}
		catch (Exception^ ex)
		{
			MessageBox::Show(ex->ToString(), L"Fuck", MessageBoxButton::OK);
			return nullptr;
		}
	}	
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// a singleton class to provide the one gui for 64klang
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public ref class DotNetGUI
{
public:
		
	static DotNetGUI^ instance()
	{
		if (_instance == nullptr)
			_instance = gcnew DotNetGUI();
		return _instance;
	}

	void OpenWindow()
	{
		if (mainWindow == nullptr)
		{
			try
			{
				mainWindow = gcnew _64klang2GUI::SynthWindow();
			}
			catch (Exception^ ex)
			{
				MessageBox::Show(ex->ToString(), L"Exception while creating UI", MessageBoxButton::OK);
			}

			mainWindow->SynthCanvas()->menuItemHandler = gcnew _64klang2GUI::SynthCanvas::MenuItemHandler(this, &DotNetGUI::MenuItemCB);
			mainWindow->SynthCanvas()->copyNodeHandler = gcnew _64klang2GUI::SynthCanvas::CopyNodeHandler(this, &DotNetGUI::CopyNodeCB);
			mainWindow->SynthCanvas()->nodeMenuItemHandler = gcnew _64klang2GUI::SynthCanvas::NodeMenuItemHandler(this, &DotNetGUI::NodeMenuItemCB);
			mainWindow->SynthCanvas()->removeNodeHandler = gcnew _64klang2GUI::SynthCanvas::RemoveNodeHandler(this, &DotNetGUI::RemoveNodeCB);
			mainWindow->SynthCanvas()->connectionInputHandler = gcnew _64klang2GUI::SynthCanvas::ConnectionInputHandler(this, &DotNetGUI::ConnectionCB);
			mainWindow->SynthCanvas()->updateNodeValuesHandler = gcnew _64klang2GUI::SynthCanvas::UpdateNodeValuesHandler(this, &DotNetGUI::UpdateNodeValuesCB);			
			mainWindow->SynthCanvas()->valueChangedHandler = gcnew _64klang2GUI::SynthCanvas::ValueChangedHandler(this, &DotNetGUI::ValueChangedCB);
			mainWindow->SynthCanvas()->modeChangedHandler = gcnew _64klang2GUI::SynthCanvas::ModeChangedHandler(this, &DotNetGUI::ModeChangedCB);
			mainWindow->SynthCanvas()->nodeResetEventSignalHandler = gcnew _64klang2GUI::SynthCanvas::NodeResetEventSignaldHandler(this, &DotNetGUI::NodeResetEventSignalCB);
			mainWindow->SynthCanvas()->getArpStepDataHandler = gcnew _64klang2GUI::SynthCanvas::GetArpStepDataHandler(this, &DotNetGUI::GetArpStepDataCB);
			mainWindow->SynthCanvas()->getArpPlayPosHandler = gcnew _64klang2GUI::SynthCanvas::GetArpPlayPosHandler(this, &DotNetGUI::GetArpPlayPosCB);
			mainWindow->SynthCanvas()->setArpStepDataHandler = gcnew _64klang2GUI::SynthCanvas::SetArpStepDataHandler(this, &DotNetGUI::SetArpStepDataCB);
			mainWindow->SynthCanvas()->setNodeNameHandler = gcnew _64klang2GUI::SynthCanvas::SetNodeNameHandler(this, &DotNetGUI::SetNodeNameCB);
			mainWindow->SynthCanvas()->getNodeNameHandler = gcnew _64klang2GUI::SynthCanvas::GetNodeNameHandler(this, &DotNetGUI::GetNodeNameCB);
			mainWindow->SynthCanvas()->getNodeSignalHandler = gcnew _64klang2GUI::SynthCanvas::GetNodeSignalHandler(this, &DotNetGUI::GetNodeSignalCB);
			mainWindow->SynthCanvas()->getNodeValueHandler = gcnew _64klang2GUI::SynthCanvas::GetNodeValueHandler(this, &DotNetGUI::GetNodeValueCB);
			mainWindow->SynthCanvas()->getSAPITextHandler = gcnew _64klang2GUI::SynthCanvas::GetSAPITextHandler(this, &DotNetGUI::GetSAPITextCB);
			mainWindow->SynthCanvas()->setSAPITextHandler = gcnew _64klang2GUI::SynthCanvas::SetSAPITextHandler(this, &DotNetGUI::SetSAPITextCB);
			mainWindow->SynthCanvas()->getFormulaTextHandler = gcnew _64klang2GUI::SynthCanvas::GetFormulaTextHandler(this, &DotNetGUI::GetFormulaTextCB);
			mainWindow->SynthCanvas()->setFormulaTextHandler = gcnew _64klang2GUI::SynthCanvas::SetFormulaTextHandler(this, &DotNetGUI::SetFormulaTextCB);
			mainWindow->SynthCanvas()->clearSelectionHandler = gcnew _64klang2GUI::SynthCanvas::ClearSelectionHandler(this, &DotNetGUI::ClearSelectionCB);
			mainWindow->SynthCanvas()->setSelectedHandler = gcnew _64klang2GUI::SynthCanvas::SetSelectedHandler(this, &DotNetGUI::SetSelectedCB);
			mainWindow->SynthCanvas()->setNodeProcessingFlagsHandler = gcnew _64klang2GUI::SynthCanvas::SetNodeProcessingFlagsHandler(this, &DotNetGUI::SetNodeProcessingFlagsCB);			
			mainWindow->SynthCanvas()->saveSelectionHandler = gcnew _64klang2GUI::SynthCanvas::SaveSelectionHandler(this, &DotNetGUI::SaveSelectionCB);
			mainWindow->SynthCanvas()->loadSelectionHandler = gcnew _64klang2GUI::SynthCanvas::LoadSelectionHandler(this, &DotNetGUI::LoadSelectionCB);
			mainWindow->SynthCanvas()->getNumActiveVoicesHandler = gcnew _64klang2GUI::SynthCanvas::GetNumActiveVoicesHandler(this, &DotNetGUI::GetNumActiveVoicesCB);			

			mainWindow->voiceUpdateHandler = gcnew _64klang2GUI::SynthWindow::VoiceUpdateHandler(this, &DotNetGUI::VoiceUpdateCB);
			mainWindow->panicHandler = gcnew _64klang2GUI::SynthWindow::PanicHandler(this, &DotNetGUI::PanicCB);
			mainWindow->killVoicesHandler = gcnew _64klang2GUI::SynthWindow::KillVoicesHandler(this, &DotNetGUI::KillVoicesCB);
			mainWindow->queryCoreProcessingMutexHandler = gcnew _64klang2GUI::SynthWindow::QueryCoreProcessingMutexHandler(this, &DotNetGUI::QueryCoreProcessingMutexCB);
			mainWindow->loadPatchHandler = gcnew _64klang2GUI::SynthWindow::LoadPatchHandler(this, &DotNetGUI::LoadPatchCB);
			mainWindow->savePatchHandler = gcnew _64klang2GUI::SynthWindow::SavePatchHandler(this, &DotNetGUI::SavePatchCB);
			mainWindow->resetPatchHandler = gcnew _64klang2GUI::SynthWindow::ResetPatchHandler(this, &DotNetGUI::ResetPatchCB);
			mainWindow->exportPatchHandler = gcnew _64klang2GUI::SynthWindow::ExportPatchHandler(this, &DotNetGUI::ExportPatchCB);
			mainWindow->startRecordingHandler = gcnew _64klang2GUI::SynthWindow::StartRecordingHandler(this, &DotNetGUI::StartRecordingCB);
			mainWindow->stopRecordingHandler = gcnew _64klang2GUI::SynthWindow::StopRecordingHandler(this, &DotNetGUI::StopRecordingCB);
			mainWindow->exportSongHandler = gcnew _64klang2GUI::SynthWindow::ExportSongHandler(this, &DotNetGUI::ExportSongCB);	
			mainWindow->loadChannelHandler = gcnew _64klang2GUI::SynthWindow::LoadChannelHandler(this, &DotNetGUI::LoadChannelCB);
			mainWindow->saveChannelHandler = gcnew _64klang2GUI::SynthWindow::SaveChannelHandler(this, &DotNetGUI::SaveChannelCB);

			mainWindow->setWaveFileHandler = gcnew _64klang2GUI::SynthWindow::SetWaveFileHandler(this, &DotNetGUI::SetWaveFileCB);	
			mainWindow->getWaveFileFrequencyHandler = gcnew _64klang2GUI::SynthWindow::GetWaveFileFrequencyHandler(this, &DotNetGUI::GetWaveFileFrequencyCB);	
			mainWindow->getWaveFileNameHandler = gcnew _64klang2GUI::SynthWindow::GetWaveFileNameHandler(this, &DotNetGUI::GetWaveFileNameCB);	
			mainWindow->getWaveFileCompressedSizeHandler = gcnew _64klang2GUI::SynthWindow::GetWaveFileCompressedSizeHandler(this, &DotNetGUI::GetWaveFileCompressedSizeCB);	
		}
	}

	void CloseWindow()
	{
		if (mainWindow != nullptr)
		{
			mainWindow->PrepareClosing();
			mainWindow->Close();
			mainWindow = nullptr;
		}
	}

private:

	static DotNetGUI^ _instance = nullptr;
	_64klang2GUI::SynthWindow^ mainWindow;
	
	// -------------------------------------------------------------------------------------------

	DotNetGUI() 
	{			
		mainWindow = nullptr;
	}

	// -------------------------------------------------------------------------------------------

	void MenuItemCB(System::String^ item)
	{
		if (item == "Copy")
			MessageBox::Show("Copy", L"MenuItemCB", MessageBoxButton::OK);
		if (item == "Paste")
			MessageBox::Show("Paste", L"MenuItemCB", MessageBoxButton::OK);
	}

	// -------------------------------------------------------------------------------------------

	int CopyNodeCB(int id, int x, int y, int copyFlag)
	{
		SynthNode* oldnode = SynthController::instance()->getNode(id);	
		// extend the name if it has one
		std::string name = SynthController::instance()->getName(id);
		if (name != "")
			name += " (Copy)";
		// create new node in core
		bool globalFlag = oldnode->isGlobal;
		if (copyFlag == 1)
			globalFlag = true;
		if (copyFlag == 2)
			globalFlag = false;
		SynthNode* newnode = SynthController::instance()->createGUINode(oldnode->id, -2, globalFlag, x, y);
		// create new node in gui
		System::String^ namestring = gcnew System::String(name.c_str());
		mainWindow->SynthCanvas()->AddNode(newnode->valueOffset, newnode->id, -2, newnode->isGlobal, x, y, namestring);
		// copy parameters
		int inputs = oldnode->numInputs;
		for (int i = 0; i < inputs; i++)
		{
			// set input constants in gui
			double v1 = SynthController::instance()->getInputValue(oldnode->valueOffset, i, 0);
			double v2 = SynthController::instance()->getInputValue(oldnode->valueOffset, i, 1);
			int mode = SynthController::instance()->getInputMode(oldnode->valueOffset, i);
			SynthController::instance()->setInputValue(newnode->valueOffset, i, v1, v2);
			SynthController::instance()->setInputMode(newnode->valueOffset, i, mode, 0xffffffff);
			mainWindow->SynthCanvas()->SetInitialValue(newnode->valueOffset, i, v1, v2, mode);
		}

		// special case for constant
		if (oldnode->id == CONSTANT_ID)
		{
			double v1 = SynthController::instance()->getInputValue(oldnode->valueOffset, -1, 0);
			double v2 = SynthController::instance()->getInputValue(oldnode->valueOffset, -1, 1);
			SynthController::instance()->setInputValue(newnode->valueOffset, -1, v1, v2);
			mainWindow->SynthCanvas()->SetInitialValue(newnode->valueOffset, -1, v1, v2, 0);
		}

		// special case for voicemanager	
		if (oldnode->id == VOICEMANAGER_ID)
		{
			SynthController::instance()->setArpStepData(newnode->valueOffset, 0xffffffff, SynthController::instance()->getArpStepData(oldnode->valueOffset, 0xffffffff));
			for (int i = 0; i < 32; i++)
				SynthController::instance()->setArpStepData(newnode->valueOffset, i, SynthController::instance()->getArpStepData(oldnode->valueOffset, i));			
		}

		// special case for sapi
		if (oldnode->id == SAPI_ID)
		{
			SynthController::instance()->setSAPIText(newnode->valueOffset, SynthController::instance()->getSAPIText(oldnode->valueOffset));
		}

		// special case for formula
		if (oldnode->id == FORMULA_ID)
		{
			SynthController::instance()->setFormulaText(newnode->valueOffset, oldnode->specialDataText, oldnode->specialDataText2);
		}

		mainWindow->SynthCanvas()->UpdateConfigText(newnode->valueOffset);
		return newnode->valueOffset;
	}

	// -------------------------------------------------------------------------------------------

	void NodeMenuItemCB(int type, int channel, bool isGlobal, int x, int y)
	{
		// synthroot, channelroot, notecontroller and voicemanager and a couple of other nodes are always global
		if (type < VOICEROOT_ID || type == MIDISIGNAL_ID || type == CONSTANT_ID || type == SAPI_ID)
			isGlobal = true;
		SynthNode* node = SynthController::instance()->createGUINode(type, -2, isGlobal, x, y);
		System::String^ pstring = gcnew System::String("");

		mainWindow->SynthCanvas()->AddNode(node->valueOffset, type, channel, isGlobal, x, y, pstring);

		int inputs = node->numInputs;
		for (int i = 0; i < inputs; i++)
		{
			// set input constants in gui
			double v1 = SynthController::instance()->getInputValue(node->valueOffset, i, 0);
			double v2 = SynthController::instance()->getInputValue(node->valueOffset, i, 1);
			int mode = SynthController::instance()->getInputMode(node->valueOffset, i);
			mainWindow->SynthCanvas()->SetInitialValue(node->valueOffset, i, v1, v2, mode);
		}

		// special case for constant
		if (type == CONSTANT_ID)
		{
			double v1 = SynthController::instance()->getInputValue(node->valueOffset, -1, 0);
			double v2 = SynthController::instance()->getInputValue(node->valueOffset, -1, 1);
			mainWindow->SynthCanvas()->SetInitialValue(node->valueOffset, -1, v1, v2, 0);
		}

		mainWindow->SynthCanvas()->UpdateConfigText(node->valueOffset);
	}

	// -------------------------------------------------------------------------------------------

	void RemoveNodeCB(int id)
	{
		SynthController::instance()->deleteNode(id);
	}

	// -------------------------------------------------------------------------------------------

	void ConnectionCB(int from, int to, int index, bool connect)
	{
		if (connect)
			SynthController::instance()->connectInput(from, to, index);
		else
			SynthController::instance()->disconnectInput(to, index, true);
	}

	// -------------------------------------------------------------------------------------------

	void UpdateNodeValuesCB(int id)
	{
		SynthNode* node = SynthController::instance()->getNode(id);	
		if (node == NULL)
			return;

		// special case for constant
		if (node->id == CONSTANT_ID)
		{
			double v1 = SynthController::instance()->getInputValue(node->valueOffset, -1, 0);
			double v2 = SynthController::instance()->getInputValue(node->valueOffset, -1, 1);
			mainWindow->SynthCanvas()->SetInitialValue(node->valueOffset, -1, v1, v2, 0);
		}
		else
		{
			int inputs = node->numInputs;
			for (int i = 0; i < inputs; i++)
			{
				// set input constants in gui
				double v1 = SynthController::instance()->getInputValue(node->valueOffset, i, 0);
				double v2 = SynthController::instance()->getInputValue(node->valueOffset, i, 1);
				int mode = SynthController::instance()->getInputMode(node->valueOffset, i);
				mainWindow->SynthCanvas()->SetInitialValue(node->valueOffset, i, v1, v2, mode);
			}
		}
		mainWindow->SynthCanvas()->UpdateConfigText(node->valueOffset);
	}

	// -------------------------------------------------------------------------------------------

	void ValueChangedCB(int id, int index, double value1, double value2)
	{
		SynthController::instance()->setInputValue(id, index, value1, value2);
	}

	// -------------------------------------------------------------------------------------------

	void ModeChangedCB(int id, int index, int mode, int modemask)
	{
		SynthController::instance()->setInputMode(id, index, mode, modemask);
	}

	// -------------------------------------------------------------------------------------------

	void NodeResetEventSignalCB(int id)
	{
		SynthController::instance()->resetEventSignal(id);
	}

	// -------------------------------------------------------------------------------------------

	int GetArpStepDataCB(int id, int step)
	{
		return SynthController::instance()->getArpStepData(id, step);
	}

	// -------------------------------------------------------------------------------------------

	int GetArpPlayPosCB(int id)
	{
		return SynthController::instance()->getArpPlayPos(id);
	}

	// -------------------------------------------------------------------------------------------

	void SetArpStepDataCB(int id, int step, int value)
	{
		SynthController::instance()->setArpStepData(id, step, value);
	}

	// -------------------------------------------------------------------------------------------

	void SetNodeNameCB(int id, System::String^ name)
	{
		std::string cname = msclr::interop::marshal_as<std::string>(name);	
		SynthController::instance()->setName(id, cname);
	}

	// -------------------------------------------------------------------------------------------

	System::String^ GetNodeNameCB(int id)
	{
		std::string nname = SynthController::instance()->getName(id);
		System::String^ retstring = gcnew System::String(nname.c_str());
		return retstring;		
	}

	// -------------------------------------------------------------------------------------------

	double GetNodeSignalCB(int id, int left, int input)
	{
		return SynthController::instance()->getNodeSignal(id, left, input);
	}

	// -------------------------------------------------------------------------------------------

	double GetNodeValueCB(int id, int index, int channel)
	{
		return SynthController::instance()->getNodeValue(id, index, channel);
	}

	// -------------------------------------------------------------------------------------------

	void SetNodeProcessingFlagsCB(int id, int flags)
	{
		SynthController::instance()->setNodeProcessingFlags(id, flags);
	}

	// -------------------------------------------------------------------------------------------

	System::String^ GetSAPITextCB(int id)
	{
		std::string ntext = SynthController::instance()->getSAPIText(id);
		System::String^ retstring = gcnew System::String(ntext.c_str());
		return retstring;
	}

	// -------------------------------------------------------------------------------------------

	void SetSAPITextCB(int id, System::String^ text)
	{
		std::string ctext = msclr::interop::marshal_as<std::string>(text);	
		SynthController::instance()->setSAPIText(id, ctext);
	}

	// -------------------------------------------------------------------------------------------

	System::String^ GetFormulaTextCB(int id)
	{
		std::string ntext = SynthController::instance()->getFormulaText(id);
		System::String^ retstring = gcnew System::String(ntext.c_str());
		return retstring;
	}

	// -------------------------------------------------------------------------------------------

	void SetFormulaTextCB(int id, System::String^ text, System::String^ rpn)
	{
		std::string ctext = msclr::interop::marshal_as<std::string>(text);
		std::string ctext2 = msclr::interop::marshal_as<std::string>(rpn);
		SynthController::instance()->setFormulaText(id, ctext, ctext2);
	}

	// -------------------------------------------------------------------------------------------

	void ClearSelectionCB()
	{		
		SynthController::instance()->clearSelection();
	}

	// -------------------------------------------------------------------------------------------

	void SetSelectedCB(int id, int selected)
	{
		SynthController::instance()->setSelected(id, selected);
	}

	// -------------------------------------------------------------------------------------------

	void SaveSelectionCB(System::String^ filename)
	{
		UpdateGUIPositions();
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		if (!SynthController::instance()->saveSelection(fname))
		{
			MessageBox::Show(filename, L"ERROR Saving Selection", MessageBoxButton::OK);
		}
	}

	// -------------------------------------------------------------------------------------------

	void LoadSelectionCB(System::String^ filename, int x, int y)
	{
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		if (!SynthController::instance()->loadSelection(fname, x, y))
		{
			MessageBox::Show(filename, L"ERROR Loading Selection", MessageBoxButton::OK);
		}

		// add to gui according to core nodes	
		DWORD numNodes = SynthController::instance()->numSelectedGUINodes();
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			int id = SynthController::instance()->gnID(n);
			int type = SynthController::instance()->gnType(n);
			int channel = SynthController::instance()->gnChannel(n);
			bool isGlobal = SynthController::instance()->gnIsGlobal(n);
			double x = SynthController::instance()->gnX(n);
			double y = SynthController::instance()->gnY(n);
			std::string name = SynthController::instance()->gnName(n);
			System::String^ pstring = gcnew System::String(name.c_str());

			mainWindow->SynthCanvas()->AddNode(id, type, channel, isGlobal, x, y, pstring);
		}

		// create connections
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			SynthNode* node = SynthController::instance()->gnNode(n);
			int to = SynthController::instance()->gnID(n);
			int type = SynthController::instance()->gnType(n);
			int inputs = SynthController::instance()->gnNodeInputs(n);
			for (int i = 0; i < inputs; i++)
			{
				// add connection
				int from = SynthController::instance()->gnInput(n, i);
				mainWindow->SynthCanvas()->AddConnection(from, to, i, true);

				// set input constants in gui
				double v1 = SynthController::instance()->getInputValue(node->valueOffset, i, 0);
				double v2 = SynthController::instance()->getInputValue(node->valueOffset, i, 1);
				int mode = SynthController::instance()->getInputMode(node->valueOffset, i);
				mainWindow->SynthCanvas()->SetInitialValue(to, i, v1, v2, mode);
			}
			// special case for constant
			if (type == CONSTANT_ID)
			{
				double v1 = SynthController::instance()->getInputValue(node->valueOffset, -1, 0);
				double v2 = SynthController::instance()->getInputValue(node->valueOffset, -1, 1);
				mainWindow->SynthCanvas()->SetInitialValue(to, -1, v1, v2, 0);
			}
		}

		// update node configuration texts
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			int node = SynthController::instance()->gnID(n);
			mainWindow->SynthCanvas()->UpdateConfigText(node);

			// and add to selection
			mainWindow->SynthCanvas()->AddToSelection(node);
		}
	}

	// -------------------------------------------------------------------------------------------

	void VoiceUpdateCB()
	{
		int v = SynthController::instance()->getNumActiveVoices();
		mainWindow->SetVoiceCountInfo(v);
	}

	// -------------------------------------------------------------------------------------------

	int GetNumActiveVoicesCB(int id)
	{
		int v = SynthController::instance()->getNumActiveVoices(id);
		return v;
	}

	// -------------------------------------------------------------------------------------------

	void LoadPatchCB(System::String^ filename)
	{
		UpdateGUIPositions();
		std::string fname = msclr::interop::marshal_as<std::string>(filename);		
		if (SynthController::instance()->loadPatch(fname))
		{
			RebuildGUI(true);
		}
		else
		{
			MessageBox::Show(filename, L"ERROR Loading Patch", MessageBoxButton::OK);
		}
	}

	// -------------------------------------------------------------------------------------------

	void SavePatchCB(System::String^ filename)
	{
		UpdateGUIPositions();
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		SynthController::instance()->savePatch(fname);
	}

	// -------------------------------------------------------------------------------------------

	void ResetPatchCB()
	{
		SynthController::instance()->resetPatch(true, true);
		RebuildGUI(true);
	}

	// -------------------------------------------------------------------------------------------

	void StartRecordingCB()
	{
		SynthController::instance()->startRecording();
	}
	
	// -------------------------------------------------------------------------------------------

	void StopRecordingCB()
	{
		SynthController::instance()->stopRecording();
	}

	// -------------------------------------------------------------------------------------------

	void ExportPatchCB(System::String^ filename)
	{
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		SynthController::instance()->exportPatch(fname);
	}

	// -------------------------------------------------------------------------------------------

	void ExportSongCB(System::String^ filename, int timeQuant)
	{
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		SynthController::instance()->exportSong(fname, timeQuant);
	}

	// -------------------------------------------------------------------------------------------

	void LoadChannelCB(int channel, System::String^ filename)
	{
		UpdateGUIPositions();
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		int res = SynthController::instance()->loadChannel(channel, fname);
		if (res == 1)
		{
			RebuildGUI(false);
		}
		else
		{
			if (res == -1)
			{
				MessageBox::Show(filename, L"ERROR Loading Channel", MessageBoxButton::OK);
			}
			if (res == -2)
			{
				MessageBox::Show(L"The channel has connections from/to other channels.\n"
								L"You need to temporarily disconnect those signals to be able to load a new Channel.\n",
								L"ERROR Saving Channel", MessageBoxButton::OK);
			}
		}
	}

	// -------------------------------------------------------------------------------------------

	void SaveChannelCB(int channel, System::String^ filename)
	{
		UpdateGUIPositions();		
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		if (!SynthController::instance()->saveChannel(channel, fname))
		{
			MessageBox::Show(L"The channel has connections from/to other channels.\n"
							L"You need to temporarily disconnect those signals to be able to save the Channel.\n"
							L"Or save the whole Patch instead.", L"ERROR Saving Channel", MessageBoxButton::OK);
		}
	}

	// -------------------------------------------------------------------------------------------

	void UpdateGUIPositions()
	{
		// get current gui positions
		DWORD numNodes = SynthController::instance()->numGUINodes();
		for (DWORD n = 0; n < numNodes; n++)
		{
			int id = SynthController::instance()->gnID(n);
			double x = mainWindow->SynthCanvas()->GetNodeX(id);
			double y = mainWindow->SynthCanvas()->GetNodeY(id);
			if (x >= 0 && y >= 0)
			{
				SynthController::instance()->setX(id, x);
				SynthController::instance()->setY(id, y);
			}
		}
	}

	// -------------------------------------------------------------------------------------------

	void PanicCB()
	{
		SynthController::instance()->panic();
	}

	// -------------------------------------------------------------------------------------------

	void KillVoicesCB()
	{
		SynthController::instance()->killVoices();
	}

	// -------------------------------------------------------------------------------------------

	void QueryCoreProcessingMutexCB(bool acquire)
	{
		SynthController::instance()->queryCoreProcessingMutex(acquire);
	}

	// -------------------------------------------------------------------------------------------

	int SetWaveFileCB(int index, int frequency, System::String^ filename)
	{
		std::string fname = msclr::interop::marshal_as<std::string>(filename);
		SynthController::instance()->setWaveFileReference(index, 0, frequency, fname);
		return SynthController::instance()->getWaveFileCompressedSize(index);
	}

	// -------------------------------------------------------------------------------------------

	int GetWaveFileFrequencyCB(int index)
	{
		return SynthController::instance()->getWaveFileFrequency(index);
	}

	// -------------------------------------------------------------------------------------------

	System::String^ GetWaveFileNameCB(int index)
	{
		std::string fname = SynthController::instance()->getWaveFileName(index);
		System::String^ retstring = gcnew System::String(fname.c_str());
		return retstring;
	}

	// -------------------------------------------------------------------------------------------

	int GetWaveFileCompressedSizeCB(int index)
	{
		return SynthController::instance()->getWaveFileCompressedSize(index);
	}

	// -------------------------------------------------------------------------------------------

public:

	void RebuildGUI(bool resetViewer)
	{
		mainWindow->SynthCanvas()->Reset(resetViewer);

		// create the initial gui according to core nodes	
		DWORD numNodes = SynthController::instance()->numGUINodes();
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			int id = SynthController::instance()->gnID(n);
			int type = SynthController::instance()->gnType(n);
			int channel = SynthController::instance()->gnChannel(n);
			bool isGlobal = SynthController::instance()->gnIsGlobal(n);
			double x = SynthController::instance()->gnX(n);
			double y = SynthController::instance()->gnY(n);
			std::string name = SynthController::instance()->gnName(n);
			System::String^ pstring = gcnew System::String(name.c_str());

			mainWindow->SynthCanvas()->AddNode(id, type, channel, isGlobal, x, y, pstring);
		}

		// create connections
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			SynthNode* node = SynthController::instance()->gnNode(n);
			int to = SynthController::instance()->gnID(n);
			int type = SynthController::instance()->gnType(n);
			int inputs = SynthController::instance()->gnNodeInputs(n);
			for (int i = 0; i < inputs; i++)
			{
				// add connection
				int from = SynthController::instance()->gnInput(n, i);
				mainWindow->SynthCanvas()->AddConnection(from, to, i, true);

				// set input constants in gui
				double v1 = SynthController::instance()->getInputValue(node->valueOffset, i, 0);
				double v2 = SynthController::instance()->getInputValue(node->valueOffset, i, 1);
				int mode = SynthController::instance()->getInputMode(node->valueOffset, i);
				mainWindow->SynthCanvas()->SetInitialValue(to, i, v1, v2, mode);
			}
			// special case for constant
			if (type == CONSTANT_ID)
			{
				double v1 = SynthController::instance()->getInputValue(node->valueOffset, -1, 0);
				double v2 = SynthController::instance()->getInputValue(node->valueOffset, -1, 1);
				mainWindow->SynthCanvas()->SetInitialValue(to, -1, v1, v2, 0);
			}
		}

		// update node configuration texts
		for (DWORD n = 0; n < numNodes; n++)
		{
			if (!SynthController::instance()->gnIsVisible(n))
				continue;
			int node = SynthController::instance()->gnID(n);
			mainWindow->SynthCanvas()->UpdateConfigText(node);
		}
		
		if (resetViewer)
		{
			mainWindow->SynthCanvas()->SetZoomOut(0.25);
			mainWindow->ScrollTo(4096-mainWindow->ActualWidth/2, 4096-mainWindow->ActualHeight/2);
		}
	}

	void PrePatchLoad()
	{
		mainWindow->PreLoadPatchActions();
	}

	void PostPatchLoad()
	{
		mainWindow->PostLoadPatchActions();
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// wrapper native c++ api called from the plugin
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthWrapper* SynthWrapper::_instance = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthWrapper::SynthWrapper()
{
	_windowOpen = 0;
	_rebuildNodes = true;
	// create the .net wrapper
	DotNet::Instance();
	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SynthWrapper* SynthWrapper::instance()
{
	if (_instance == NULL)
		_instance = new SynthWrapper();
	return _instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthWrapper::invalidateGUI()
{
	_rebuildNodes = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthWrapper::loadPatchAndUpdateGUI(std::string filename)
{
	DotNetGUI::instance()->PrePatchLoad();

	SynthController::instance()->loadPatch(filename);
	DotNetGUI::instance()->RebuildGUI(true);

	DotNetGUI::instance()->PostPatchLoad();

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SynthWrapper::openWindow()
{
	_windowOpen++;
	if (_windowOpen > 1)
	{
		MessageBox::Show("There can be only one physical instance of 64klang2 at a time!\nUse alias plugins for successive channels or route all channels midi signals to the one plugin", "64klang2", MessageBoxButton::OK);
		return false;
	}

	//// create some dummy window under the provided parent
	//HWND frame = CreateWindowEx(0, L"STATIC", 0,
	//			WS_CHILD | WS_VISIBLE, 0, 0, x, y,
	//			(HWND)ptr, 0, 0, 0);
	//ShowWindow(frame, SW_SHOW);

	//// create a HwndSource object to wrap the wpf gui in that dummy window
	//HwndSource^ source = gcnew HwndSource(
	//		0, // class style
	//		WS_VISIBLE | WS_CHILD, // style
	//		0, // exstyle
	//		0, 0, x, y,
	//		"HwndSource", // NAME
	//		IntPtr(frame)        // parent window 
	//	);
	//source->RootVisual = DotNetGUI::MainWindow();
	//if (_rebuildNodes)
	//{
	//	_rebuildNodes = false;
	//	DotNetGUI::instance()->RebuildGUI();
	//}

	// reset patch open window and rebuild gui
	//SynthController::instance()->resetPatch();
	DotNetGUI::instance()->OpenWindow();
	DotNetGUI::instance()->RebuildGUI(true);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SynthWrapper::closeWindow()
{
	_windowOpen--;
	if (_windowOpen <= 0)
	{
		_windowOpen = 0;
		DotNetGUI::instance()->CloseWindow();
	}
}


