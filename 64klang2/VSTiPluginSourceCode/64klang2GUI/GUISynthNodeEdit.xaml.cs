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

namespace _64klang2GUI
{
	/// <summary>
	/// Interaction logic for GUISynthNodeEdit.xaml
	/// </summary>
	public partial class GUISynthNodeEdit : UserControl
	{
		public GUISynthNode Node { get; private set; }

		private Dictionary<object, ModeItem> ControlItemMap;
		private Dictionary<object, ModeGroup> ControlGroupMap;

		private ComboBox SpecialFlags;
		private Knob SpecialKnob;

		public ComboBox WavetableComboBox;

		public GUISynthNodeEdit(GUISynthNode synthNode)
		{
			InitializeComponent();

			Node = synthNode;
			ControlItemMap = new Dictionary<object, ModeItem>();
			ControlGroupMap = new Dictionary<object, ModeGroup>();
			SpecialFlags = null;
			SpecialKnob = null;
			WavetableComboBox = null;
			NodeEditName.Content = synthNode.NodeName.Content.ToString();

			// special case for constants
			if (Node.Info.TypeID == NodeInfo.ConstantTypeID())
			{
				// remove all but one knob row
				for (int i = 0; i < 15; i++)
					NodeEditVerticalStackPanel.Children.RemoveAt(NodeEditVerticalStackPanel.Children.Count - 2);
				Grid grid = NodeEditVerticalStackPanel.Children[2] as Grid;
				TextBox inputName = grid.Children[0] as TextBox;
				Knob inputKnob = grid.Children[1] as Knob;
				InputInfo info = new InputInfo();
				info.MinValueL = -128;
				info.MaxValueL = 128;
				info.RangeL = 128;
				info.MinValueR = -128;
				info.MaxValueR = 128;
				info.RangeR = 128;
				info.SingleInput = false;
				inputKnob.Init(this, - 1, info, Node.InitialValues[0].LeftValue, Node.InitialValues[0].RightValue, false, true);
				inputKnob.valueChangedHandler += valueChangedCB;
				inputName.Text = "Value";
			}
			else
			{
				// hide signal inputs
				for (int i = 0; i < Node.Info.NumReqGUISignals; i++)
				{
					NodeEditVerticalStackPanel.Children[2 + i].Visibility = System.Windows.Visibility.Collapsed;
				}
				// adjust names for parameters
				int MaxGUISignals = Node.Info.NumMaxGUISignals;

				// need a scale knob for the mode constant?
				if (Node.Info.TypeID == 14 || // scaler
					Node.Info.TypeID == 29 || // midisignal
					Node.Info.TypeID == 35 || // osrand
					Node.Info.TypeID > NodeInfo.ConstantTypeID()) // voice constants
				{
					MaxGUISignals++;
				}

				for (int i = Node.Info.NumReqGUISignals; i < MaxGUISignals; i++)
				{
					InputInfo info = Node.Info.InputInfos[i];
					// different range for midisignal and voice constants
					if (Node.Info.TypeID == 29 || Node.Info.TypeID > NodeInfo.ConstantTypeID())
					{
						info.MinValueL = -128;
						info.MaxValueL = 128;
						info.RangeL = 128;
						info.MinValueR = -128;
						info.MaxValueR = 128;
						info.RangeR = 128;
					}

					Grid grid = NodeEditVerticalStackPanel.Children[2 + i] as Grid;
					TextBox inputName = grid.Children[0] as TextBox;
					Knob inputKnob = grid.Children[1] as Knob;
					bool editable = false;
					// scale has editable as default
					if (Node.Info.TypeID == 14)
						editable = true;
					inputKnob.Init(this, i, info, Node.InitialValues[i].LeftValue, Node.InitialValues[i].RightValue, false, editable);
					inputKnob.valueChangedHandler += valueChangedCB;
					inputName.Text = info.Name;
				}
				// remove the rest
				for (int i = MaxGUISignals; i < 16; i++)
					NodeEditVerticalStackPanel.Children.RemoveAt(NodeEditVerticalStackPanel.Children.Count - 2);

				// add mode
				if (MaxGUISignals < Node.Info.NumInputs)
				{
					InputInfo info = Node.Info.InputInfos[MaxGUISignals];
					// trigger sequencer has several modes
					if (Node.Info.TypeID == 40)
					{
						// add the pattern count knob
						RowDefinition rowdef = new RowDefinition();
						rowdef.Height = GridLength.Auto;
						ModeGrid.RowDefinitions.Add(rowdef);

						Label label = new Label();
						label.Content = info.Name;
						label.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
						ModeGrid.Children.Add(label);
						Grid.SetRow(label, 0);
						Grid.SetColumn(label, 0);

						SpecialKnob = new Knob();
						int patterncount = Node.InitialValues[4].ModeValue & 0xff;
						SpecialKnob.Init(this, 4, info, patterncount / 16.0, patterncount / 16.0, false);
						SpecialKnob.valueChangedHandler += bitpatternCountChangedCB;
						ModeGrid.Children.Add(SpecialKnob);
						Grid.SetRow(SpecialKnob, 0);
						Grid.SetColumn(SpecialKnob, 1);

						// add the bpm sync combobox (put it in the knob layout)
						StackPanel hpanel = new StackPanel();
						hpanel.Orientation = Orientation.Horizontal;
						SpecialFlags = new ComboBox();						
						string[] BPM_TIME_NAME = new string[]
						{	
							"Trigger Only",
							"1/64T",								
							"1/128D",								
							"1/64",								
							"1/32T",
							"1/64D",
							"1/32",								
							"1/16T",
							"1/32D",
							"1/16",								
							"1/8T",
							"1/16D",
							"1/8",								
							"1/4T",
							"1/8D",
							"1/4",								
							"1/2T",
							"1/4D",
							"1/2",								
							"1T",
							"1/2D",
							"1",
							"1D",
							"3/8",
							"5/8",
							"7/8",
							"9/8",
							"11/8",
							"13/8",
							"15/8",
							"3/4",
							"5/4",
							"7/4",							
						};
						foreach (string cbi in BPM_TIME_NAME) SpecialFlags.Items.Add(cbi);
						SpecialFlags.SelectedIndex = Node.InitialValues[4].ModeValue >> 8;
						SpecialFlags.SelectionChanged += new SelectionChangedEventHandler(SpecialFlags_SelectionChanged);

						Label tlabel = new Label();
						tlabel.Content = "BPM Sync";

						hpanel.VerticalAlignment = System.Windows.VerticalAlignment.Center;
						hpanel.Children.Add(tlabel);
						hpanel.Children.Add(SpecialFlags);

						SpecialKnob.KnobGrid.Children.Add(hpanel);
						Grid.SetRow(hpanel, 0);
						Grid.SetColumn(hpanel, 2);
						Grid.SetColumnSpan(hpanel, 2);

						for (int i = 0; i < 16; i++)
						{
							rowdef = new RowDefinition();
							rowdef.Height = GridLength.Auto;
							ModeGrid.RowDefinitions.Add(rowdef);

							if (i < (Node.InitialValues[4].ModeValue & 0xff))
								rowdef.Height = GridLength.Auto;
							else
								rowdef.Height = new GridLength(0);

							label = new Label();
							label.Content = "Pattern " + i.ToString() + ":";
							label.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
							ModeGrid.Children.Add(label);
							Grid.SetRow(label, 1+i);
							Grid.SetColumn(label, 0);

							BitPattern bp = new BitPattern();
							int model = Node.InitialValues[5 + i/4 + 0].ModeValue;
							int moder = Node.InitialValues[5 + i/4 + 4].ModeValue;
							bp.Init(5 + i / 4, i % 4, (uint)model, (uint)moder);
							bp.valueChangedHandler += bitpatternChangedCB;
							ModeGrid.Children.Add(bp);
							Grid.SetRow(bp, 1+i);
							Grid.SetColumn(bp, 1);
						}						
					}
					// samplerec has record time as knob and reset button
					else if (Node.Info.TypeID == 41)
					{
						RowDefinition rowdef = new RowDefinition();
						rowdef.Height = GridLength.Auto;
						ModeGrid.RowDefinitions.Add(rowdef);

						Label label = new Label();
						label.Content = info.Name;
						label.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
						ModeGrid.Children.Add(label);
						Grid.SetRow(label, 0);
						Grid.SetColumn(label, 0);

						Knob knob = new Knob();
						int mode = Node.InitialValues[MaxGUISignals].ModeValue >> 12;
						double time = mode / (1024.0 * 512.0);
						knob.Init(this, MaxGUISignals, info, time, time, true);
						knob.valueChangedHandler += recordTimeChangedCB;
						ModeGrid.Children.Add(knob);
						Grid.SetRow(knob, 0);
						Grid.SetColumn(knob, 1);

						// put button in knob layout
						Button resetSampleRec = new Button();
						resetSampleRec.MaxHeight = 25.0;
						resetSampleRec.Content = "Reset";
						resetSampleRec.Click += new RoutedEventHandler(resetSampleRec_Click);
						knob.KnobGrid.Children.Add(resetSampleRec);
						Grid.SetRow(resetSampleRec, 0);
						Grid.SetColumn(resetSampleRec, 2);
						Grid.SetColumnSpan(resetSampleRec, 2);
					}
					else
					{
						int initialBits = Node.InitialValues[MaxGUISignals].ModeValue;

						StackPanel flagsp = null;

						int mgIndex = 0;
						foreach (object o in info.Modes)
						{
							ModeGroup mg = o as ModeGroup;							
							if (mg != null)
							{
								RowDefinition rowdef = new RowDefinition();
								rowdef.Height = GridLength.Auto;
								ModeGrid.RowDefinitions.Add(rowdef);

								StackPanel sp = new StackPanel();
								sp.Orientation = Orientation.Horizontal;
								
								int currentBits = (initialBits & mg.Mask) >> mg.Shift;								

								// up to 4 modes use buttons
								if (mg.Items.Count <= 4)
								{
									foreach (ModeItem tmi in mg.Items)
									{
										ModeButton button = new ModeButton();
										button.Content = tmi.Name;
										button.GroupName = mg.Name + Node.ID.ToString();
										if (currentBits == tmi.Value)
											button.IsChecked = true;
										else
											button.IsChecked = false;
										button.Checked += new RoutedEventHandler(button_Checked);
										// hide mode if visibility restriction is not matching
										if (((tmi.Visibility == 1) && Node.IsGlobal) || ((tmi.Visibility == 2) && !Node.IsGlobal))
										{
											button.IsEnabled = false;
											button.Opacity = 0.1;
										}

										sp.Children.Add(button);
										ControlItemMap.Add(button, tmi);
									}
								}
								// combobox for more than 4 items
								else
								{
									ComboBox cb = new ComboBox();
									int i = 0, selected = 0;
									foreach (ModeItem tmi in mg.Items)
									{
										string iname = tmi.Name; 
										
										// special case for sample player where the wavetable names are not read from the modegroup
                                        if (((Node.Info.TypeID == 42) || (Node.Info.TypeID == 56)) && (mg.Name == "Wavetable"))
										{
											iname = Node.synthCanvas.MainWindow.GetWaveFileName(i);
											WavetableComboBox = cb;
                                            // hide wavetable selector for input on sampler and wtf osc nodes
                                            int mode = CurrentMode() & 0x00000007;
                                            if (mode == 0)
                                                WavetableComboBox.Visibility = Visibility.Visible;
                                            else                
                                                WavetableComboBox.Visibility = Visibility.Hidden;
										}

										ComboBoxItem cbi = new ComboBoxItem();
										cbi.Content = iname;
										cb.Items.Add(cbi);
										if (currentBits == tmi.Value)
											selected = i;
										i++;
									}
									cb.SelectedIndex = selected;
									cb.SelectionChanged += new SelectionChangedEventHandler(cb_SelectionChanged);
									sp.Children.Add(cb);
									ControlGroupMap.Add(cb, mg);

								}

								Label label = new Label();
								label.Content = mg.Name;
								label.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
								label.HorizontalContentAlignment = System.Windows.HorizontalAlignment.Left;
								label.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
								label.VerticalAlignment = System.Windows.VerticalAlignment.Stretch;
								ModeGrid.Children.Add(label);
								Grid.SetRow(label, mgIndex);
								Grid.SetColumn(label, 0);

								ModeGrid.Children.Add(sp);
								Grid.SetRow(sp, mgIndex);
								Grid.SetColumn(sp, 1);

								mgIndex++;
							}

							ModeItem mi = o as ModeItem;
							if (mi != null)
							{
								// onetime creation of the flag stack panel
								if (flagsp == null)
								{
									RowDefinition rowdef = new RowDefinition();
									rowdef.Height = GridLength.Auto;
									ModeGrid.RowDefinitions.Add(rowdef);

									flagsp = new StackPanel();
									flagsp.Orientation = Orientation.Horizontal;
									
									Label label = new Label();
									label.Content = "Flags";
									label.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
									label.HorizontalContentAlignment = System.Windows.HorizontalAlignment.Left;
									label.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
									label.VerticalAlignment = System.Windows.VerticalAlignment.Stretch;
									ModeGrid.Children.Add(label);
									Grid.SetRow(label, mgIndex);
									Grid.SetColumn(label, 0);

									ModeGrid.Children.Add(flagsp);
									Grid.SetRow(flagsp, mgIndex);
									Grid.SetColumn(flagsp, 1);

									mgIndex++;
								}

								CheckBox cb = new CheckBox();
								cb.VerticalAlignment = System.Windows.VerticalAlignment.Center;
								cb.Content = mi.Name;
								if ((initialBits & mi.Value) != 0)
									cb.IsChecked = true;
								else
									cb.IsChecked = false;
								cb.Checked += new RoutedEventHandler(cb_Checked);
								cb.Unchecked += new RoutedEventHandler(cb_Unchecked);
								// hide flag if visibility restriction is not matching
								if (((mi.Visibility == 1) && Node.IsGlobal) || ((mi.Visibility == 2) && !Node.IsGlobal))
								{
									cb.IsEnabled = false;
									cb.Opacity = 0.1;
								}
								flagsp.Children.Add(cb);
								ControlItemMap.Add(cb, mi);
							}
						}		
					}

					// voicemanager needs arpeggiator edit
					if (Node.Info.TypeID == 3)
					{
						ArpeggiatorEdit arpe = new ArpeggiatorEdit(Node);
						arpe.ReadData();
						SpecialGrid.Children.Add(arpe);
					}

					// texttospeech needs textinput (doesnt have modes, therefore its inserted in special grid)
					if (Node.Info.TypeID == 50)
					{
						StackPanel sp = new StackPanel();
						sp.Orientation = Orientation.Vertical;

						TextBox tb = new TextBox();
						tb.Text = Node.synthCanvas.GetSAPIText(Node.ID);
						tb.Width = 200;
						tb.Height = 100;
						tb.AcceptsReturn = true;
						tb.TextWrapping = TextWrapping.Wrap;
						tb.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
						sp.Children.Add(tb);

						Button b = new Button();
						b.Content = "Update";
						b.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
						b.ToolTip = "For markup options see: http://msdn.microsoft.com/en-us/library/ms717077(v=vs.85).aspx";
						b.Click += new RoutedEventHandler(SAPI_Update_Click);
						sp.Children.Add(b);

						SpecialGrid.Children.Add(sp);
					}

					// formula needs textinput (doesnt have modes, therefore its inserted in special grid)
					if (Node.Info.TypeID == 54)
					{
						double width = 300;

						StackPanel sp = new StackPanel();
						sp.Orientation = Orientation.Vertical;

						TextBox tb = new TextBox();
						tb.Text = Node.synthCanvas.GetFormulaText(Node.ID);
						tb.Width = width;
						tb.Height = 150;
						tb.AcceptsReturn = true;
						tb.AcceptsTab = true;
						tb.TextWrapping = TextWrapping.Wrap;
						tb.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
                        tb.ToolTip =
                            "General:\n" +                            
                            "\tFormula MUST contain one expression where the output is assigned to variable 'out'. E.g: 'out=1;'\n" +
                            "\tAll expressions must be terminated with ';'\n" +
                            "\tArbitrary number of () braces allowed. As are spaces tabs and newlines for formatting\n" +
                            "\tUp to 22 variables with arbitraty name can be (re)assigned and used in expressions\n" +                                                         
                            "\t'out' is one default variable. The other one is 'time' which contains the current time in seconds.\n" +
                            "Arithmetic Operators:\n" +
                            "\t+ - * / %\n" +
                            "Logic Operators(for use in condition of ifthen)\n" +
                            "\t== != > >= < <= && ||\n" +
                            "Arithmetic Functions:\n" +
                            "\tmin(x,y) max(x,y) abs(x) sqrt(x) sqr(x) ceil(x) floor(x) sin(x) cos(x) exp2(x) log2(x) lerp(x,y,t)\n" +
                            "Special Functions:\n" +
                            "\tifthen(condition,a,b) return expression a or b based on condition expression (see logic ops)\n" +
                            "\trand()\t\treturn a random number\n" +
                            "\tpi()\t\treturn the number pi\n" +
                            "\ttau()\t\treturn the number tau (2*pi)\n" +
                            "\ttautime()\t\treturn 'time' multiplied by tau()\n" +
                            "\tin0()\t\treturn the current value connected at input0\n" +
                            "\tin1()\t\treturn the current value connected at input1\n" +
                            "\tvfrequency()\treturn the current voice frequency (in Hz)\n" +
                            "\tvnote()\t\treturn the current voice note value (normalized from 0..1)\n" +
                            "\tvvelocity()\treturn the current voice velocity value (normalized from 0..1)\n" +
                            "\tvtrigger()\treturn the current voice trigger signal (1 only at the first sample, then 0)\n" +
                            "\tvgate()\t\treturn the current voice gate signal (1 as long as key pressed, then 0)\n" +
                            "\tvaftertouch()\treturn the current voice aftertouch signal (normalized from 0..1)\n" +
							"\ttrisaw(p,c)\treturn a trisaw wave with the given phase and color (both must be in range 0..1)\n" +
							"\tpulse(p,c)\treturn a pulse wave with the given phase and color (both must be in range 0..1)\n" +
                            "\tmaxtime(time,a)\treturn expression a within time seconds, else 0. signals event for that time as well\n" +
                            "Examples:\n" +
                            "\tSine with 440hz\n" +
                            "\t\tout=sin(tautime()*440);\n" +
                            "\t2 sines with 440 and 880hz\n" +
                            "\t\tmyfreq=tautime()*440;\n" +
                            "\t\tout=sin(myfreq)+sin(2*myfreq);\n" +
                            "\tAD envelope with 0.25s attack and 1s decay. Can be used instead of default ADSR for VoiceRoot Input\n" +
                            "\t\tout=maxtime(1.25,ifthen(time<0.25,time*4,1.25-time));\n";                            

                       // "maxtime(1.0,ifthen(time()<0.25,time()*4,(time()-0.25)))"
                        ToolTipService.SetShowDuration(tb, 10000);
						sp.Children.Add(tb);

						Button b = new Button();
						b.Content = "Update";
						b.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
//                        b.ToolTip = "For markup options see: http://msdn.microsoft.com/en-us/library/ms717077(v=vs.85).aspx";
						b.Click += new RoutedEventHandler(Formula_Update_Click);
						sp.Children.Add(b);

						ComboBox cb = new ComboBox();
						cb.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
						cb.Items.Add(new ComboBoxItem() { Content = "0.01" });
						cb.Items.Add(new ComboBoxItem() { Content = "0.025" });
						cb.Items.Add(new ComboBoxItem() { Content = "0.05" });
						cb.Items.Add(new ComboBoxItem() { Content = "0.1" });
						cb.Items.Add(new ComboBoxItem() { Content = "0.25" });
						cb.Items.Add(new ComboBoxItem() { Content = "0.5" });
						cb.Items.Add(new ComboBoxItem() { Content = "1.0" });
						cb.Items.Add(new ComboBoxItem() { Content = "2.5" });
						cb.Items.Add(new ComboBoxItem() { Content = "5.0" });
						cb.Items.Add(new ComboBoxItem() { Content = "10.0" });
						cb.SelectedIndex = 3;
						sp.Children.Add(cb);

                        Canvas c = new Canvas() { ClipToBounds = true, Width = width, Height = 200, Background = Brushes.Black };
						sp.Children.Add(c);

						SpecialGrid.Children.Add(sp);
					}

                    // update button for the wtf oscillator
                    if (Node.Info.TypeID == 56)
                    {
                        StackPanel sp = new StackPanel();
                        sp.Orientation = Orientation.Vertical;

                        Button b = new Button();
                        b.Content = "Update";
                        b.HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch;
                        b.ToolTip = "Rescan the input wavetable based on the current scan settings.";
                        b.Click += new RoutedEventHandler(WTFOSC_Update_Click);
                        sp.Children.Add(b);

                        SpecialGrid.Children.Add(sp);
                    }

#if false
					// bqf response curve
					if (Node.Info.TypeID == 9)
					{
						SpecialGrid.Height = 200;
						SpecialGrid.Background = Brushes.Black;

						// the response curves
						PathFigure pathFigure;
						PathGeometry pathGeo;

						pathFigure = new PathFigure();
						pathFigure.StartPoint = new Point(0, 0);
						for (int i = 1; i <= 128; i++)
						{
							pathFigure.Segments.Add(new LineSegment(new Point(i * 3, i), true));
						}
						pathGeo = new PathGeometry();
						pathGeo.Figures.Add(pathFigure);
						Path pathl = new Path();
						pathl.Stroke = Brushes.Lime;
						pathl.StrokeThickness = 2;
						pathl.Data = pathGeo;


						pathFigure = new PathFigure();
						pathFigure.StartPoint = new Point(0, 0);
						for (int i = 1; i <= 128; i++)
						{
							pathFigure.Segments.Add(new LineSegment(new Point(i * 3, i), true));
						}
						pathGeo = new PathGeometry();
						pathGeo.Figures.Add(pathFigure);
						Path pathr = new Path();
						pathr.Stroke = Brushes.Red;
						pathr.StrokeThickness = 2;
						pathr.Data = pathGeo;

						SpecialGrid.Children.Add(pathl);
						SpecialGrid.Children.Add(pathr);

						// base line
						Line one = new Line();
						one.X1 = 0;
						one.Y1 = 50;
						one.X2 = 400;
						one.Y2 = 50;
						one.Stroke = Brushes.White;
						one.StrokeThickness = 2;
						SpecialGrid.Children.Add(one);

						updateBQFResponse();
					}
#endif
				}				
			}

			UpdateLabels();
			UpdateKnobsText();

            CompositionTarget.Rendering += CompositionTarget_Rendering;
		}

        ~GUISynthNodeEdit()
		{
			KillTimer();
		}

		public void KillTimer()
		{
            CompositionTarget.Rendering -= CompositionTarget_Rendering;
		}

		int SeqLastPattern = -1;
		int SeqLastStep = -1;

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
			Node.synthCanvas.MainWindow.QueryCoreProcessingMutex(true);

			// trigger sequencer position
			if (Node.Info.TypeID == 40)
			{
				double step = Node.synthCanvas.GetNodeValue(Node.ID, 0, 0);
				double pattern = Node.synthCanvas.GetNodeValue(Node.ID, 1, 0);
				int s = (int)step;
				switch (s)
				{
					case   1: s = 0; break;
					case   2: s = 1; break;
					case   4: s = 2; break;
					case   8: s = 3; break;
					case  16: s = 4; break;
					case  32: s = 5; break;
					case  64: s = 6; break;
					case 128: s = 7; break;
				}
				int p = (int)pattern;
				if (p != SeqLastPattern || s != SeqLastStep)
				{
					BitPattern bp = ModeGrid.Children[2 + p*2 + 1] as BitPattern;
					if (bp != null)
					{					
						CheckBox cbl = bp.TickGrid.Children[s] as CheckBox;
						CheckBox cbr = bp.TickGrid.Children[s + 8] as CheckBox;
						cbl.Background = Brushes.DarkGray;
						cbr.Background = Brushes.DarkGray;
						// clear old markers
						if (SeqLastPattern >= 0 && SeqLastStep >= 0)
						{
							bp = ModeGrid.Children[2 + SeqLastPattern*2 + 1] as BitPattern;
							if (bp != null)
							{
								cbl = bp.TickGrid.Children[SeqLastStep] as CheckBox;
								cbr = bp.TickGrid.Children[SeqLastStep + 8] as CheckBox;
								cbl.Background = Brushes.White;
								cbr.Background = Brushes.White;
							}
						}						
						SeqLastPattern = p;
						SeqLastStep = s;
					}
				}
			}

			// midi signal
			if (Node.Info.TypeID == 29)
			{
				Grid grid = NodeEditVerticalStackPanel.Children[2] as Grid;
				TextBox inputName = grid.Children[0] as TextBox;
				Knob inputKnob = grid.Children[1] as Knob;
				double left = Node.synthCanvas.GetNodeSignal(Node.ID, 0, -3);
				double right = Node.synthCanvas.GetNodeSignal(Node.ID, 1, -3);
				inputKnob.UpdateModMarkers(left, right);
			}
			// voice signals
			if (Node.Info.TypeID > NodeInfo.ConstantTypeID())
			{
				Grid grid = NodeEditVerticalStackPanel.Children[2] as Grid;
				TextBox inputName = grid.Children[0] as TextBox;
				Knob inputKnob = grid.Children[1] as Knob;
				double left = Node.synthCanvas.GetNodeSignal(Node.ID, 0, -4);
				double right = Node.synthCanvas.GetNodeSignal(Node.ID, 1, -4);
				inputKnob.UpdateModMarkers(left, right);
			}

			for (int i = Node.Info.NumReqGUISignals; i < Node.Info.NumMaxGUISignals; i++)
			{
				Grid grid = NodeEditVerticalStackPanel.Children[2 + i] as Grid;
				TextBox inputName = grid.Children[0] as TextBox;
				Knob inputKnob = grid.Children[1] as Knob;
				double left = Node.synthCanvas.GetNodeSignal(Node.ID, 0, i);
				double right = Node.synthCanvas.GetNodeSignal(Node.ID, 1, i);
				inputKnob.UpdateModMarkers(left, right);
			}

			Node.synthCanvas.MainWindow.QueryCoreProcessingMutex(false);
		}
		
		// samplerecord needs a way to retrigger whole calculation, too many things to reset, so just call a panic :)
		private static bool resettingSampleRec = false;
		void resetSampleRec_Click(object sender, RoutedEventArgs e)
		{
			if (resettingSampleRec)
				return;
			resettingSampleRec = true;
			if (Node.synthCanvas.MainWindow.panicHandler != null)
				Node.synthCanvas.MainWindow.panicHandler();
			resettingSampleRec = false;
		}

		void SAPI_Update_Click(object sender, RoutedEventArgs e)
		{
			StackPanel sp = SpecialGrid.Children[0] as StackPanel;
			TextBox tb = sp.Children[0] as TextBox;
			Node.synthCanvas.SetSAPIText(Node.ID, tb.Text);
		}

		void Formula_Update_Click(object sender, RoutedEventArgs e)
		{
			StackPanel sp = SpecialGrid.Children[0] as StackPanel;
			TextBox tb = sp.Children[0] as TextBox;
			ComboBox cb = sp.Children[2] as ComboBox;
			Canvas c = sp.Children[3] as Canvas;
			String formula = tb.Text;
			formula = formula.Replace(" ", "");
			formula = formula.Replace("\n", "");
            formula = formula.Replace("\r", "");
            formula = formula.Replace("\t", "");
            string[] statements = formula.Split(new char[] { ';' });

			// restrict expression parser to tokens we support
			MultiParse.Expression exp = new MultiParse.Expression(
				MultiParse.Default.MPDefault.DataTypes.Double | 
				MultiParse.Default.MPDefault.DataTypes.Boolean |
                MultiParse.Default.MPDefault.DataTypes.Variable,
				
				MultiParse.Default.MPDefault.Operators.Addition |
                MultiParse.Default.MPDefault.Operators.Assignment |                
				MultiParse.Default.MPDefault.Operators.ConditionalAnd |
				MultiParse.Default.MPDefault.Operators.ConditionalOr |
				MultiParse.Default.MPDefault.Operators.Division |
				MultiParse.Default.MPDefault.Operators.Equality |
				MultiParse.Default.MPDefault.Operators.Inequality |
				MultiParse.Default.MPDefault.Operators.Modulo |
				MultiParse.Default.MPDefault.Operators.Multiplication |
				MultiParse.Default.MPDefault.Operators.Negative |
				MultiParse.Default.MPDefault.Operators.RelationalLarger |
				MultiParse.Default.MPDefault.Operators.RelationalLargerEqual |
                MultiParse.Default.MPDefault.Operators.RelationalSmaller |
                MultiParse.Default.MPDefault.Operators.RelationalSmallerEqual |                
				MultiParse.Default.MPDefault.Operators.Subtraction,
				
				MultiParse.Default.MPDefault.Functions.Abs |
				MultiParse.Default.MPDefault.Functions.Ceiling |
				MultiParse.Default.MPDefault.Functions.Cos |				
				MultiParse.Default.MPDefault.Functions.Floor |				
				MultiParse.Default.MPDefault.Functions.Max |
				MultiParse.Default.MPDefault.Functions.Min |				
				MultiParse.Default.MPDefault.Functions.Sin |				
				MultiParse.Default.MPDefault.Functions.Sqrt
                // synth related are added always
                //MultiParse.Default.MPDefault.Functions.Exp2
                //MultiParse.Default.MPDefault.Functions.IfThen
				//MultiParse.Default.MPDefault.Functions.In0
				//MultiParse.Default.MPDefault.Functions.In1
                //MultiParse.Default.MPDefault.Functions.Lerp
				//MultiParse.Default.MPDefault.Functions.Log2
                //MultiParse.Default.MPDefault.Functions.Out
				//MultiParse.Default.MPDefault.Functions.Pi
				//MultiParse.Default.MPDefault.Functions.Rand
                //MultiParse.Default.MPDefault.Functions.Sqr
                //MultiParse.Default.MPDefault.Functions.Tau
                //MultiParse.Default.MPDefault.Functions.Time
                //MultiParse.Default.MPDefault.Functions.Tautime
                //MultiParse.Default.MPDefault.Functions.MaxTime
                //MultiParse.Default.MPDefault.Functions.VFrequency
                //MultiParse.Default.MPDefault.Functions.VNote
                //MultiParse.Default.MPDefault.Functions.VVelocity
                //MultiParse.Default.MPDefault.Functions.VTrigger
                //MultiParse.Default.MPDefault.Functions.VGate
                //MultiParse.Default.MPDefault.Functions.VAftertouch
                //MultiParse.Default.MPDefault.Functions.Last0
                //MultiParse.Default.MPDefault.Functions.Last1
			);
            // disable variable auto generation
            MultiParse.MPDataType[] vars = exp.FindDataType(typeof(MultiParse.Default.MPVariable));
            //(vars[0] as MultiParse.Default.MPVariable).AutoGenerate = false;
            // Add the default available variables with a value of the double 0.0
            MultiParse.Default.MPVariableInstance outvar = new MultiParse.Default.MPVariableInstance((double)0.0, "out");
            (vars[0] as MultiParse.Default.MPVariable).Variables.Add("out", outvar);
            MultiParse.Default.MPVariableInstance timevar = new MultiParse.Default.MPVariableInstance((double)0.0, "time");
            (vars[0] as MultiParse.Default.MPVariable).Variables.Add("time", timevar);

            // remapping table for arbitrary variable names
            Dictionary<string, string> varRemap = new Dictionary<string, string>();
            varRemap["="] = "=";
            varRemap["+"] = "+";
            varRemap["*"] = "*";
            varRemap["=="] = "==";
            varRemap["!="] = "!=";
            varRemap[">"] = ">";
            varRemap[">="] = ">=";
            varRemap["&&"] = "&&";
            varRemap["||"] = "||";
            varRemap["<"] = "<";
            varRemap["<="] = "<=";
            varRemap["-"] = "-";
            varRemap["/"] = "/";
            varRemap["[mM]in"] = "[mM]in";
            varRemap["[mM]ax"] = "[mM]ax";
            varRemap["%"] = "%";
            varRemap["[lL]erp"] = "[lL]erp";
            varRemap["[iI]fthen"] = "[iI]fthen";
            varRemap["[mM]axtime"] = "[mM]axtime";
            varRemap["Neg"] = "Neg";
            varRemap["[aA]bs"] = "[aA]bs";
            varRemap["[sS]qrt"] = "[sS]qrt";
            varRemap["[cC]eil"] = "[cC]eil";
            varRemap["[fF]loor"] = "[fF]loor";
            varRemap["[sS]qr"] = "[sS]qr";
            varRemap["[cC]os"] = "[cC]os";
            varRemap["[sS]in"] = "[sS]in";
            varRemap["[eE]xp2"] = "[eE]xp2";
            varRemap["[lL]og2"] = "[lL]og2";
            varRemap["[rR]and"] = "[rR]and";
            varRemap["[pP]i"] = "[pP]i";
            varRemap["[tT]au"] = "[tT]au";
            varRemap["[tT]autime"] = "[tT]autime";
            varRemap["[iI]n0"] = "[iI]n0";
            varRemap["[iI]n1"] = "[iI]n1";
            varRemap["[vV]frequency"] = "[vV]frequency";
            varRemap["[vV]note"] = "[vV]note";
            varRemap["[vV]velocity"] = "[vV]velocity";
            varRemap["[vV]trigger"] = "[vV]trigger";
            varRemap["[vV]gate"] = "[vV]gate";
            varRemap["[vV]aftertouch"] = "[vV]aftertouch";
			varRemap["[tT]risaw"] = "[tT]risaw";
			varRemap["[pP]ulse"] = "[pP]ulse";
            varRemap["out"] = "out";
            varRemap["time"] = "time";
            List<string> availableVars = new List<string>() { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v" };            

            object res = null;
            String rpn = "";
            MultiParse.Expression.Time = 0.0;
            MultiParse.Expression.In0 = -1.0;
            MultiParse.Expression.In1 = 1.0;
            foreach (string statement in statements)
            {
                if (statement == "")
                    continue;
                exp.ParseExpression = statement;
                // evaluate formula
                try
                {                    
                    res = exp.Evaluate();
                    string trpn = exp.RPN.Replace(',', '.');
                    // adjust the RPN                    

                    List<string> parts = trpn.Split(new char[] { ':' }).ToList();
                    if (parts.Count > 0 && parts[parts.Count - 1] == "")
                        parts.RemoveAt(parts.Count - 1);

                    // handle variable assignments and rename the username to our vars (a..v)
                    for (int i = 0; i < parts.Count; i++)
                    {
                        if (!varRemap.ContainsKey(parts[i]))
                        {
                            // if its not a double it is a variable not known yet
                            double tout;
                            if (!double.TryParse(parts[i], System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out tout))
                            {
                                if (availableVars.Count == 0)
                                {
                                    MessageBox.Show("Formula Error:" + Environment.NewLine + "Too many variables used");
                                    return;
                                }
                                varRemap[parts[i]] = availableVars[0];
                                availableVars.RemoveAt(0);
                                parts[i] = varRemap[parts[i]];
                            }
                        }
                        else 
                        {
                            parts[i] = varRemap[parts[i]];
                        }
                    }

                    // build new rpn string
                    trpn = "";
                    for (int pi = 0; pi < parts.Count; pi++)
                    {                        
                        double tout;
                        // change number,neg combination to simply the negative number
                        if (double.TryParse(parts[pi], System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out tout))
                        {
                            if (pi+1 < parts.Count)
                            {
                                if (parts[pi+1] == "Neg")
                                {
                                    parts[pi] = "-" + parts[pi];
                                    parts.RemoveAt(pi+1);
                                }
                            }
                        }
                        trpn += parts[pi] + ":";
                    }

                    // transform assignment expressions to better format
                    // e.g. 
                    // a:44:[tT]autime:*:=: 
                    // becomes
                    // 44:[tT]autime:*:a=: 
                    if (trpn.EndsWith("=:"))
                    {                       
                        int varend = trpn.IndexOf(":");
                        string varname = trpn.Substring(0, varend);
                        trpn = trpn.Substring(varend+1);
                        trpn = trpn.Substring(0, trpn.Length - 2);
                        trpn += varname + "=:";
                    }
                    rpn += trpn;                    
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Formula Error:" + Environment.NewLine + ex.Message);
                    return;
                }
            }

            // store formula in core
            Node.synthCanvas.SetFormulaText(Node.ID, tb.Text, rpn);

			// plot the formula
            Mouse.OverrideCursor = Cursors.Wait;
			try
			{
				c.Children.Clear();
				// 1 0 -1 lines
				c.Children.Add(new Line() { X1 = 0, Y1 = 25, X2 = c.Width, Y2 = 25, Stroke = Brushes.LightGray, StrokeThickness = 1 });
                c.Children.Add(new Line() { X1 = 0, Y1 = 100, X2 = c.Width, Y2 = 100, Stroke = Brushes.LightGray, StrokeThickness = 2 });
                c.Children.Add(new Line() { X1 = 0, Y1 = 175, X2 = c.Width, Y2 = 175, Stroke = Brushes.LightGray, StrokeThickness = 1 });
				// in0 (upramp) and in1 (downramp*4) lines
                c.Children.Add(new Line() { X1 = 0, Y1 = 175, X2 = c.Width, Y2 = 25, Stroke = Brushes.LightSalmon, StrokeThickness = 1, Opacity = 0.3 });

                c.Children.Add(new Line() { X1 = 0.0 * c.Width / 4.0, Y1 = 25, X2 = 1.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 1.0 * c.Width / 4.0, Y1 = 25, X2 = 1.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 1.0 * c.Width / 4.0, Y1 = 25, X2 = 2.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 2.0 * c.Width / 4.0, Y1 = 25, X2 = 2.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 2.0 * c.Width / 4.0, Y1 = 25, X2 = 3.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 3.0 * c.Width / 4.0, Y1 = 25, X2 = 3.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });
                c.Children.Add(new Line() { X1 = 3.0 * c.Width / 4.0, Y1 = 25, X2 = 4.0 * c.Width / 4.0, Y2 = 175, Stroke = Brushes.LightBlue, StrokeThickness = 1, Opacity = 0.3 });                
				// plot
				ComboBoxItem cbi = cb.SelectedItem as ComboBoxItem;
				double time = double.Parse(cbi.Content.ToString(), System.Globalization.CultureInfo.InvariantCulture);
                double lastres = (double)outvar.Get();
				for (int i = 0; i < c.Width; i++)
				{
                    MultiParse.Expression.Time = time * (double)i / (double)c.Width;
                    MultiParse.Expression.In0 = -1.0 + 2.0 * (double)i / (double)c.Width;
                    MultiParse.Expression.In1 = 1.0 - 2.0 * ((4*(double)i / (double)c.Width) % 1.0);
                    timevar.Assign(MultiParse.Expression.Time);
                    foreach (string statement in statements)
                    {
                        if (statement == "")
                            continue;
                        exp.ParseExpression = statement;                        
                        res = exp.Evaluate();                        
                    }
                    c.Children.Add(new Line() { X1 = i, Y1 = 100 - lastres * 75.0, X2 = i + 1, Y2 = 100 - (double)outvar.Get() * 75.0, Stroke = Brushes.Lime, StrokeThickness = 1 });
                    lastres = (double)outvar.Get();
				}                   
			}
			catch (Exception ex)
			{
				MessageBox.Show("Formula Plotting Error:" + Environment.NewLine + ex.Message);
			}

            Mouse.OverrideCursor = null;            
		}

        void WTFOSC_Update_Click(object sender, RoutedEventArgs e)
        {
            Node.synthCanvas.NodeResetEventSignal(Node.ID);
        }

#if false
		double GetBQFResponseAt(double f, double b0, double b1, double b2, double a1, double a2, bool mag = true)
		{
			// H(z) = (b0 + b1 / z + b2 / z^2) / (1 + a1 / z + a2 / z^2)
			//
			// Compute H(exp(i * pi * f)).  No native complex numbers in javascript, so break H(exp(i * pi * // f))
			// in to the real and imaginary parts of the numerator and denominator.  Let omega = pi * f.
			// Then the numerator is
			//
			// b0 + b1 * cos(omega) + b2 * cos(2 * omega) - i * (b1 * sin(omega) + b2 * sin(2 * omega))
			//
			// and the denominator is
			//
			// 1 + a1 * cos(omega) + a2 * cos(2 * omega) - i * (a1 * sin(omega) + a2 * sin(2 * omega))
			//
			// Compute the magnitude and phase from the real and imaginary parts.
			var omega = Math.PI * f;
			var numeratorReal = b0 + b1 * Math.Cos(omega) + b2 * Math.Cos(2 * omega);
			var numeratorImag = -(b1 * Math.Sin(omega) + b2 * Math.Sin(2 * omega));
			var denominatorReal = 1 + a1 * Math.Cos(omega) + a2 * Math.Cos(2 * omega);
			var denominatorImag = -(a1 * Math.Sin(omega) + a2 * Math.Sin(2 * omega));
			var magnitude = Math.Sqrt((numeratorReal * numeratorReal + numeratorImag * numeratorImag)
									  / (denominatorReal * denominatorReal + denominatorImag * denominatorImag));
			var phase = Math.Atan2(numeratorImag, numeratorReal) - Math.Atan2(denominatorImag, denominatorReal);

			if (phase >= Math.PI)
			{
				phase -= 2 * Math.PI;
			}
			else if (phase <= -Math.PI)
			{
				phase += 2 * Math.PI;
			}

			if (mag)
				return Math.Log(Math.Abs(magnitude) + 0.000000001, 2.0) * 6.0;
			else
				return phase;
		}

		double GetBQFrequencyAtIndex(int i)
		{
			double nf = 1.0 / Math.Pow(2.0, (1.0 - (double)i / 128.0) * 10.0);
			return nf;
		}
#endif

		void updateBQFResponse()
		{
			return;
#if false
			Path pathl = SpecialGrid.Children[0] as Path;
			Path pathr = SpecialGrid.Children[1] as Path;
			if (pathl == null || pathr == null)
				return;
			PathGeometry pathGeo;
			PathFigure pathFigure;
			double b0, b1, b2, a1, a2, mag;

			// left channel
			pathGeo = pathl.Data as PathGeometry;
			pathFigure = pathGeo.Figures[0] as PathFigure;
			b0 = Node.synthCanvas.GetNodeValue(Node.ID, 0, 0);
			b1 = Node.synthCanvas.GetNodeValue(Node.ID, 1, 0);
			b2 = Node.synthCanvas.GetNodeValue(Node.ID, 2, 0);
			a1 = Node.synthCanvas.GetNodeValue(Node.ID, 4, 0);
			a2 = Node.synthCanvas.GetNodeValue(Node.ID, 5, 0);
			mag = GetBQFResponseAt(GetBQFrequencyAtIndex(0), b0, b1, b2, a1, a2, true);
			pathFigure.StartPoint = new Point(0, 50.0 - mag*150.0 / 72.0);
			for (int i = 1; i <= 128; i++)
			{
				LineSegment lineSeg = pathFigure.Segments[i-1] as LineSegment;
				mag = GetBQFResponseAt(GetBQFrequencyAtIndex(i), b0, b1, b2, a1, a2, true);
				lineSeg.Point = new Point(i * 3, 50.0 - mag * 150.0 / 72.0);
			}

			// right channel
			pathGeo = pathr.Data as PathGeometry;
			pathFigure = pathGeo.Figures[0] as PathFigure;
			b0 = Node.synthCanvas.GetNodeValue(Node.ID, 0, 1);
			b1 = Node.synthCanvas.GetNodeValue(Node.ID, 1, 1);
			b2 = Node.synthCanvas.GetNodeValue(Node.ID, 2, 1);
			a1 = Node.synthCanvas.GetNodeValue(Node.ID, 4, 1);
			a2 = Node.synthCanvas.GetNodeValue(Node.ID, 5, 1);
			mag = GetBQFResponseAt(GetBQFrequencyAtIndex(0), b0, b1, b2, a1, a2, true);
			pathFigure.StartPoint = new Point(0, 50.0 - mag * 150.0 / 72.0);
			for (int i = 1; i <= 128; i++)
			{
				LineSegment lineSeg = pathFigure.Segments[i - 1] as LineSegment;
				mag = GetBQFResponseAt(GetBQFrequencyAtIndex(i), b0, b1, b2, a1, a2, true);
				lineSeg.Point = new Point(i * 3, 50.0 - mag * 150.0 / 72.0);
			}
#endif
		}
		
		// get current mode from gui states
		public int CurrentMode()
		{
			int mode = 0;
			// mode buttons and checkboxes
			foreach (object o in ControlItemMap.Keys)
			{
				ModeButton mb = o as ModeButton;
				// modebutton?
				if ((mb != null) && (mb.IsChecked == true))
				{
					ModeItem mi = ControlItemMap[o];
					int value = mi.Value << mi.Group.Shift;
					mode |= value;
				}
				// checkbox?
				else
				{
					CheckBox cb = o as CheckBox;
					if ((cb != null) && cb.IsChecked == true)
					{
						ModeItem mi = ControlItemMap[o];
						int value = mi.Value;
						mode |= value;
					}
				}
			}
			// mode comboboxes
			foreach (object o in ControlGroupMap.Keys)
			{
				ComboBox cb = o as ComboBox;
				// combobox?
				if ((cb != null))
				{
					ModeGroup mg = ControlGroupMap[cb];
					int value = mg.Items[cb.SelectedIndex].Value << mg.Shift;
					mode |= value;
				}
			}
			return mode;
		}

		// update labels if needed
		void UpdateLabels()
		{
			// special case for glitch, needs renaming and restructuring of edit labels
			if (Node.Info.TypeID == 49)
			{
				Grid grid;
				Knob inputKnob;
				switch (CurrentMode())
				{
					case 0: // tapestop
						{
							grid = NodeEditVerticalStackPanel.Children[2 + 2] as Grid;
							grid.Visibility = System.Windows.Visibility.Collapsed;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(true);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 3] as Grid;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(true);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 4] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(true);
							inputKnob.SetDecoupled(true);
							inputKnob.SetValuesRange(0, 128, 128, 0, 128, 128);
							inputKnob.SetValues();

							grid = NodeEditVerticalStackPanel.Children[2 + 5] as Grid;
							grid.Visibility = System.Windows.Visibility.Collapsed;

							Node.InputName2.Content = "";
							Node.InputName3.Content = "Slowdown";
							Node.InputName4.Content = "Speedup";
							Node.InputName5.Content = "";
							InputName2.Text = "";
							InputName3.Text = "Slowdown";
							InputName4.Text = "Speedup";
							InputName5.Text = "";
							break;
						}
					case 1: // retrigger
						{
							grid = NodeEditVerticalStackPanel.Children[2 + 2] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(true);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 3] as Grid;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(true);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 4] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(false);
							inputKnob.SetDecoupled(true);
							inputKnob.SetValuesRange(-128, 128, 128, -128, 128, 128);
							inputKnob.SetValues();

							grid = NodeEditVerticalStackPanel.Children[2 + 5] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;

							Node.InputName2.Content = "Slice Size";
							Node.InputName3.Content = "Slice Cut";
							Node.InputName4.Content = "dSlice/dPitch";
							Node.InputName5.Content = "Transpose";
							InputName2.Text = "Slice Size (BPM/4)";
							InputName3.Text = "Slice Cut";
							InputName4.Text = "dSlice/dPitch (per Loop)";
							InputName5.Text = "Transpose (Pitch)";
							break;
						}
					case 2: // shuffle
						{
							grid = NodeEditVerticalStackPanel.Children[2 + 2] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(false);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 3] as Grid;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(false);
							inputKnob.SetDecoupled(true);

							grid = NodeEditVerticalStackPanel.Children[2 + 4] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;
							inputKnob = grid.Children[1] as Knob;
							inputKnob.SetSingleInput(false);
							inputKnob.SetDecoupled(true);
							inputKnob.SetValuesRange(0, 128, 128, 0, 128, 128);
							inputKnob.SetValues();

							grid = NodeEditVerticalStackPanel.Children[2 + 5] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;

							Node.InputName2.Content = "Grain Min/Max";
							Node.InputName3.Content = "Buffer Backrange";
							Node.InputName4.Content = "Repeat/Amount";
							Node.InputName5.Content = "Transpose";
							InputName2.Text = "Grain Min/Max (BPM/4)";
							InputName3.Text = "Buffer Backrange";
							InputName4.Text = "Repeat/Amount Probability";
							InputName5.Text = "Transpose (Pitch)";
							break;
						}
					case 3: // reverse
						{
							grid = NodeEditVerticalStackPanel.Children[2 + 2] as Grid;
							grid.Visibility = System.Windows.Visibility.Collapsed;

							grid = NodeEditVerticalStackPanel.Children[2 + 3] as Grid;
							grid.Visibility = System.Windows.Visibility.Collapsed;

							grid = NodeEditVerticalStackPanel.Children[2 + 4] as Grid;
							grid.Visibility = System.Windows.Visibility.Collapsed;

							grid = NodeEditVerticalStackPanel.Children[2 + 5] as Grid;
							grid.Visibility = System.Windows.Visibility.Visible;

							Node.InputName2.Content = "";
							Node.InputName3.Content = "";
							Node.InputName4.Content = "";
							Node.InputName5.Content = "Transpose";
							InputName2.Text = "";
							InputName3.Text = "";
							InputName4.Text = "";
							InputName5.Text = "Transpose (Pitch)";
							break;
						}

				}
			}
            // wtf osc needs decoupled inputs
            if (Node.Info.TypeID == 56)
            {
				Grid grid;
				Knob inputKnob;
                grid = NodeEditVerticalStackPanel.Children[2 + 3] as Grid;
                inputKnob = grid.Children[1] as Knob;
                inputKnob.SetDecoupled(true);
                grid = NodeEditVerticalStackPanel.Children[2 + 4] as Grid;
                inputKnob = grid.Children[1] as Knob;
                inputKnob.SetDecoupled(true);
            }
		}

		// loop all available knobs to update their text display
		private void UpdateKnobsText()
		{
			foreach (UIElement uie in NodeEditVerticalStackPanel.Children)
			{
				Grid grid = uie as Grid;
				if (grid != null)
				{
					if ((grid.Visibility != System.Windows.Visibility.Collapsed) && (grid.Children.Count > 1))
					{
						Knob inputKnob = grid.Children[1] as Knob;
						if (inputKnob != null)
						{
							inputKnob.UpdateText();
						}
					}
				}				
			}
		}

		// mode combobox selection changed
		void cb_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			int modeindex = Node.Info.NumMaxGUISignals;
			// special case for midisignal where signal index is NumMaxGUISignals + 1
			if (Node.Info.TypeID == 29)
				modeindex++;

			// update bq filter response gui
			if (Node.Info.TypeID == 9)
				updateBQFResponse();

			Node.synthCanvas.NodeModeChanged(Node.ID, modeindex, CurrentMode());
			UpdateLabels();
			UpdateKnobsText();
		}
		
		// mode group button changed
		void button_Checked(object sender, RoutedEventArgs e)
		{
			// update bq filter response gui
			if (Node.Info.TypeID == 9)
				updateBQFResponse();

            // hide wavetable selector for input on sampler and wtf osc nodes
            if ((Node.Info.TypeID == 42) || (Node.Info.TypeID == 56))
            {
                int mode = CurrentMode() & 0x00000007;
                if (mode == 0)
                    WavetableComboBox.Visibility = Visibility.Visible;
                else                
                    WavetableComboBox.Visibility = Visibility.Hidden;
            }
								
			Node.synthCanvas.RaiseEditWindow(Node);
			Node.synthCanvas.NodeModeChanged(Node.ID, Node.Info.NumMaxGUISignals, CurrentMode());
			UpdateLabels();
			UpdateKnobsText();
		}

		// mode flag checked
		void cb_Unchecked(object sender, RoutedEventArgs e)
		{
			// update bq filter response gui
			if (Node.Info.TypeID == 9)
				updateBQFResponse();

			Node.synthCanvas.RaiseEditWindow(Node);
			Node.synthCanvas.NodeModeChanged(Node.ID, Node.Info.NumMaxGUISignals, CurrentMode());
			UpdateLabels();
			UpdateKnobsText();
		}

		// mode flag unchecked
		void cb_Checked(object sender, RoutedEventArgs e)
		{
			// update bq filter response gui
			if (Node.Info.TypeID == 9)
				updateBQFResponse();

			Node.synthCanvas.RaiseEditWindow(Node);
			Node.synthCanvas.NodeModeChanged(Node.ID, Node.Info.NumMaxGUISignals, CurrentMode());
			UpdateLabels();
			UpdateKnobsText();
		}

		// value knob changed
		void valueChangedCB(int index, double value1, double value2)
		{
			// update bq filter response gui
			if (Node.Info.TypeID == 9)
				updateBQFResponse();

			Node.synthCanvas.NodeValueChanged(Node.ID, index, value1, value2);
		}

		// triggerseq number of patterns changed (via knob)
		void bitpatternCountChangedCB(int index, double value1, double value2)
		{

			int count = (int)(value1 * 16);			
			for (int i = 0; i < 16; i++)
			{
				RowDefinition rowdef = ModeGrid.RowDefinitions[1+i];
				if (i < count)
					rowdef.Height = GridLength.Auto;
				else
					rowdef.Height = new GridLength(0);
			}			
			
			if (SpecialFlags != null)
				count |= SpecialFlags.SelectedIndex << 8;
			Node.synthCanvas.NodeModeChanged(Node.ID, index, count);
		}

		void bitpatternChangedCB(int index, int patternindex, uint model, uint moder)
		{
			int modemask = (int)((uint)0xff << patternindex*8);
			model = model << patternindex * 8;
			Node.synthCanvas.NodeModeChanged(Node.ID, index, (int)model, modemask);
			moder = moder << patternindex * 8;
			Node.synthCanvas.NodeModeChanged(Node.ID, index + 4, (int)moder, modemask);
		}

		void SpecialFlags_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			// triggerseq
			int count = (int)(SpecialKnob.value1 / SpecialKnob.rangeL * 16);
			count |= SpecialFlags.SelectedIndex << 8;
			Node.synthCanvas.NodeModeChanged(Node.ID, 4, count);
		}

		// samplerec recordtime changed (via knob)
		void recordTimeChangedCB(int index, double value1, double value2)
		{
			int modemask = 0x7ffff000;
			int value = (int)(value1 * 1024.0 * 512.0);
			if (value >= 0x80000)
				value = 0x7ffff;
			value = value << 12;
			Node.synthCanvas.NodeModeChanged(Node.ID, index, value, modemask);
		}

		private void NodeEditVerticalStackPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Node.synthCanvas.ClearSelection();
			Node.synthCanvas.AddSelection(Node);

			Node.synthCanvas.RaiseEditWindow(Node);

			e.Handled = true;
		}

		private void NodeEditClose_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			KillTimer();
		}
	}
}
