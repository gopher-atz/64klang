using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace _64klang2GUI
{
	public class TouchScrolling : DependencyObject
	{
		public static bool GetIsEnabled(DependencyObject obj)
		{
			return (bool)obj.GetValue(IsEnabledProperty);
		}

		public static void SetIsEnabled(DependencyObject obj, bool value)
		{
			obj.SetValue(IsEnabledProperty, value);
		}

		public bool IsEnabled
		{
			get { return (bool)GetValue(IsEnabledProperty); }
			set { SetValue(IsEnabledProperty, value); }
		}

		public static readonly DependencyProperty IsEnabledProperty =
			DependencyProperty.RegisterAttached("IsEnabled", typeof(bool), typeof(TouchScrolling), new UIPropertyMetadata(false, IsEnabledChanged));

		static Dictionary<object, MouseCapture> _captures = new Dictionary<object, MouseCapture>();

		static void IsEnabledChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
		{
			var target = d as ScrollViewer;
			if (target == null) return;

			if ((bool)e.NewValue)
			{
				target.Loaded += target_Loaded;
			}
			else
			{
				target_Unloaded(target, new RoutedEventArgs());
			}
		}

		static void target_Unloaded(object sender, RoutedEventArgs e)
		{
			var target = sender as ScrollViewer;
			if (target == null) return;

			_captures.Remove(sender);

			target.Loaded -= target_Loaded;
			target.Unloaded -= target_Unloaded;
			target.PreviewMouseRightButtonDown -= target_PreviewMouseRightButtonDown;
			target.PreviewMouseMove -= target_PreviewMouseMove;
			target.PreviewMouseRightButtonUp -= target_PreviewMouseRightButtonUp;
			target.PreviewMouseWheel -= target_PreviewMouseWheel;
		}

		static void target_PreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
		{
			var target = sender as ScrollViewer;
			if (target == null) return;

			var point = e.GetPosition(target);

			double sbwidth  = target.VerticalScrollBarVisibility == ScrollBarVisibility.Visible ? SystemParameters.VerticalScrollBarWidth : 0;
			double sbheight = target.HorizontalScrollBarVisibility == ScrollBarVisibility.Visible ? SystemParameters.HorizontalScrollBarHeight : 0;
			if ((point.X >= target.ActualWidth - sbwidth) || point.Y >= target.ActualHeight - sbheight)
				return;
			
			_captures[sender] = new MouseCapture
			{
				HorizontalOffset = target.HorizontalOffset,
				VerticalOffset = target.VerticalOffset,
				Point = e.GetPosition(target),
			};
			target.Cursor = Cursors.SizeAll;
		}

		static void target_Loaded(object sender, RoutedEventArgs e)
		{
			var target = sender as ScrollViewer;
			if (target == null) return;

			target.Unloaded += target_Unloaded;
			target.PreviewMouseRightButtonDown += target_PreviewMouseRightButtonDown;
			target.PreviewMouseMove += target_PreviewMouseMove;
			target.PreviewMouseRightButtonUp += target_PreviewMouseRightButtonUp;
			target.PreviewMouseWheel += target_PreviewMouseWheel;
		}

		static void target_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
		{
			var target = sender as ScrollViewer;
			if (target == null) return;

			var canvas = target.Content as SynthCanvas;
			if (canvas == null) return;

			canvas.SetZoom(e, false);

			e.Handled = true;
		}

		static void target_PreviewMouseRightButtonUp(object sender, MouseButtonEventArgs e)
		{
			var target = sender as ScrollViewer;
			if (target == null) return;

			target.ReleaseMouseCapture();
			target.Cursor = Cursors.Arrow;
		}

		static void target_PreviewMouseMove(object sender, MouseEventArgs e)
		{
			if (!_captures.ContainsKey(sender)) return;

			if (e.RightButton != MouseButtonState.Pressed)
			{
				_captures.Remove(sender);
				return;
			}

			var target = sender as ScrollViewer;
			if (target == null) return;

			var capture = _captures[sender];

			var point = e.GetPosition(target);

			var dy = point.Y - capture.Point.Y;
			var dx = point.X - capture.Point.X;
			if (Math.Abs(dy) > 5 || Math.Abs(dx) > 5)
			{
				target.CaptureMouse();
			}

			target.ScrollToHorizontalOffset(capture.HorizontalOffset - dx);
			target.ScrollToVerticalOffset(capture.VerticalOffset - dy);
		}

		internal class MouseCapture
		{
			public Double HorizontalOffset { get; set; }
			public Double VerticalOffset { get; set; }
			public Point Point { get; set; }
		}
	}
}
