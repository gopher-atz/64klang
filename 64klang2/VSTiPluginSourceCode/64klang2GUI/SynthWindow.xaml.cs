using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Windows.Input;
using System.Reflection;
using Microsoft.Win32;

namespace _64klang2GUI
{
	/// <summary>
	/// Interaction logic for Clock.xaml
	/// </summary>
	public partial class SynthWindow : Window
	{
		private WaveFileDialog WFD;
		public string CurrentDirectory { get; set; }
		public bool AllowClose { get; private set; }

		public SynthWindow()
		{
			InitializeComponent();

			MainCanvas.MainWindow = this;

			WFD = null;

			ScrollCapture = null;

            CompositionTarget.Rendering += CompositionTarget_Rendering;

			CurrentDirectory = System.IO.Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + @"\64klang2";

			this.Title = "64klang by Gopher/Alcatraz (" + System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString() + ")";

			Show();
			AllowClose = false;
		}

        public void PrepareClosing()
		{
            CompositionTarget.Rendering -= CompositionTarget_Rendering;
			AllowClose = true;
		}

		public SynthCanvas SynthCanvas()
		{
			return MainCanvas;
		}

		private void NavigationScrollViewer_MouseEnter(object sender, MouseEventArgs e)
		{
			//this.Focus();
		}

		public void ScrollTo(double x, double y)
		{
			NavigationScrollViewer.ScrollToHorizontalOffset(x);
			NavigationScrollViewer.ScrollToVerticalOffset(y);
		}

		public delegate void PanicHandler();
		public PanicHandler panicHandler;
		private void Panic_Click(object sender, RoutedEventArgs e)
		{
			if (panicHandler != null)
				panicHandler();
		}

		public delegate void KillVoicesHandler();
		public KillVoicesHandler killVoicesHandler;
		public void KillVoices()
		{
			if (killVoicesHandler != null)
				killVoicesHandler();
		}

		public delegate void QueryCoreProcessingMutexHandler(bool acquire);
		public QueryCoreProcessingMutexHandler queryCoreProcessingMutexHandler;
		public void QueryCoreProcessingMutex(bool acquire)
		{
			if (queryCoreProcessingMutexHandler != null)
				queryCoreProcessingMutexHandler(acquire);
		}

		public void SetVoiceCountInfo(int total)
		{
			this.VoiceCount.Content = "Total Voices: " + total.ToString();
		}		

#region Timer

		public delegate void VoiceUpdateHandler();
		public VoiceUpdateHandler voiceUpdateHandler;

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
			if (voiceUpdateHandler != null)
				voiceUpdateHandler();
		}

#endregion

#region TouchScroll
		
		internal class MouseCapture
		{
			public Double HorizontalOffset { get; set; }
			public Double VerticalOffset { get; set; }
			public Point Point { get; set; }
		}

		MouseCapture ScrollCapture;

		private void NavigationScrollViewer_PreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
		{
			var point = e.GetPosition(NavigationScrollViewer);

			double sbwidth = NavigationScrollViewer.VerticalScrollBarVisibility == ScrollBarVisibility.Visible ? SystemParameters.VerticalScrollBarWidth : 0;
			double sbheight = NavigationScrollViewer.HorizontalScrollBarVisibility == ScrollBarVisibility.Visible ? SystemParameters.HorizontalScrollBarHeight : 0;
			if ((point.X >= NavigationScrollViewer.ActualWidth - sbwidth) || point.Y >= NavigationScrollViewer.ActualHeight - sbheight)
				return;

			ScrollCapture = new MouseCapture
			{
				HorizontalOffset = NavigationScrollViewer.HorizontalOffset,
				VerticalOffset = NavigationScrollViewer.VerticalOffset,
				Point = e.GetPosition(NavigationScrollViewer),
			};
			NavigationScrollViewer.Cursor = Cursors.SizeAll;
		}

		private void NavigationScrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
		{
			MainCanvas.SetZoom(e, false);
			//e.Handled = true;
		}

		private void NavigationScrollViewer_PreviewMouseRightButtonUp(object sender, MouseButtonEventArgs e)
		{
			if (ScrollCapture == null)
				return;

			ScrollCapture = null;
			NavigationScrollViewer.ReleaseMouseCapture();
			NavigationScrollViewer.Cursor = Cursors.Arrow;
		}

		private void NavigationScrollViewer_PreviewMouseMove(object sender, MouseEventArgs e)
		{
			if (ScrollCapture == null)
				return;

			var point = e.GetPosition(NavigationScrollViewer);

			var dy = point.Y - ScrollCapture.Point.Y;
			var dx = point.X - ScrollCapture.Point.X;
			if (Math.Abs(dy) > 5 || Math.Abs(dx) > 5)
			{
				NavigationScrollViewer.CaptureMouse();
			}

			ScrollTo(ScrollCapture.HorizontalOffset - dx, ScrollCapture.VerticalOffset - dy);
		}

#endregion

#region FileOperations

		public static string LastPatchName = null;
		public static string LastChannelName = null;
		
        public void PreLoadPatchActions()
        {
            Mouse.OverrideCursor = Cursors.Wait;

			MainCanvas.KillNodeTimers();
			if (WFD != null)
			{
				this.EditGrid.Children.Remove(WFD);
				WFD = null;
				this.EditWavetables.IsEnabled = true;
			}
        }
		public void PostLoadPatchActions()
        {
            // update quickjump item names
			for (int i = 0; i < 16; i++)
			{
				ComboBoxItem item = JumpToChannel.Items[i] as ComboBoxItem;
				item.Content = MainCanvas.GetChannelName(i, true);
			}

            Mouse.OverrideCursor = null;
        }

		public delegate void LoadPatchHandler(string filename);
		public LoadPatchHandler loadPatchHandler;
		private void LoadPatch_Click(object sender, RoutedEventArgs e)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			if (LastPatchName != null)
			{
				FileInfo fi = new FileInfo(LastPatchName);
				ofd.InitialDirectory = fi.DirectoryName;
			}
			else
			{
				ofd.InitialDirectory = CurrentDirectory;
			}
			ofd.DefaultExt = ".64k2Patch";
			ofd.Filter = "64klang2 Patch|*.64k2Patch";
			Nullable<bool> result = ofd.ShowDialog();
			if (result == true)
			{
                PreLoadPatchActions();

				FileInfo fi = new FileInfo(ofd.FileName);
				CurrentDirectory = fi.DirectoryName;

				LastPatchName = ofd.FileName;
				if (loadPatchHandler != null)
					loadPatchHandler(ofd.FileName);

                PostLoadPatchActions();
			}
		}

		public delegate void SavePatchHandler(string filename);
		public SavePatchHandler savePatchHandler;
		private void SavePatch_Click(object sender, RoutedEventArgs e)
		{
			SaveFileDialog sfd = new SaveFileDialog();
			if (LastPatchName != null)
				sfd.FileName = LastPatchName;
			sfd.DefaultExt = ".64k2Patch";
			sfd.Filter = "64klang2 Patch|*.64k2Patch";
			Nullable<bool> result = sfd.ShowDialog();
			if (result == true)
			{
				LastPatchName = sfd.FileName;

				FileInfo fi = new FileInfo(sfd.FileName);
				CurrentDirectory = fi.DirectoryName;

				if (savePatchHandler != null)
					savePatchHandler(sfd.FileName);
			}
		}

		public delegate void ResetPatchHandler();
		public ResetPatchHandler resetPatchHandler;
		private void ResetPatch_Click(object sender, RoutedEventArgs e)
		{
			if (MessageBox.Show("Reset the whole Patch?", "64klang2", MessageBoxButton.YesNo) == MessageBoxResult.Yes)
			{
				Mouse.OverrideCursor = Cursors.Wait;

				MainCanvas.KillNodeTimers();
				if (WFD != null)
				{
					this.EditGrid.Children.Remove(WFD);
					WFD = null;
					this.EditWavetables.IsEnabled = true;
				}

				if (resetPatchHandler != null)
					resetPatchHandler();

				// update quickjump item names
				for (int i = 0; i < 16; i++)
				{
					ComboBoxItem item = JumpToChannel.Items[i] as ComboBoxItem;
					item.Content = MainCanvas.GetChannelName(i, true);
				}

				Mouse.OverrideCursor = null;
			}
		}

		public delegate void LoadChannelHandler(int channel, string filename);
		public LoadChannelHandler loadChannelHandler;
		public string LoadChannel(int channel)
		{
			OpenFileDialog ofd = new OpenFileDialog();			
			if (LastChannelName != null)
			{
				FileInfo fi = new FileInfo(LastChannelName);
				ofd.InitialDirectory = fi.DirectoryName;
			}
			else if (LastPatchName != null)
			{
				FileInfo fi = new FileInfo(LastPatchName);
				ofd.InitialDirectory = fi.DirectoryName;
			}
			else
			{
				ofd.InitialDirectory = CurrentDirectory;
			}

			ofd.DefaultExt = ".64k2Channel";
			ofd.Filter = "64klang2 Channel|*.64k2Channel";
			Nullable<bool> result = ofd.ShowDialog();
			if (result == true)
			{
				Mouse.OverrideCursor = Cursors.Wait;

				MainCanvas.KillNodeTimers();

				FileInfo fi = new FileInfo(ofd.FileName);
				CurrentDirectory = fi.DirectoryName;

				LastChannelName = ofd.FileName;
				if (loadChannelHandler != null)
					loadChannelHandler(channel, ofd.FileName);

				// update quickjump item names
				ComboBoxItem item = JumpToChannel.Items[channel] as ComboBoxItem;
				item.Content = MainCanvas.GetChannelName(channel, true);

				Mouse.OverrideCursor = null;

				MainCanvas.SelectChannel(channel, false);

				return fi.Name.Substring(0, fi.Name.Length - fi.Extension.Length);
			}
			return "";
		}

		public delegate void SaveChannelHandler(int channel, string filename);
		public SaveChannelHandler saveChannelHandler;
		public string SaveChannel(int channel)
		{
			string cname = MainCanvas.GetChannelName(channel, false);

			SaveFileDialog sfd = new SaveFileDialog();
			if ((LastChannelName != null) || (cname != ""))
			{
				if (LastChannelName != null)
				{
					FileInfo fi = new FileInfo(LastChannelName);
					if (cname != "")
						sfd.FileName = fi.DirectoryName + @"\" + cname;
					else
						sfd.InitialDirectory = fi.DirectoryName;
				}
				else
				{
					sfd.FileName = cname;
				}
			}
						
			sfd.DefaultExt = ".64k2Channel";
			sfd.Filter = "64klang2 Channel|*.64k2Channel";
			Nullable<bool> result = sfd.ShowDialog();
			if (result == true)
			{
				LastChannelName = sfd.FileName;

				FileInfo fi = new FileInfo(sfd.FileName);
				CurrentDirectory = fi.DirectoryName;

				if (saveChannelHandler != null)
					saveChannelHandler(channel, sfd.FileName);

				MainCanvas.UpdateChannelName(channel);
				// update quickjump item names
				ComboBoxItem item = JumpToChannel.Items[channel] as ComboBoxItem;
				item.Content = MainCanvas.GetChannelName(channel, true);

				return fi.Name.Substring(0, fi.Name.Length - fi.Extension.Length);
			}
			return "";
		}	

		public delegate void ExportPatchHandler(string filename);
		public ExportPatchHandler exportPatchHandler;
		private void ExportPatch_Click(object sender, RoutedEventArgs e)
		{
			SaveFileDialog sfd = new SaveFileDialog();
			if (LastPatchName != null)
			{
				FileInfo fi = new FileInfo(LastPatchName);
				sfd.InitialDirectory = fi.DirectoryName;
			}
			sfd.FileName = "64k2Patch.h";
			sfd.DefaultExt = ".h";
			sfd.Filter = "64klang2 Patch Header (.h)|*.h";
			Nullable<bool> result = sfd.ShowDialog();
			if (result == true)
			{
				if (exportPatchHandler != null)
					exportPatchHandler(sfd.FileName);
			}
		}

		public delegate void StartRecordingHandler();
		public StartRecordingHandler startRecordingHandler;

		public delegate void StopRecordingHandler();
		public StopRecordingHandler stopRecordingHandler;

		public delegate void ExportSongHandler(string filename, int timeQuant);
		public ExportSongHandler exportSongHandler;
		private void ExportSong_Checked(object sender, RoutedEventArgs e)
		{
			this.ExportSong.Content = "Recording ...";

			if (startRecordingHandler != null)
				startRecordingHandler();
		}
		private void ExportSong_Unchecked(object sender, RoutedEventArgs e)
		{
			if (stopRecordingHandler != null)
				stopRecordingHandler();

			this.ExportSong.Content = "Export Song";

			SaveFileDialog sfd = new SaveFileDialog();
			if (LastPatchName != null)
			{
				FileInfo fi = new FileInfo(LastPatchName);
				sfd.InitialDirectory = fi.DirectoryName;
			}
			sfd.FileName = "64k2Song.h";
			sfd.DefaultExt = ".h";
			sfd.Filter = "64klang2 Song Header (.h)|*.h";
			Nullable<bool> result = sfd.ShowDialog();
			if (result == true)
			{
                ComboBoxItem cbi = this.SongQuantization.SelectedItem as ComboBoxItem;
                int quantSamples = Convert.ToInt32(cbi.Content.ToString());
				if (exportSongHandler != null)
					exportSongHandler(sfd.FileName, quantSamples);
			}
		}

#endregion		

#region WaveFile

		private void EditWavetables_Click(object sender, RoutedEventArgs e)
		{
			if (WFD == null)
			{
				WFD = new WaveFileDialog(this);
				WFD.WaveEditClose.MouseLeftButtonDown += new MouseButtonEventHandler(WaveEditClose_MouseLeftButtonDown);
				this.EditGrid.Children.Add(WFD);
				Grid.SetRow(WFD, 1);
				Grid.SetColumn(WFD, 0);
				this.EditWavetables.IsEnabled = false;
			}
		}

		void WaveEditClose_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			this.EditGrid.Children.Remove(WFD);
			WFD = null;
			this.EditWavetables.IsEnabled = true;
		}
		
		public delegate int SetWaveFileHandler(int index, int frequency, string filename);
		public SetWaveFileHandler setWaveFileHandler;
		public int SetWaveFile(int index, int frequency, string filename)
		{
			if (setWaveFileHandler != null)
				return setWaveFileHandler(index, frequency, filename);
			else
				return 0;
		}

		public delegate int GetWaveFileFrequencyHandler(int index);
		public GetWaveFileFrequencyHandler getWaveFileFrequencyHandler;
		public int GetWaveFileFrequency(int index)
		{
			if (getWaveFileFrequencyHandler != null)
				return getWaveFileFrequencyHandler(index);
			else
				return 0;
		}

		public delegate string GetWaveFileNameHandler(int index);
		public GetWaveFileNameHandler getWaveFileNameHandler;
		public string GetWaveFileName(int index, bool withPath=false)
		{
			if (getWaveFileNameHandler != null)
			{
				string fname = getWaveFileNameHandler(index);
				if (fname != "")
				{
					FileInfo finfo = new FileInfo(fname);
					if (finfo.Exists)
					{
						if (withPath)
							return fname;
						else
							return finfo.Name;
					}
					else
						return "";
				}
				return "";
			}
			else
				return "";
		}

		public delegate int GetWaveFileCompressedSizeHandler(int index);
		public GetWaveFileCompressedSizeHandler getWaveFileCompressedSizeHandler;
		public int GetWaveFileCompressedSize(int index)
		{
			if (getWaveFileCompressedSizeHandler != null)
				return getWaveFileCompressedSizeHandler(index);
			else
				return 0;
		}

#endregion

		private void WantTopMost_Checked(object sender, RoutedEventArgs e)
		{
			this.Topmost = true;
		}

		private void WantTopMost_Unchecked(object sender, RoutedEventArgs e)
		{
			this.Topmost = false;
		}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if (!AllowClose)
			{
				this.WindowState = System.Windows.WindowState.Minimized;
				e.Cancel = true;
			}
		}

		private void JumpToChannel_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
			if (IsInitialized)
			{
				MainCanvas.SelectChannel(JumpToChannel.SelectedIndex, true);
				JumpToChannel.SelectedIndex = -1;
			}
		}

		private void SearchBox_TextChanged(object sender, TextChangedEventArgs e)
		{
			MainCanvas.HighlightSearchNodes(this.SearchBox.Text);
		}

		

	}
}
