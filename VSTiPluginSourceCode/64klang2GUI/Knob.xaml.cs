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
	/// Interaction logic for Knob.xaml
	/// </summary>
	public partial class Knob : UserControl
	{
		GUISynthNodeEdit NodeEdit;
		MouseCapture capture1, capture2;
		public double value1, value2;
		public int index, minValueL, maxValueL, deltaValueL, rangeL, minValueR, maxValueR, deltaValueR, rangeR;
	
		public Knob()
		{
			InitializeComponent();

			NodeEdit = null;
			capture1 = capture2 = null;
			value1 = value2 = 0;
			index = 0;
			minValueL = minValueR = 0;
			maxValueL = maxValueR = 128;
			deltaValueL = deltaValueR = maxValueL - minValueL;
			rangeL = rangeR = 128;
		}

		public void Init(GUISynthNodeEdit nodeedit, int idx, InputInfo info, double v1, double v2, bool decoupled, bool valueEditable = false)
		{
			NodeEdit = nodeedit;
			index = idx;
			
			SetValuesRange(info.MinValueL, info.MaxValueL, info.RangeL, info.MinValueR, info.MaxValueR, info.RangeR);

			value1 = Math.Round(v1 * rangeL * 128.0) / 128.0;
			value2 = Math.Round(v2 * rangeR * 128.0) / 128.0;

			SetValues();
			
			if (info.SingleInput)
			{
				SetSingleInput(info.SingleInput);
			}
			else if (decoupled)
			{
				SetDecoupled(decoupled);
			}

			if (!valueEditable)
			{
				this.Value1Text.IsReadOnly = true;
				this.Value2Text.IsReadOnly = true;
			}
		}

		public void SetSingleInput(bool single)
		{
			if (single)
			{
				this.Knob2.Visibility = System.Windows.Visibility.Collapsed;
				this.Value2Marker.Visibility = System.Windows.Visibility.Collapsed;
				this.Value2Modulator.Visibility = System.Windows.Visibility.Collapsed;
				this.Value2Text.Visibility = System.Windows.Visibility.Collapsed;
				this.ValueSync.Visibility = System.Windows.Visibility.Collapsed;
			}
			else
			{
				this.Knob2.Visibility = System.Windows.Visibility.Visible;
				this.Value2Marker.Visibility = System.Windows.Visibility.Visible;
				this.Value2Modulator.Visibility = System.Windows.Visibility.Visible;
				this.Value2Text.Visibility = System.Windows.Visibility.Visible;
				this.ValueSync.Visibility = System.Windows.Visibility.Visible;
			}
		}

		public void SetDecoupled(bool decoupled)
		{
			if (decoupled)
			{
				this.ValueSync.IsChecked = false;
				this.ValueSync.Visibility = System.Windows.Visibility.Collapsed;
			}
			else
			{
				this.ValueSync.IsChecked = true;
				this.ValueSync.Visibility = System.Windows.Visibility.Visible;
			}
		}


		public void SetValuesRange(int minL, int maxL, int rngL, int minR, int maxR, int rngR, bool updateData = false)
		{
			minValueL = minL;
			maxValueL = maxL;
			deltaValueL = Math.Max(maxValueL - minValueL, 1);
			rangeL = rngL;

			minValueR = minR;
			maxValueR = maxR;
			deltaValueR = Math.Max(maxValueR - minValueR, 1);
			rangeR = rngR;

			if (updateData)
			{
				value1 = Math.Min(Math.Max(value1, minValueL), maxValueL);
				value2 = Math.Min(Math.Max(value2, minValueR), maxValueR);
				NodeEdit.Node.synthCanvas.NodeValueChanged(NodeEdit.Node.ID, index, value1 / rangeL, value2 / rangeR);
			}
		}

		public void SetValues()
		{			
			RotateTransform marker1Rot = this.Value1Marker.RenderTransform as RotateTransform;
			marker1Rot.Angle = -140 + 280 * Math.Min(Math.Max(((value1 - minValueL) / deltaValueL), 0.0), 1.0);
			RotateTransform mod1Rot = this.Value1Modulator.RenderTransform as RotateTransform;
			mod1Rot.Angle = marker1Rot.Angle;

			RotateTransform marker2Rot = this.Value2Marker.RenderTransform as RotateTransform;
			marker2Rot.Angle = -140 + 280 * Math.Min(Math.Max(((value2 - minValueR) / deltaValueR), 0.0), 1.0);
			RotateTransform mod2Rot = this.Value2Modulator.RenderTransform as RotateTransform;
			mod2Rot.Angle = marker2Rot.Angle;

			if (marker1Rot.Angle != marker2Rot.Angle)
			{
				this.ValueSync.IsChecked = false;
			}
		}

		public void UpdateText()
		{
			SyncingText = true;
			Value1Text.Text = FormatText(value1, rangeL);
			Value2Text.Text = FormatText(value2, rangeR);
			SyncingText = false;
		}

		private void Ellipse_PreviewMouseLeftButtonDown1(object sender, MouseButtonEventArgs e)
		{
			NodeEdit.Node.synthCanvas.RaiseEditWindow(NodeEdit.Node);

			var target = sender as Ellipse;
			if (target == null) return;

			var point = e.GetPosition(target);
			Point centerPoint = new Point();
			centerPoint.X = target.ActualWidth / 2;
			centerPoint.Y = target.ActualHeight / 2;

			capture1 = new MouseCapture
			{
				StartValue = value1,
				StartPoint = point,
				CenterPoint = centerPoint,
				HighPrecision = (Keyboard.Modifiers & ModifierKeys.Control) > 0
			};

			target.CaptureMouse();
			target.Cursor = Cursors.ScrollNS;
			
			e.Handled = true;
		}

		private void Ellipse_PreviewMouseLeftButtonUp1(object sender, MouseButtonEventArgs e)
		{
			var target = sender as Ellipse;
			if (target == null || capture1 == null) return;

			target.ReleaseMouseCapture();
			target.Cursor = Cursors.Arrow;

			capture1 = null;

			e.Handled = true;
		}

		private void Ellipse_PreviewMouseMove1(object sender, MouseEventArgs e)
		{
			var target = sender as Ellipse;
			if (target == null || capture1 == null) return;

			var point = e.GetPosition(target);

			var dx = -point.Y + capture1.StartPoint.Y;
			//var dx = point.X - capture1.StartPoint.X;

			double zoom = 1.0;
			// try to get canvas
			FrameworkElement fe = (FrameworkElement)sender;
			while ((fe != null) && (fe.GetType() != typeof(SynthCanvas)))
			{
				fe = (FrameworkElement)(fe.Parent);
			}
			if (fe != null)
				zoom = ((SynthCanvas)fe).Zoom();

			// high precision
			if (capture1.HighPrecision == true)
				value1 = Math.Min(Math.Max(capture1.StartValue + (int)(dx*zoom*0.5) / 128.0, minValueL), maxValueL);
			// normal precision
			else
				value1 = Math.Min(Math.Max(capture1.StartValue + (int)(dx*zoom*0.5), minValueL), maxValueL);
			RotateTransform marker1Rot = this.Value1Marker.RenderTransform as RotateTransform;
			marker1Rot.Angle = -140 + 280 * ((value1 - minValueL) / deltaValueL);
			RotateTransform mod1Rot = this.Value1Modulator.RenderTransform as RotateTransform;
			mod1Rot.Angle = marker1Rot.Angle;
			// TODO: map signal for output
			SyncingText = true;
			Value1Text.Text = FormatText(value1, rangeL);

			if (this.ValueSync.IsChecked == true)
			{
				value2 = value1;
				RotateTransform marker2Rot = this.Value2Marker.RenderTransform as RotateTransform;
				marker2Rot.Angle = marker1Rot.Angle;
				RotateTransform mod2Rot = this.Value2Modulator.RenderTransform as RotateTransform;
				mod2Rot.Angle = marker2Rot.Angle;
				Value2Text.Text = Value1Text.Text;
			}
			SyncingText = false;

			if (valueChangedHandler != null)
				valueChangedHandler(index, value1/rangeL, value2/rangeL);

			e.Handled = true;
		}

		private void Ellipse_PreviewMouseLeftButtonDown2(object sender, MouseButtonEventArgs e)
		{
			NodeEdit.Node.synthCanvas.RaiseEditWindow(NodeEdit.Node);

			var target = sender as Ellipse;
			if (target == null) return;

			var point = e.GetPosition(target);
			Point centerPoint = new Point();
			centerPoint.X = target.ActualWidth / 2;
			centerPoint.Y = target.ActualHeight / 2;

			capture2 = new MouseCapture
			{
				StartValue = value2,
				StartPoint = point,
				CenterPoint = centerPoint,
				HighPrecision = (Keyboard.Modifiers & ModifierKeys.Control) > 0
			};

			target.CaptureMouse();
			target.Cursor = Cursors.ScrollNS;

			e.Handled = true;
		}

		private void Ellipse_PreviewMouseLeftButtonUp2(object sender, MouseButtonEventArgs e)
		{
			var target = sender as Ellipse;
			if (target == null || capture2 == null) return;

			target.ReleaseMouseCapture();
			target.Cursor = Cursors.Arrow;

			capture2 = null;

			e.Handled = true;
		}

		private void Ellipse_PreviewMouseMove2(object sender, MouseEventArgs e)
		{
			var target = sender as Ellipse;
			if (target == null || capture2 == null) return;

			var point = e.GetPosition(target);

			var dx = -point.Y + capture2.StartPoint.Y;
			//var dx = point.X - capture2.StartPoint.X;

			double zoom = 1.0;
			// try to get canvas
			FrameworkElement fe = (FrameworkElement)sender;
			while ((fe != null) && (fe.GetType() != typeof(SynthCanvas)))
			{
				fe = (FrameworkElement)(fe.Parent);
			}
			if (fe != null)
				zoom = ((SynthCanvas)fe).Zoom();

			// high precision
			if (capture2.HighPrecision == true)
				value2 = Math.Min(Math.Max(capture2.StartValue + (int)(dx*zoom*0.5) / 128.0, minValueR), maxValueR);
			// normal precision
			else
				value2 = Math.Min(Math.Max(capture2.StartValue + (int)(dx*zoom*0.5), minValueR), maxValueR);
			RotateTransform marker2Rot = this.Value2Marker.RenderTransform as RotateTransform;
			marker2Rot.Angle = -140 + 280 * ((value2 - minValueR) / deltaValueR);
			RotateTransform mod2Rot = this.Value2Modulator.RenderTransform as RotateTransform;
			mod2Rot.Angle = marker2Rot.Angle;
			// TODO: map signal for output
			SyncingText = true;
			Value2Text.Text = FormatText(value2, rangeR);

			if (this.ValueSync.IsChecked == true)
			{
				value1 = value2;
				RotateTransform marker1Rot = this.Value1Marker.RenderTransform as RotateTransform;
				marker1Rot.Angle = marker2Rot.Angle;
				RotateTransform mod1Rot = this.Value1Modulator.RenderTransform as RotateTransform;
				mod1Rot.Angle = marker1Rot.Angle;
				Value1Text.Text = Value2Text.Text;
			}
			SyncingText = false;

			if (valueChangedHandler != null)
				valueChangedHandler(index, value1/rangeR, value2/rangeR);

			e.Handled = true;
		}

		private void ValueSync_Click(object sender, RoutedEventArgs e)
		{
			NodeEdit.Node.synthCanvas.RaiseEditWindow(NodeEdit.Node);
		}

		public void UpdateModMarkers(double v1, double v2)
		{
			v1 = Math.Min(Math.Max(Math.Round(v1 * rangeL * 128.0) / 128.0, minValueL), maxValueL);
			v2 = Math.Min(Math.Max(Math.Round(v2 * rangeR * 128.0) / 128.0, minValueR), maxValueR);

			RotateTransform mod1Rot = this.Value1Modulator.RenderTransform as RotateTransform;
			mod1Rot.Angle = -140 + 280 * ((v1 - minValueL) / deltaValueL);

			RotateTransform mod2Rot = this.Value2Modulator.RenderTransform as RotateTransform;
			mod2Rot.Angle = -140 + 280 * ((v2 - minValueR) / deltaValueR);
		}

		private bool SyncingText;
		private void Value1Text_TextChanged(object sender, TextChangedEventArgs e)
		{
			if (!this.IsInitialized)
				return;

			try
			{
				if (SyncingText)
					return;

				double v = Convert.ToDouble(this.Value1Text.Text, System.Globalization.CultureInfo.InvariantCulture);
				value1 = v * 128.0;

				if (this.ValueSync.IsChecked == true)
				{
					SyncingText = true;
					value2 = value1;
					this.Value2Text.Text = this.Value1Text.Text;
					SyncingText = false;
				}

				if (valueChangedHandler != null)
					valueChangedHandler(index, value1 / rangeL, value2 / rangeR);
			}
			catch 
			{
			}
		}

		private void Value2Text_TextChanged(object sender, TextChangedEventArgs e)
		{
			if (!this.IsInitialized)
				return;

			try
			{
				if (SyncingText)
					return;

				double v = Convert.ToDouble(this.Value2Text.Text, System.Globalization.CultureInfo.InvariantCulture);
				value2 = v * 128.0;

				if (this.ValueSync.IsChecked == true)
				{
					SyncingText = true;
					value1 = value2;
					this.Value1Text.Text = this.Value2Text.Text;
					SyncingText = false;
				}

				if (valueChangedHandler != null)
					valueChangedHandler(index, value1 / rangeL, value2 / rangeR);
			}
			catch
			{
			}
		}		

		public delegate void ValueChangedHandler(int index, double value1, double value2);
		public ValueChangedHandler valueChangedHandler;

		string DefaultFormat(double value)
		{
			string ret = "";
			double i = Math.Truncate(value);
			double f = Math.Abs(100.0 * Math.Round(value - i, 2));
			if (f != 0)
			{
				ret = i.ToString() + ".\n" + f.ToString();
			}
			else
			{
				ret = i.ToString() + ".\n";
			}
			return ret;
		}

		string FormatText(double value, double range)
		{
			double norm = value / range;
			string ret = "";
			// constant or voice inputs?
			if (NodeEdit.Node.Info.TypeID >= 64)
			{
				ret = norm.ToString(System.Globalization.CultureInfo.InvariantCulture);
				return ret;
			}
			// special case for adsr gain?
			if ((NodeEdit.Node.Info.TypeID == 5) && (NodeInfo.Info(NodeEdit.Node.Info.TypeID).InputInfos[index].Name == "Gain"))
			{
				// is the dbgain checkbox checked?				
				if ((NodeEdit.CurrentMode() & 128) != 0)
				{
					double db = (norm - 0.75) * 128.0;
					double tailscale = 100.0;
					db = Math.Truncate(db * tailscale) / tailscale;
					ret = db.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\ndB";
					return ret;
				}
			}
			switch (NodeInfo.Info(NodeEdit.Node.Info.TypeID).InputInfos[index].Mapping)
			{
				case InputMapping.Default:
				{
					ret = DefaultFormat(value);
					break;
				}
				case InputMapping.ADSR_Rate:
				{
					double samples = norm * norm * norm * 418381.824 + 1.0;
					double ms = samples / 44.1;
					double tailscale = 10000.0;
					if (ms >= 10.0)
						tailscale = 1000.0;
					if (ms >= 100.0)
						tailscale = 100.0;
					if (ms >= 1000.0)
						tailscale = 10.0;
					ms = Math.Truncate(ms * tailscale) / tailscale;
					ret = ms.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nms";
					break;
				}
				case InputMapping.LFO_Frequency:
				{
					// bpm sync?
					if ((NodeEdit.CurrentMode() & 2048) != 0)
					{
						double scale = Math.Pow(2.0, (norm-0.5)*128.0/12.0);						
						if (scale >= 1.0)
						{
							double tailscale = 100.0;
							scale = Math.Truncate(scale * tailscale) / tailscale;
							ret = "BPM *\n" + scale.ToString(System.Globalization.CultureInfo.InvariantCulture);
						}
						else
						{
							scale = 1.0 / scale;
							double tailscale = 100.0;
							scale = Math.Truncate(scale * tailscale) / tailscale;
							ret = "BPM /\n" + scale.ToString(System.Globalization.CultureInfo.InvariantCulture);
						}
					}
					// free frequency
					else
					{
						double freq = 44100.0 * norm * norm * norm * norm * norm * 0.001;
						double tailscale = 100000.0;
						if (freq >= 0.1)
							tailscale = 10000.0;
						if (freq >= 1.0)
							tailscale = 1000.0;
						if (freq >= 10.0)
							tailscale = 100.0;
						freq = Math.Truncate(freq * tailscale) / tailscale;
						ret = freq.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nhz";
					}
					break;
				}
				case InputMapping.OSC_Frequency:
				{
					// free frequency
					if ((NodeEdit.CurrentMode() & 2048) == 0)
					{
						double freq = 44100.0 * 0.5 * norm;
						double tailscale = 1000.0;
						if (freq >= 100.0)
							tailscale = 100.0;
						if (freq >= 1000.0)
							tailscale = 100.0;
						if (freq >= 10000.0)
							tailscale = 10.0;
						freq = Math.Truncate(freq * tailscale) / tailscale;
						ret = freq.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nhz";
					}
					else
					{
						ret = DefaultFormat(value);
					}
					break;
				}
				case InputMapping.BQF_Frequency:
				{
					double scale = 1.0;
					if (NodeEdit.Node.Info.TypeID == 43) // svfilter has different frequency range // actually the scale is 1/(pi/2)
						scale = 0.63;
					// free frequency
					if ((NodeEdit.CurrentMode() & 128) == 0)
					{
						double freq;
						if ((NodeEdit.CurrentMode() & 64) != 0)
							freq = 44100.0 * Math.Min(norm*norm, 0.99765) * 0.5 * scale;
						else
							freq = 44100.0 * Math.Min(1.0 / Math.Pow(2.0, (1.0 - norm) * 10.0), 0.99765) * 0.5 * scale;

						double tailscale = 1000.0;
						if (freq >= 100.0)
							tailscale = 100.0;
						if (freq >= 1000.0)
							tailscale = 100.0;
						if (freq >= 10000.0)
							tailscale = 10.0;
						freq = Math.Truncate(freq * tailscale) / tailscale;
						ret = freq.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nhz";
					}
					else
					{
						ret = DefaultFormat(value);
					}
					break;
				}
				case InputMapping.Attack:
				{
					double ms = Math.Max(norm * 128.0, 0.00128);
					double tailscale = 10000.0;
					if (ms >= 10.0)
						tailscale = 1000.0;
					if (ms >= 100.0)
						tailscale = 100.0;
					if (ms >= 1000.0)
						tailscale = 10.0;
					ms = Math.Truncate(ms * tailscale) / tailscale;
					ret = ms.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nms";
					break;
				}
				case InputMapping.Release:
				{
					double ms = Math.Max(norm * 128.0 * 10, 0.00128);
					double tailscale = 10000.0;
					if (ms >= 10.0)
						tailscale = 1000.0;
					if (ms >= 100.0)
						tailscale = 100.0;
					if (ms >= 1000.0)
						tailscale = 10.0;
					ms = Math.Truncate(ms * tailscale) / tailscale;
					ret = ms.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nms";
					break;
				}
				case InputMapping.Delay_Time:
				{
					double ms = -1.0;
					int delaymode;
					// voice manager and glitch needs no mode, just selectino of bpm fraction
					if (NodeEdit.Node.Info.TypeID == 3 ||
						NodeEdit.Node.Info.TypeID == 49)
						delaymode = 0;
					else
						delaymode = NodeEdit.CurrentMode() & 0xf;
					switch (delaymode)
					{
						// bpm sync
						case 0:
						{
							string[] DELAY_TIME_NAME = new string[]
							{	
								"1/128",
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
							int i = (int)(norm * 128.0) / 4;
							ret = DELAY_TIME_NAME[i];
							break;
						}
						// short
						case 1:
						{
							ms = norm * 1640.0 / 44.1;
							break;
						}
						// middle
						case 2:
						{
							ms = norm * 5644.8 / 44.1;
							break;
						}
						// long
						case 3:
						{
							ms = norm * 44100.0 * 2.0 / 44.1;
							break;
						}
						// notemap
						case 4:
						{
							ret = DefaultFormat(-value + range/2);
							break;
						}
						// notemap2
						case 5:
						{
							ret = DefaultFormat(value);
							break;
						}
					}
					if (ms >= 0.0)
					{
						double tailscale = 10000.0;
						if (ms >= 10.0)
							tailscale = 1000.0;
						if (ms >= 100.0)
							tailscale = 100.0;
						if (ms >= 1000.0)
							tailscale = 10.0;
						ms = Math.Truncate(ms * tailscale) / tailscale;
						ret = ms.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nms";
					}
					break;
				}
				case InputMapping.Decibel:
				{
					double db = (norm-0.75) * 128.0;
					double tailscale = 100.0;
					db = Math.Truncate(db * tailscale) / tailscale;
					ret = db.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\ndB";
					break;
				}
				case InputMapping.Ratio:
				{
					double ratio = Math.Pow(2.0, norm * 6.0);
					if (ratio <= 1.0)
					{
						ratio = 1.0 / ratio;
						double tailscale = 100.0;
						ratio = Math.Truncate(ratio * tailscale) / tailscale;
						ret = ratio.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\n: 1";
					}
					else
					{
						double tailscale = 100.0;
						ratio = Math.Truncate(ratio * tailscale) / tailscale;
						ret = "1 :\n" + ratio.ToString(System.Globalization.CultureInfo.InvariantCulture);
					}
					break;
				}
				case InputMapping.Speed:
				{
					double speed = Math.Pow(2.0, norm * 128.0 / 12.0);
					double tailscale = 100.0;
					speed = Math.Truncate(speed * tailscale) / tailscale;
					ret = speed.ToString(System.Globalization.CultureInfo.InvariantCulture);
					break;
				}
				case InputMapping.RecordTime:
				{
					double samples = norm * 1024.0 * 512.0;
					double ms = samples / 44.1;
					double tailscale = 10.0;
					ms = Math.Truncate(ms * tailscale) / tailscale;
					ret = ms.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nms";
					break;
				}
				case InputMapping.GlideTime:
				{
					string[] GLIDE_TIME_NAME = new string[]
					{	
						"OFF",
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
						"Instant",
					};
					int i = (int)(norm * 128.0) / 4;
					ret = GLIDE_TIME_NAME[i];
					break;			
				}
				case InputMapping.Normalized:
				{
					ret = norm.ToString(System.Globalization.CultureInfo.InvariantCulture);
					break;
				}
                case InputMapping.SNH_Frequency:
                {
                    // trigger
                    if ((NodeEdit.CurrentMode() & 16) != 0)
                    {
                        if (norm < 0.5)
                            ret = "0";
                        else
                            ret = "1";
                    }
                    // free frequency
                    else if ((NodeEdit.CurrentMode() & 128) == 0)
                    {
                        double freq;
                        if ((NodeEdit.CurrentMode() & 64) != 0)
                            freq = 44100.0 * Math.Min(norm * norm, 0.99765) * 0.5;
                        else
                            freq = 44100.0 * Math.Min(1.0 / Math.Pow(2.0, (1.0 - norm) * 10.0), 0.99765) * 0.5;

                        double tailscale = 1000.0;
                        if (freq >= 100.0)
                            tailscale = 100.0;
                        if (freq >= 1000.0)
                            tailscale = 100.0;
                        if (freq >= 10000.0)
                            tailscale = 10.0;
                        freq = Math.Truncate(freq * tailscale) / tailscale;
                        ret = freq.ToString(System.Globalization.CultureInfo.InvariantCulture) + "\nhz";
                    }
                    // transposed note frequency
                    else
                    {
                        double db = (-0.25 + norm) * 128.0;
                        double tailscale = 100.0;
                        db = Math.Truncate(db * tailscale) / tailscale;
                        ret = db.ToString(System.Globalization.CultureInfo.InvariantCulture);                       
                    }
                    break;
                }
			}
			return ret;
		}
		
		internal class MouseCapture
		{
			public double StartValue { get; set; }
			public Point StartPoint { get; set; }
			public Point CenterPoint { get; set; }
			public bool HighPrecision { get; set; }
		}

		
	}
}
