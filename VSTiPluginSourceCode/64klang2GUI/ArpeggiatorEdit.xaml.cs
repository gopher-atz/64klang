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
	/// Interaction logic for ArpeggiatorEdit.xaml
	/// </summary>
	public partial class ArpeggiatorEdit : UserControl
	{
		public GUISynthNode Node { get; private set; }

		int lastPlayPos;

		Point startPoint;
		int startStep;
		bool dragMode;
		ArpStep dragStep;
		public List<ArpStep> ArpStepList;

		public ArpeggiatorEdit(GUISynthNode node)
		{
			InitializeComponent();

			Node = node;

			ArpStepList = new List<ArpStep>();
			startPoint = new Point();
			startStep = 0;
			dragMode = false;
			dragStep = null;

			lastPlayPos = 0;
            CompositionTarget.Rendering += CompositionTarget_Rendering;
		}

        ~ArpeggiatorEdit()
		{
            CompositionTarget.Rendering -= CompositionTarget_Rendering;
		}

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
			int playpos = Node.synthCanvas.GetArpPlayPos(Node.ID);
			if (playpos != lastPlayPos)
			{
				PlayPos.Margin = new Thickness(lastPlayPos * 4 * 3, 0, 0, 0);
				lastPlayPos = playpos;
			}
		}

		public void ReadData()
		{
			ArpStep curStep = null;
			for (int i = 0; i < 32; i++)
			{
				int stepdata = Node.synthCanvas.GetArpStepData(Node.ID, i);

				int byte1 = stepdata & 0xff;
				int byte2 = stepdata >> 8;

				int gate = (byte1 >> 6) + 1;
				int transpose = (byte1 & 0x3f) - 32;
				int velocity = byte2;

				// new note
				if (velocity != 0)
				{
					if (curStep != null)
						curStep.UpdateStep();
					curStep = new ArpStep(this, i, transpose, gate);
					curStep.Velocity = velocity;
					ArpStepList.Add(curStep);
				}
				// hold
				else if ((byte1 != 0) && (byte2 == 0))
				{
					curStep.DeltaStepFractions += gate;
				}
			}
			if (curStep != null)
				curStep.UpdateStep();

			// set loop index
			int loopindex = Node.synthCanvas.GetArpStepData(Node.ID, -1);
			LoopStart.Margin = new Thickness(loopindex * 4 * 3, 0, 0, 0);
		}

		public void WriteData()
		{
			int[] steps = new int[32];
			for (int i = 0; i < 32; i++)
				steps[i] = 0;
			
			foreach (ArpStep a in ArpStepList)
			{
				int i = a.StartStep;
				int fs = a.DeltaStepFractions;
								
				int transpose = a.Transpose + 8;
				int gate = Math.Min(fs - 1, 3);
				int velocity = a.Velocity;

				int byte1 = transpose | (gate << 6);
				int byte2 = velocity;

				int stepdata = byte1 | (byte2 << 8);
				steps[i++] = stepdata;

				fs -= 4;
				// hold as long as needed
				while (fs > 0)
				{
					gate = Math.Min(fs - 1, 3);
					byte1 = transpose | (gate << 6);
					stepdata = byte1;
					steps[i++] = stepdata;
					fs -= 4;
				}
			}

			for (int i = 0; i < 32; i++)
			{
				Node.synthCanvas.SetArpStepData(Node.ID, i, steps[i]);
			}
		}

		private void StepGrid_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Node.synthCanvas.RaiseEditWindow(Node);

			startPoint = e.GetPosition(this.StepGrid);
			startStep = (int)(startPoint.X / (4.0 * 3.0));
			int startOctave = 3 - (int)(startPoint.Y / 24.0);

			// loop point set?
			if (startOctave > 2)
			{
				LoopStart.Margin = new Thickness(startStep * 4 * 3, 0, 0, 0);
				e.Handled = true;
				// update core data
				Node.synthCanvas.SetArpStepData(Node.ID, -1, startStep);
				return;
			}

			if (startOctave < -2)
				startOctave = -2;
			startOctave *= 12;

			dragStep = new ArpStep(this, startStep, startOctave);
			dragStep.ValidateInsertion();

			dragMode = true;
			this.StepGrid.CaptureMouse();
			this.StepGrid.Cursor = Cursors.Hand;

			e.Handled = true;
		}

		private void StepGrid_MouseMove(object sender, MouseEventArgs e)
		{
			if (dragMode)
			{
				Point p = e.GetPosition(this.StepGrid);

				if (Math.Abs(startPoint.X-p.X) < 2)
					return;
				else
					startPoint.X = -1;
				
				int clickStep = (int)(p.X / 3.0);
				int clickOctave = 3 - (int)(p.Y / 24.0);
				if (clickOctave < -2)
					clickOctave = -2;
				if (clickOctave > 2)
					clickOctave = 2;
				clickOctave *= 12;

				if (clickStep > 32 * 4)
					clickStep = 32 * 4;
				int deltaSteps = clickStep - (startStep*4);
				if (deltaSteps < 1)
					deltaSteps = 1;

				dragStep.Transpose = clickOctave+24;
				dragStep.DeltaStepFractions = deltaSteps;
				dragStep.UpdateStep();
				dragStep.ValidateInsertion();

				e.Handled = true;
			}
		}

		private void StepGrid_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			if (dragMode)
			{
				dragStep = null;
				dragMode = false;
				this.StepGrid.ReleaseMouseCapture();
				this.StepGrid.Cursor = Cursors.Arrow;

				e.Handled = true;

				WriteData();
			}
		}
	}

	public class ArpStep
	{		
		ArpeggiatorEdit AE;

		public int StartStep;
		public int DeltaStepFractions;
		public int Transpose;
		public int Velocity;

		public Border StepBorder;
		public Border VelocityBorder;
		public Label TransposeLabel;
		public Label VelocityLabel;

		private bool TransposeDrag;
		private int TransposeStart;
		private Point TransposeStartPoint;
		private bool VelocityDrag;
		private int VelocityStart;
		private Point VelocityStartPoint;

		public ArpStep(ArpeggiatorEdit ae, int start, int transpose, int delta=4)
		{
			AE = ae;
			
			StartStep = start;
			Transpose = transpose + 24;
			DeltaStepFractions = delta;
			Velocity = 127;

			StepBorder = null;
			VelocityBorder = null;
			TransposeLabel = null;
			VelocityLabel = null;			

			AE.ArpStepList.Add(this);
			UpdateStep();
		}

		public void ValidateInsertion()
		{
			// remove/split existing ArpSteps
			int il = StartStep;
			int ir = StepIndex(StartStep * 4 + DeltaStepFractions);

			ArpStep[] arpSteps = new ArpStep[AE.ArpStepList.Count];
			AE.ArpStepList.CopyTo(arpSteps);
			foreach (ArpStep a in arpSteps)
			{
				if (a == this)
					continue;

				int al = a.StartStep;
				int ar = StepIndex(a.StartStep * 4 + a.DeltaStepFractions);

				// no overlap, go to next
				if (ir < al || il > ar)
					continue;
				// exact or more overlap, remove and done
				if (il <= al && ir >= ar)
				{
					a.RemoveFromGrid();
					break;
				}
				// split, adjust left span and add new span to the right, and done
				if (il > al && ir < ar)
				{
					int oldStepFractions = a.DeltaStepFractions;
					// adjust old step to be on the left
					a.DeltaStepFractions = (il - al) * 4;
					// create new step on the right
					ArpStep rStep = new ArpStep(AE, ir + 1, a.Transpose - 24, oldStepFractions - a.DeltaStepFractions - (1 + ir - il) * 4);
					rStep.Velocity = Velocity;
					// adjust old step to be on the left
					a.UpdateStep();
					break;
				}
				// trim right or left
				else
				{
					if (ir < ar)
					{
						int dsteps = ir + 1 - al;
						a.StartStep += dsteps;
						a.DeltaStepFractions -= dsteps * 4;
						a.UpdateStep();
						break;
					}
					else if (il > al)
					{
						a.DeltaStepFractions = (il - al) * 4;
						a.UpdateStep();
						break;
					}
					// need to remove the last step?
					else if (al == ar)
					{
						a.RemoveFromGrid();
						break;
					}
					else
					{
						//throw new NotImplementedException();
					}
				}
			}
		}

		int StepIndex(int fStep)
		{
			if ((fStep % 4) != 0)
			{
				fStep = fStep / 4;
				fStep++;
			}
			else
			{
				fStep = fStep / 4;
			}
			return fStep-1;
		}

		public void UpdateStep()
		{
			RemoveFromGrid();
			
			StepBorder = new Border();
			StepBorder.Margin = new Thickness(0.5 + StartStep * 4 * 3, 0.5 + 72.0 - (Transpose-24) * 2, 0, 0);
			StepBorder.Width = DeltaStepFractions * 3 - 1;
			StepBorder.Height = 23;
			StepBorder.CornerRadius = new CornerRadius(6);
			StepBorder.Background = Brushes.PaleGoldenrod;
			StepBorder.Opacity = 0.75;
			StepBorder.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
			StepBorder.VerticalAlignment = System.Windows.VerticalAlignment.Top;
			StepBorder.PreviewMouseLeftButtonDown += new MouseButtonEventHandler(StepBorder_PreviewMouseLeftButtonDown);

			VelocityBorder = new Border();
			VelocityBorder.Margin = new Thickness(0.5 + StartStep * 4 * 3, 143.5 - 0.9375 * Velocity, 0, 0);
			VelocityBorder.Width = DeltaStepFractions * 3 - 1;
			VelocityBorder.Height = 0.9375 * Velocity;
			VelocityBorder.CornerRadius = new CornerRadius(6);
			VelocityBorder.Background = Brushes.LightSalmon;
			VelocityBorder.Opacity = 0.5;
			VelocityBorder.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
			VelocityBorder.VerticalAlignment = System.Windows.VerticalAlignment.Top;

			int dsf = (StepIndex(DeltaStepFractions) + 1) * 4;
			double width = dsf * 3;

			TransposeLabel = new Label();
			TransposeLabel.Margin = new Thickness(StartStep * 4 * 3, 0, 0, 0);
			TransposeLabel.Width = width;
			TransposeLabel.Height = 24;
			TransposeLabel.Background = Brushes.PaleGoldenrod;
			TransposeLabel.Padding = new Thickness(0);
			TransposeLabel.FontSize = 6;
			TransposeLabel.BorderBrush = Brushes.Black;
			TransposeLabel.BorderThickness = new Thickness(0.5);
			TransposeLabel.Content = (Transpose-24).ToString();
			TransposeLabel.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
			TransposeLabel.VerticalAlignment = System.Windows.VerticalAlignment.Top;
			TransposeLabel.HorizontalContentAlignment = System.Windows.HorizontalAlignment.Center;
			TransposeLabel.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
			TransposeLabel.MouseLeftButtonDown += new MouseButtonEventHandler(TransposeLabel_MouseLeftButtonDown);
			TransposeLabel.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(TransposeLabel_MouseLeftButtonUp);
			TransposeLabel.PreviewMouseMove += new MouseEventHandler(TransposeLabel_MouseMove);

			VelocityLabel = new Label();
			VelocityLabel.Margin = new Thickness(StartStep * 4 * 3, 0, 0, 0);
			VelocityLabel.Width = width;
			VelocityLabel.Height = 24;
			VelocityLabel.Background = Brushes.LightSalmon;
			VelocityLabel.Padding = new Thickness(0);
			VelocityLabel.FontSize = 6;
			VelocityLabel.BorderBrush = Brushes.Black;
			VelocityLabel.BorderThickness = new Thickness(0.5);
			VelocityLabel.Content = Velocity.ToString();
			VelocityLabel.HorizontalAlignment = System.Windows.HorizontalAlignment.Left;
			VelocityLabel.VerticalAlignment = System.Windows.VerticalAlignment.Top;
			VelocityLabel.HorizontalContentAlignment = System.Windows.HorizontalAlignment.Center;
			VelocityLabel.VerticalContentAlignment = System.Windows.VerticalAlignment.Center;
			VelocityLabel.MouseLeftButtonDown += new MouseButtonEventHandler(VelocityLabel_MouseLeftButtonDown);
			VelocityLabel.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(VelocityLabel_MouseLeftButtonUp);
			VelocityLabel.PreviewMouseMove += new MouseEventHandler(VelocityLabel_MouseMove);
			
			AddToGrid();						
		}

		void StepBorder_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			if ((Keyboard.Modifiers & ModifierKeys.Control) > 0)
			{
				Border b = sender as Border;
				foreach (ArpStep a in AE.ArpStepList)
				{
					if (a.StepBorder == b)
					{
						RemoveFromGrid();
						e.Handled = true;
						AE.WriteData();
						break;
					}
				}
			}
		}

		void TransposeLabel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Label l = sender as Label;
			foreach (ArpStep a in AE.ArpStepList)
			{
				if (a.TransposeLabel == l)
				{
					TransposeStart = a.Transpose;
					break;
				}
			}
			TransposeStartPoint = e.GetPosition(l);
			TransposeDrag = true;
			l.CaptureMouse();
			l.Cursor = Cursors.ScrollNS;

			e.Handled = true;
		}

		void TransposeLabel_MouseMove(object sender, MouseEventArgs e)
		{
			Label l = sender as Label;
			if (TransposeDrag)
			{
				Point p = e.GetPosition(l);
				double dy = (TransposeStartPoint.Y - p.Y)/4.0;
				foreach (ArpStep a in AE.ArpStepList)
				{
					if (a.TransposeLabel == l)
					{
						a.Transpose = TransposeStart + (int)dy;
						if (a.Transpose < 0)
							a.Transpose = 0;
						if (a.Transpose > 48)
							a.Transpose = 48;
						a.TransposeLabel.Content = (a.Transpose - 24).ToString();
						a.StepBorder.Margin = new Thickness(0.5 + a.StartStep * 4 * 3, 0.5 + 72.0 - (a.Transpose - 24) * 2, 0, 0);
						AE.WriteData();
						break;
					}
				}
			}
		}

		void TransposeLabel_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			Label l = sender as Label;
			if (TransposeDrag)
			{
				foreach (ArpStep a in AE.ArpStepList)
				{
					if (a.TransposeLabel == l)
					{
						TransposeDrag = false;
						l.ReleaseMouseCapture();
						l.Cursor = Cursors.Arrow;
						a.UpdateStep();
						AE.WriteData();
						break;
					}
				}
			}
		}

		void VelocityLabel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			Label l = sender as Label;
			foreach (ArpStep a in AE.ArpStepList)
			{
				if (a.VelocityLabel == l)
				{
					VelocityStart = a.Velocity;
					break;
				}
			}
			VelocityStartPoint = e.GetPosition(l);
			VelocityDrag = true;
			l.CaptureMouse();
			l.Cursor = Cursors.ScrollNS;
			e.Handled = true;
		}

		void VelocityLabel_MouseMove(object sender, MouseEventArgs e)
		{
			Label l = sender as Label;
			if (VelocityDrag)
			{
				Point p = e.GetPosition(l);
				double dy = (VelocityStartPoint.Y - p.Y) / 2.0;
				foreach (ArpStep a in AE.ArpStepList)
				{
					if (a.VelocityLabel == l)
					{
						a.Velocity = VelocityStart + (int)dy;
						if (a.Velocity < 1)
							a.Velocity = 1;
						if (a.Velocity > 127)
							a.Velocity = 127;
						a.VelocityLabel.Content = a.Velocity.ToString();
						a.VelocityBorder.Margin = new Thickness(0.5 + a.StartStep * 4 * 3, 143.5 - 0.9375 * a.Velocity, 0, 0);
						a.VelocityBorder.Height = 0.9375 * a.Velocity;
						AE.WriteData();
						break;
					}
				}
			}
		}

		void VelocityLabel_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			Label l = sender as Label;
			if (VelocityDrag)
			{
				foreach (ArpStep a in AE.ArpStepList)
				{
					if (a.VelocityLabel == l)
					{
						VelocityDrag = false;
						l.ReleaseMouseCapture();
						l.Cursor = Cursors.Arrow;
						a.UpdateStep();
						AE.WriteData();
						break;
					}
				}
			}
		}

		

		public void AddToGrid()
		{
			AE.StepGrid.Children.Add(VelocityBorder);
			AE.StepGrid.Children.Add(StepBorder);
			AE.TransposeGrid.Children.Add(TransposeLabel);
			AE.VelocityGrid.Children.Add(VelocityLabel);

			AE.ArpStepList.Add(this);
		}

		public void RemoveFromGrid()
		{
			if (VelocityBorder != null)
				AE.StepGrid.Children.Remove(VelocityBorder);
			if (StepBorder != null)
				AE.StepGrid.Children.Remove(StepBorder);
			if (TransposeLabel != null)
				AE.TransposeGrid.Children.Remove(TransposeLabel);
			if (VelocityLabel != null)
				AE.VelocityGrid.Children.Remove(VelocityLabel);

			StepBorder = null;
			VelocityBorder = null;
			TransposeLabel = null;
			VelocityLabel = null;

			AE.ArpStepList.Remove(this);
		}


	}
}
