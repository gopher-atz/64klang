using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Xml;
using System.IO;
using System.Reflection;

namespace _64klang2GUI
{

	

	/// <summary>
	/// Interaction logic for GUISynthNode.xaml
	/// </summary>
	public partial class GUISynthNode : UserControl
	{
		public static Dictionary<int, GUISynthNode> IDNodeMap = new Dictionary<int,GUISynthNode>();

		public SynthCanvas synthCanvas;

		public GUISynthNodeEdit EditWindow;
		public int IsEditOpen;

		public int ID; // slot in the core array of nodes
		public int Channel;
		public bool IsGlobal;
		public NodeInfo Info;
		public List<int> InputID;
		public List<int> OutputID;
		public int NumVariableInputs;
		public List<InputValues> InitialValues;
		private bool _isSelected;
		public bool	IsSelected	
		{ 
			get
			{
				return _isSelected;
			}
			set 
			{
				BrushConverter bc = new BrushConverter();
				if (value == true)
				{
					this.NodeVerticalStackPanel.Background = (Brush)bc.ConvertFrom("#FFFFCD50");
				}
				else
				{
					if (IsGlobal)
						NodeVerticalStackPanel.Background = (Brush)bc.ConvertFrom("#FFB7A0B7");
					else
						NodeVerticalStackPanel.Background = (Brush)bc.ConvertFrom("#FF7FA3BA");
				}
				_isSelected = value;
			} 
		}

		public GUISynthNode(SynthCanvas canvas, int id, int type, int channel, bool isGlobal, string guiname)
		{
			InitializeComponent();

			synthCanvas = canvas;
			EditWindow = null;
			IsEditOpen = 0;

			ID = id;			
			Channel = channel;
			IsGlobal = isGlobal;
			Info = NodeInfo.Info(type);

			InputID = new List<int>();
			OutputID = new List<int>();
			InitialValues = new List<InputValues>();
			for (int i = 0; i < 16; i++)
			{
				InputID.Add(-1);
				InitialValues.Add(new InputValues());
			}

			// channel root?
			if (type == 1)
			{
				if ((guiname != null) && (guiname != ""))
					this.NodeName.Content = (channel+1).ToString() + " : " + guiname;
				else
					this.NodeName.Content = "Channel " + (channel+1).ToString();
			}
			// other nodes
			else
			{
				if ((guiname != null) && (guiname != ""))
					this.NodeName.Content = guiname;
				else
					this.NodeName.Content = Info.Name;
			}

			// set input names
			int inputCount;
			for (inputCount = 0; inputCount < Info.NumMaxGUISignals; inputCount++)
			{
				if (inputCount == 0) InputName0.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 1) InputName1.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 2) InputName2.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 3) InputName3.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 4) InputName4.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 5) InputName5.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 6) InputName6.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 7) InputName7.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 8) InputName8.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 9) InputName9.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 10) InputName10.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 11) InputName11.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 12) InputName12.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 13) InputName13.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 14) InputName14.Content = Info.InputInfos[inputCount].Name;
				if (inputCount == 15) InputName15.Content = Info.InputInfos[inputCount].Name;
			}
			// for variable size inputs set to actual input count from node
			if (Info.VariableInput)
				inputCount = 0;

			// remove non needed input slots in gui
			while (inputCount++ < 16)
			{
				if (Info.VariableInput)
					NodeVerticalStackPanel.Children[inputCount+1].Visibility = System.Windows.Visibility.Collapsed;
				else
					NodeVerticalStackPanel.Children.RemoveAt(NodeVerticalStackPanel.Children.Count - 2);
			}

			// remove edit button for variable inputs and voice root or nodes where gui signals = maxinputs
			if ((Info.VariableInput) || (Info.TypeID == 4) || ((Info.NumInputs == Info.NumReqGUISignals) && (Info.TypeID < NodeInfo.ConstantTypeID())))
			{
				NodeVerticalStackPanel.Children[NodeVerticalStackPanel.Children.Count - 1].Visibility = System.Windows.Visibility.Collapsed;
			}

			// dont ever remove SynthRoot, ChannelRoot, NoteController
			if (Info.TypeID < 3)
			{
				this.NodeRemove.Visibility = System.Windows.Visibility.Hidden;
				// vu meter for channel and synth root
				if (Info.TypeID < 2)
				{
					this.VUMeter.Visibility = System.Windows.Visibility.Visible;

                    CompositionTarget.Rendering += CompositionTarget_Rendering;
				}
			}
			
			// add voice marker for voicemanager
			if (Info.TypeID == 3)
			{
                CompositionTarget.Rendering += CompositionTarget_Rendering;
			}

			// channelroot gets buttons for load, save, reset and name
			if (Info.TypeID == 1)
			{
				Button loadButton = new Button();
				loadButton.Content = "Load Channel";
				NodeAdditionalVerticalStackPanel.Children.Add(loadButton);
				loadButton.Click += new RoutedEventHandler(loadButton_Click);

				Button saveButton = new Button();
				saveButton.Content = "Save Channel";
				NodeAdditionalVerticalStackPanel.Children.Add(saveButton);
				saveButton.Click += new RoutedEventHandler(saveButton_Click);

				TextBox nameBox = new TextBox();
				nameBox.Name = "NodeNameTB";
				if ((guiname != null) && (guiname != ""))
					nameBox.Text = guiname;
				else
					nameBox.Text = "";
				NodeAdditionalVerticalStackPanel.Children.Add(nameBox);
				nameBox.TextChanged += new TextChangedEventHandler(nameBox_TextChanged);
			}

            // give some nodes the option to change the name
            this.FreeNodeName.Text = this.NodeName.Content.ToString();              
            this.FreeNodeName.TextChanged += new TextChangedEventHandler(FreeNodeName_TextChanged);                

			// slight coloring depending on global state
			BrushConverter bc = new BrushConverter();
			if (IsGlobal)
				NodeVerticalStackPanel.Background = (Brush)bc.ConvertFrom("#FFB7A0B7");
			else
				NodeVerticalStackPanel.Background = (Brush)bc.ConvertFrom("#FF7FA3BA");

			this.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
			this.Arrange(new Rect(0, 0, this.DesiredSize.Width, this.DesiredSize.Height));

			IDNodeMap[ID] = this;			

			IsSelected = false;
			this.Cursor = Cursors.Arrow;						
		}


        ~GUISynthNode()
		{
			KillTimer();
		}

		public void KillTimer()
		{
            CompositionTarget.Rendering -= CompositionTarget_Rendering;
		}

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
			// add voice marker for voicemanager
			if (Info.TypeID == 3)
			{
				int numvoices = synthCanvas.GetNumActiveVoices(ID);
				RadialGradientBrush b = this.NodeOut.Fill as RadialGradientBrush;
				if (numvoices > 0)
					b.GradientStops[1].Color = Colors.White;
				else
					b.GradientStops[1].Color = Colors.Black;
			}
			// vu meter for channel and synth root
			if (Info.TypeID < 2)
			{
				double left = Math.Abs(synthCanvas.GetNodeSignal(ID, 0, -2));
				double right = Math.Abs(synthCanvas.GetNodeSignal(ID, 1, -2));
				// inactive channel root/synth root
				if ((left < 1.0000003385357559e-005) && (right < 1.0000003385357559e-005))
				{
                    this.VUMeter.LeftHeight = 0.0;
                    this.VUMeter.RightHeight = 0.0;
                    this.VUMeter.DrawBackground = true;
				}
				else
                {
                    this.VUMeter.LeftHeight = Math.Min(left, 1.0);
                    this.VUMeter.RightHeight = Math.Min(right, 1.0);
                    this.VUMeter.DrawBackground = false;
                }
			}
		}

		void nameBox_TextChanged(object sender, TextChangedEventArgs e)
		{
			TextBox tb = sender as TextBox;
			this.synthCanvas.SetNodeName(ID, tb.Text);

			if (tb.Text != "")
				this.NodeName.Content = (Channel+1).ToString() + " : " + tb.Text;
			else
				this.NodeName.Content = "Channel " + (Channel+1).ToString();

			ComboBoxItem item = this.synthCanvas.MainWindow.JumpToChannel.Items[Channel] as ComboBoxItem;
			item.Content = NodeName.Content.ToString();

			// need to update connections from/to this node as its size might change
			this.synthCanvas.UpdateNodeConnectionPaths(this);
		}
        
		void loadButton_Click(object sender, RoutedEventArgs e)
		{
			this.synthCanvas.MainWindow.LoadChannel(this.Channel);			
		}

		void saveButton_Click(object sender, RoutedEventArgs e)
		{
			this.synthCanvas.MainWindow.SaveChannel(this.Channel);
		}		

		public void UpdateModeText()
		{
			string mode = "";

			// special for constants and osrand
			if (this.Info.TypeID == 35 || this.Info.TypeID >= NodeInfo.ConstantTypeID())
			{
				mode = Math.Round(this.InitialValues[0].LeftValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture) + " / " + Math.Round(this.InitialValues[0].RightValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture);
			}
			// special for synthroot, channelroot, shaper, clip, crossmix, 1pole, 1zero and scaler 
			else if ((this.Info.TypeID < 2) || (this.Info.TypeID == 12) || (this.Info.TypeID == 14) || (this.Info.TypeID == 19) || (this.Info.TypeID == 10) || (this.Info.TypeID == 11) || (this.Info.TypeID == 31))
			{
				mode = Math.Round(this.InitialValues[1].LeftValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture) + " / " + Math.Round(this.InitialValues[1].RightValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture);
			}
			// special for mix
			else if (this.Info.TypeID == 20)
			{
				mode = Math.Round(this.InitialValues[2].LeftValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture) + " / " + Math.Round(this.InitialValues[2].RightValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture);
			}
			// special for panning
			else if (this.Info.TypeID == 13)
			{
				mode = Math.Round(this.InitialValues[1].LeftValue, 2).ToString(System.Globalization.CultureInfo.InvariantCulture);
			}
			else
			{
				int maxGUISignals = this.Info.NumMaxGUISignals;
				// midisignal should just display the mode
				if ((this.Info.TypeID == 29))
					maxGUISignals++;
				if ((maxGUISignals < this.Info.NumInputs) && (this.Info.InputInfos.Count > maxGUISignals))
				{
					InputInfo info = this.Info.InputInfos[maxGUISignals];
					int initialBits = this.InitialValues[maxGUISignals].ModeValue;

					foreach (object o in info.Modes)
					{
						ModeGroup mg = o as ModeGroup;
						if (mg == null)
							continue;
						if (mg.ShowModeText == false)
							continue;
						int currentBits = (initialBits & mg.Mask) >> mg.Shift;

						// for waveplayer get the selected wavetable name from core
						if ((this.Info.TypeID == 42) && (mg.Name == "Wavetable"))
						{
							mode += synthCanvas.MainWindow.GetWaveFileName(currentBits) + " ";
						}
						else
						{
							foreach (ModeItem mi in mg.Items)
							{
								if (currentBits == mi.Value)
									mode += mi.Name + " ";
							}
						}
					}

					// special case for glitch, needs renaming of edit labels
					if (this.Info.TypeID == 49)
					{
						switch (initialBits)
						{
							case 0: // tapestop
								{
									InputName2.Content = "";
									InputName3.Content = "Slowdown";
									InputName4.Content = "Speedup";
									InputName5.Content = "";
									break;
								}
							case 1: // retrigger
								{
									InputName2.Content = "Slice Size";
									InputName3.Content = "Slice Cut";
									InputName4.Content = "dSlice/dPitch";
									InputName5.Content = "Transpose";
									break;
								}
							case 2: // shuffle
								{
									InputName2.Content = "Grain Min/Max";
									InputName3.Content = "Buffer Backrange";
									InputName4.Content = "Repeat/Amount";
									InputName5.Content = "Transpose";
									break;
								}
							case 3: // reverse
								{
									InputName2.Content = "";
									InputName3.Content = "";
									InputName4.Content = "";
									InputName5.Content = "Transpose";
									break;
								}
						}
					}
				}
				if (mode == "")
					mode = "Configuration";
			}
			NodeEditButton.Content = mode;
		}

		private void NodeEditButton_Click(object sender, RoutedEventArgs e)
		{
			if (EditWindow == null)
			{
				synthCanvas.UpdateNodeValues(ID);
				EditWindow = new GUISynthNodeEdit(this);
				EditWindow.NodeEditClose.MouseLeftButtonDown += new MouseButtonEventHandler(NodeEditCloseButton_MouseLeftButtonDown);
			}

			this.NodeEditButton.IsEnabled = false;

			// add to stack panel?
			if ((Keyboard.Modifiers & ModifierKeys.Control) > 0)
			{
				synthCanvas.MainWindow.EditStackPanel.Children.Add(EditWindow);
				IsEditOpen = 2;
			}
			// directly in cavas
			else
			{
				synthCanvas.Children.Add(EditWindow);
				Canvas.SetLeft(EditWindow, Canvas.GetLeft(this) + this.ActualWidth);
				Canvas.SetTop(EditWindow, Canvas.GetTop(this));
				// put on top				
				IsEditOpen = 1;
				synthCanvas.RaiseEditWindow(this);
			}			

			synthCanvas.ClearSelection();
			synthCanvas.AddSelection(this);
		}

		private void NodeEditCloseButton_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			this.NodeEditButton.IsEnabled = true;
			EditWindow.KillTimer();
			synthCanvas.Children.Remove(EditWindow);
			synthCanvas.MainWindow.EditStackPanel.Children.Remove(EditWindow);
			IsEditOpen = 0;
			EditWindow = null;
			e.Handled = true;
		}			

		public void SetInput(int fromID, int index)
		{
			// variable inputs?
			if (Info.VariableInput)
			{
				// add new signal to the end
				if (fromID != -1)
				{
					InputID[index] = fromID;
					NodeVerticalStackPanel.Children[NumVariableInputs + 2].Visibility = System.Windows.Visibility.Visible;
					NumVariableInputs++;
					IDNodeMap[fromID].AddOutput(this.ID);
				}
				else
				{
					IDNodeMap[InputID[index]].RemoveOutput(this.ID);
					// swap with last slot if needed
					if (index < NumVariableInputs - 1)
					{
						InputID[index] = InputID[NumVariableInputs - 1];
					}
					NumVariableInputs--;
					NodeVerticalStackPanel.Children[NumVariableInputs + 2].Visibility = System.Windows.Visibility.Collapsed;					
				}
			}
			else
			{
				// mark the incoming nodes output as used
				if (fromID != -1)
				{
					IDNodeMap[fromID].AddOutput(this.ID);
				}
				// remove the old incoming node if exists
				else
				{
					if (InputID[index] != -1)
					{
						IDNodeMap[InputID[index]].RemoveOutput(this.ID);
					}
				}
				InputID[index] = fromID;
			}
			UpdateInputStates();			
		}

		public int GetInput(int index)
		{
			return InputID[index];
		}

		void AddOutput(int toID)
		{
			OutputID.Add(toID);
			RadialGradientBrush b = this.NodeOut.Fill as RadialGradientBrush;
			b.GradientStops[0].Color = Colors.LightGreen;
		}

		void RemoveOutput(int toID)
		{
			OutputID.Remove(toID);
			if (OutputID.Count == 0)
			{
				RadialGradientBrush b = this.NodeOut.Fill as RadialGradientBrush;
				b.GradientStops[0].Color = Colors.Red;
			}
		}

		public void UpdateInputStates()
		{
			for (int i = 2; i < NodeVerticalStackPanel.Children.Count-1; i++)
			{
				int index = i - 2;
				RadialGradientBrush b = null;
				switch (index)
				{
					case 0: b = Input0.Fill as RadialGradientBrush; break;
					case 1: b = Input1.Fill as RadialGradientBrush; break;
					case 2: b = Input2.Fill as RadialGradientBrush; break;
					case 3: b = Input3.Fill as RadialGradientBrush; break;
					case 4: b = Input4.Fill as RadialGradientBrush; break;
					case 5: b = Input5.Fill as RadialGradientBrush; break;
					case 6: b = Input6.Fill as RadialGradientBrush; break;
					case 7: b = Input7.Fill as RadialGradientBrush; break;
					case 8: b = Input8.Fill as RadialGradientBrush; break;
					case 9: b = Input9.Fill as RadialGradientBrush; break;
					case 10: b = Input10.Fill as RadialGradientBrush; break;
					case 11: b = Input11.Fill as RadialGradientBrush; break;
					case 12: b = Input12.Fill as RadialGradientBrush; break;
					case 13: b = Input13.Fill as RadialGradientBrush; break;
					case 14: b = Input14.Fill as RadialGradientBrush; break;
					case 15: b = Input15.Fill as RadialGradientBrush; break;
				}

				if (InputID[index] != -1)
				{
					b.GradientStops[0].Color = Colors.LightGreen;
				}
				else
				{
					if (index < Info.NumReqGUISignals)
						b.GradientStops[0].Color = Colors.Red;
					else
						b.GradientStops[0].Color = Colors.Black;
				}
			}
		}

		private void NodeRemove_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			SynthCanvas sc = this.Parent as SynthCanvas;
			if (sc != null)
			{
				sc.RemoveNode(this);
			}
			e.Handled = true;
		}

		public void UpdateCanvasPosition(Point delta)
		{
			Canvas.SetLeft(this, Canvas.GetLeft(this) + delta.X);
			Canvas.SetTop(this, Canvas.GetTop(this) + delta.Y);
			if (IsEditOpen == 1)
			{
			    Canvas.SetLeft(EditWindow, Canvas.GetLeft(this) + this.ActualWidth);
			    Canvas.SetTop(EditWindow, Canvas.GetTop(this));
			}
		}

		private void Input_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			int inputIndex = 0;
			if (Info.VariableInput)
			{
				Ellipse clicked = sender as Ellipse;
				Grid indexGrid = clicked.Parent as Grid;
				inputIndex--;
				for (int i = 2; i < NodeVerticalStackPanel.Children.Count; i++)
				{
					if (NodeVerticalStackPanel.Children[i].Visibility == System.Windows.Visibility.Visible)
						inputIndex++;
					if (indexGrid == NodeVerticalStackPanel.Children[i])
						break;
				}
			}
			//
			else
			{
				FrameworkElement fe = sender as FrameworkElement;
				string index = fe.Name.Substring(5, fe.Name.Length - 5);
				inputIndex = Convert.ToInt32(index);
			}
	
			synthCanvas.InputClicked(this, inputIndex);

			e.Handled = true;
		}

		private void NodeOut_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			synthCanvas.NewPendingConnection(this);
			e.Handled = true;
		}

		private void NodeVerticalStackPanel_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
		{
			e.Handled = true;
		}

		private void NodeVerticalStackPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			if (Info.VariableInput && synthCanvas.HasPendingConnection())
			{
				synthCanvas.InputClicked(this, -1);
				e.Handled = true;
			}
		}

        private void NodeName_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            // mute only for channelroot and voicemanager
            if (Info.TypeID == 1 || Info.TypeID == 3)
            {
                // enable mute
                if (this.Opacity == 1.0)
                {
                    synthCanvas.SetNodeProcessingFlags(ID, 1);
                    this.Opacity = 0.33;
                }
                // disable mute
                else 
                {
                    synthCanvas.SetNodeProcessingFlags(ID, 0);
                    this.Opacity = 1.0;
                }
            }
            // text edit activation
            if (Info.TypeID > 3)
            {
                this.NodeName.Visibility = System.Windows.Visibility.Collapsed;
                this.FreeNodeName.Visibility = System.Windows.Visibility.Visible;
            }
        }

        private void FreeNodeName_LostFocus(object sender, RoutedEventArgs e)
        {
            this.NodeName.Visibility = System.Windows.Visibility.Visible;
            this.FreeNodeName.Visibility = System.Windows.Visibility.Collapsed;              
        }

        void FreeNodeName_TextChanged(object sender, TextChangedEventArgs e)
        {
            TextBox tb = sender as TextBox;
            this.synthCanvas.SetNodeName(ID, tb.Text);
            this.NodeName.Content = tb.Text;

            // need to update connections from/to this node as its size might change
            this.synthCanvas.UpdateNodeConnectionPaths(this);
        }
	}

	public class InputValues
	{
		public double LeftValue;
		public double RightValue;
		public int ModeValue;

		public InputValues()
		{
			LeftValue = 0.0;
			RightValue = 0.0;
			ModeValue = 0;
		}

	}

#region NodeInfo

	public class ModeItem
	{
		public ModeGroup Group;
		public string Name;
		public int Value;
		public int Visibility;		

		public ModeItem(ModeGroup group = null)
		{
			Group = group;
			Visibility = 0;			
		}
	}

	public class ModeGroup
	{
		public string Name;
		public int Mask;
		public int Shift;
		public bool ShowModeText;
		public List<ModeItem> Items;

		public ModeGroup()
		{
			Items = new List<ModeItem>();
			ShowModeText = true;
		}
	}

	public enum InputMapping
	{
		Default	= 0,
		ADSR_Rate,
		LFO_Frequency,
		OSC_Frequency,
		BQF_Frequency,
		Attack,
		Release,
		Delay_Time,
		Decibel,
		Ratio,
		Speed,
		RecordTime,
		GlideTime,
		Normalized,
        SNH_Frequency
	}

	public class InputInfo
	{
		public string Name;
		public int MinValueL;
		public int MaxValueL;
		public int RangeL;
		public int MinValueR;
		public int MaxValueR;
		public int RangeR;
		public bool SingleInput;
		public InputMapping Mapping;
		public List<object> Modes;

		public InputInfo()
		{
			Modes = new List<object>();
			RangeL = RangeR = 128;
			Mapping = InputMapping.Default;
		}
	}

	public class NodeInfo
	{
		private static List<NodeInfo> NodeInfoList = null;
		public static NodeInfo Info(int typeid)
		{
			// parse nodeinfos
			if (NodeInfoList == null)
			{
				NodeInfoList = new List<NodeInfo>();

				string xmlName = System.IO.Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "/64klang2Config.xml";

				XmlDocument doc = new XmlDocument();
				doc.XmlResolver = null;
				doc.Load(xmlName);
				foreach (XmlNode n in doc.DocumentElement.ChildNodes[0].ChildNodes)
				{
					XmlElement e = n as XmlElement;
					NodeInfo nodeInfo = new NodeInfo();
					nodeInfo.TypeID = Convert.ToInt32(e.GetAttribute("typeid"));
					nodeInfo.Name = e.GetAttribute("name");
					nodeInfo.NumInputs = Convert.ToInt32(e.GetAttribute("numInputs"));
					nodeInfo.NumReqGUISignals = Convert.ToInt32(e.GetAttribute("numReqGUIInputs"));
					nodeInfo.NumMaxGUISignals = Convert.ToInt32(e.GetAttribute("numMaxGUIInputs"));
					nodeInfo.AllowSignalInsertion = e.GetAttribute("allowSignalInsertion") == "1" ? true : false;
					nodeInfo.VariableInput = e.GetAttribute("variableInput") == "1" ? true : false;
					foreach (XmlNode i in n)
					{
						InputInfo iinfo = new InputInfo();
						
						e = i as XmlElement;
						iinfo.Name = e.GetAttribute("name");

						int v = 0;
						if (e.HasAttribute("minValue"))
							v = Convert.ToInt32(e.GetAttribute("minValue"));
						iinfo.MinValueL = v;
						iinfo.MinValueR = v;

						v = 128;
						if (e.HasAttribute("maxValue"))
							v = Convert.ToInt32(e.GetAttribute("maxValue"));
						iinfo.MaxValueL = v;
						iinfo.MaxValueR = v;

						v = 128;
						if (e.HasAttribute("range"))
							v = Convert.ToInt32(e.GetAttribute("maxValue"));
						iinfo.RangeL = v;
						iinfo.RangeR = v;

						// update again if specific l/r values given
						if (e.HasAttribute("minValueL"))
							iinfo.MinValueL = Convert.ToInt32(e.GetAttribute("minValueL"));						
						if (e.HasAttribute("maxValueL"))
							iinfo.MaxValueL = Convert.ToInt32(e.GetAttribute("maxValueL"));
						if (e.HasAttribute("rangeL"))
							iinfo.RangeL = Convert.ToInt32(e.GetAttribute("rangeL"));

						if (e.HasAttribute("minValueR"))
							iinfo.MinValueR = Convert.ToInt32(e.GetAttribute("minValueR"));
						if (e.HasAttribute("maxValueR"))
							iinfo.MaxValueR = Convert.ToInt32(e.GetAttribute("maxValueR"));
						if (e.HasAttribute("rangeR"))
							iinfo.RangeR = Convert.ToInt32(e.GetAttribute("rangeR"));

						iinfo.SingleInput = e.HasAttribute("singleInput");

						if (e.HasAttribute("mapping"))
							iinfo.Mapping = (InputMapping)(Convert.ToInt32(e.GetAttribute("mapping")));

						nodeInfo.InputInfos.Add(iinfo);

						foreach (XmlNode cn in e.ChildNodes)
						{ 
							XmlElement te = cn as XmlElement;
							if (te.Name == "ModeGroup")
							{
								ModeGroup group = new ModeGroup();
								group.Name = te.GetAttribute("name");
								group.Mask = Convert.ToInt32(te.GetAttribute("mask"));
								group.Shift = Convert.ToInt32(te.GetAttribute("shift"));
								if (te.HasAttribute("hidemodetext"))
									group.ShowModeText = false;

								XmlNodeList il = te.SelectNodes("ModeItem");
								foreach (XmlNode mi in il)
								{
									te = mi as XmlElement;

									ModeItem item = new ModeItem(group);
									item.Name = te.GetAttribute("name");
									item.Value = Convert.ToInt32(te.GetAttribute("value"));
									if (te.HasAttribute("visibility"))
										item.Visibility = Convert.ToInt32(te.GetAttribute("visibility"));

									group.Items.Add(item);

								}
								iinfo.Modes.Add(group);
							}

							if (te.Name == "ModeFlag")
							{
								ModeItem item = new ModeItem();
								item.Name = te.GetAttribute("name");
								item.Value = Convert.ToInt32(te.GetAttribute("value"));
								if (te.HasAttribute("visibility"))
									item.Visibility = Convert.ToInt32(te.GetAttribute("visibility"));

								iinfo.Modes.Add(item);
							}
						}
					}
					// need to fill up emtpy reserved slots when hitting constant or other holes in the id sequence
					while (NodeInfoList.Count < nodeInfo.TypeID)
					{
						NodeInfoList.Add(new NodeInfo());
					}
					NodeInfoList.Add(nodeInfo);
				}
			}

			return NodeInfoList[typeid];
		}

		public static int ConstantTypeID()
		{
			return 64;
		}

		public static int MaxTypeID()
		{
			return NodeInfoList.Count;
		}

		public int TypeID;
		public string Name;
		public int NumInputs;
		public int NumReqGUISignals;
		public int NumMaxGUISignals;		
		public bool AllowSignalInsertion;
		public bool VariableInput;
		public List<InputInfo> InputInfos;

		public NodeInfo()
		{
			InputInfos = new List<InputInfo>();
			AllowSignalInsertion = false;
			VariableInput = false;
		}
	};

#endregion

}
