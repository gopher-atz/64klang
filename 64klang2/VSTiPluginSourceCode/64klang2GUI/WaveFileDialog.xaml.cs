using System;
using System.IO;
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
using Microsoft.Win32;

namespace _64klang2GUI
{
	/// <summary>
	/// Interaction logic for WaveFileDialog.xaml
	/// </summary>
	public partial class WaveFileDialog : UserControl
	{
		private SynthWindow MainWindow;

		public WaveFileDialog(SynthWindow window)
		{
			InitializeComponent();

			MainWindow = window;

			for (int i = 0; i < this.WaveFileStackPanel.Children.Count; i++)
			{
				WaveFileConfig wfc = this.WaveFileStackPanel.Children[i] as WaveFileConfig;
				wfc.Index.Content = (i+1).ToString();

				int freq = MainWindow.GetWaveFileFrequency(i);
				wfc.SampleRate.SelectedIndex = freq;

				wfc.FileName.Content = "-";
				string filename = MainWindow.GetWaveFileName(i);
				if (filename != "")
				{
					try
					{
						FileInfo fi = new FileInfo(filename);
						wfc.FileName.Content = fi.Name;
					}
					catch
					{
					}
				}

				int compressed = MainWindow.GetWaveFileCompressedSize(i);
				wfc.Info.Content = compressed.ToString() + " Bytes";

				wfc.Load.Click += new RoutedEventHandler(Load_Click);
				wfc.Clear.Click += new RoutedEventHandler(Clear_Click);
				wfc.SampleRate.SelectionChanged += new SelectionChangedEventHandler(SampleRate_SelectionChanged);
			}
		}

		void SampleRate_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			// find the WaveFileConfig which sent the event
			FrameworkElement fe = sender as FrameworkElement;
			fe = fe.Parent as FrameworkElement;
			WaveFileConfig wfc = fe.Parent as WaveFileConfig;
			int index = 0;
			for (index = 0; index < this.WaveFileStackPanel.Children.Count; index++)
			{
				if (wfc == this.WaveFileStackPanel.Children[index])
					break;
			}

			string filename = MainWindow.GetWaveFileName(index, true);
			if (filename != "")
			{
				int compressed = MainWindow.SetWaveFile(index, wfc.SampleRate.SelectedIndex, filename);
				wfc.Info.Content = compressed.ToString() + " Bytes";
				if (compressed > 0)
				{
					FileInfo fi = new FileInfo(filename);
					wfc.FileName.Content = fi.Name;
				}
				else
				{
					wfc.FileName.Content = "-";
				}
			}
			else
			{
				wfc.FileName.Content = "-";
			}
		}

		void Load_Click(object sender, RoutedEventArgs e)
		{
			// find the WaveFileConfig which sent the event
			FrameworkElement fe = sender as FrameworkElement;
			fe = fe.Parent as FrameworkElement;
			WaveFileConfig wfc = fe.Parent as WaveFileConfig;
			int index = 0;
			for (index = 0; index < this.WaveFileStackPanel.Children.Count; index++)
			{
				if (wfc == this.WaveFileStackPanel.Children[index])
					break;
			}

			OpenFileDialog ofd = new OpenFileDialog();
			ofd.InitialDirectory = MainWindow.CurrentDirectory;
			ofd.DefaultExt = ".wav";
			ofd.Filter = "Wave File|*.wav";
			Nullable<bool> result = ofd.ShowDialog();
			if (result == true)
			{				
				int compressed = MainWindow.SetWaveFile(index, wfc.SampleRate.SelectedIndex, ofd.FileName);
				wfc.Info.Content = compressed.ToString() + " Bytes";

				string tname;
				if (compressed > 0)
				{
					FileInfo fi = new FileInfo(ofd.FileName);
					tname = fi.Name;
				}
				else
				{
					tname = "-";
				}	
				wfc.FileName.Content = tname;
				
				// update all nodeedit windows for sample players or wtf oscs
				foreach (GUISynthNode gs in GUISynthNode.IDNodeMap.Values)
				{
                    if (gs.Info.TypeID == 42 || gs.Info.TypeID == 56)
					{
						if ((gs.IsEditOpen) != 0 && (gs.EditWindow.WavetableComboBox != null))
						{
							ComboBoxItem cbi = gs.EditWindow.WavetableComboBox.Items[index] as ComboBoxItem;
							cbi.Content = tname;
						}
						gs.UpdateModeText();
					}
				}
			}
		}

		void Clear_Click(object sender, RoutedEventArgs e)
		{
			// find the WaveFileConfig which sent the event
			FrameworkElement fe = sender as FrameworkElement;
			fe = fe.Parent as FrameworkElement;
			WaveFileConfig wfc = fe.Parent as WaveFileConfig;
			int index = 0;
			for (index = 0; index < this.WaveFileStackPanel.Children.Count; index++)
			{
				if (wfc == this.WaveFileStackPanel.Children[index])
					break;
			}

			wfc.FileName.Content = "-";
			wfc.Info.Content = "0 Bytes";

			MainWindow.SetWaveFile(index, 0, "");
		}
	}
}
