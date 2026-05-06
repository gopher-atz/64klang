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
    /// Interaction logic for VUMeter.xaml
    /// </summary>
    public partial class VUMeter : UserControl
    {
        public VUMeter()
        {
            InitializeComponent();

            _gradientBrush = new LinearGradientBrush();
            _gradientBrush.StartPoint = new Point(0.5, 1.0);
            _gradientBrush.EndPoint = new Point(0.5, 0.0);
            _gradientBrush.GradientStops.Add(new GradientStop(Colors.Red, 1.0));
            _gradientBrush.GradientStops.Add(new GradientStop(Colors.Yellow, 0.7));
            _gradientBrush.GradientStops.Add(new GradientStop(Colors.Lime, 0.0));
            _gradientBrush.Freeze();

            _backgroundBrush = new SolidColorBrush(Colors.Black);
            _backgroundBrush.Freeze();

            _grayBrush = new SolidColorBrush(Colors.Gray);
            _grayBrush.Freeze();
        }

        public double LeftHeight
        {
            get
            {
                return _leftHeight;
            }
            set
            {
                if (_leftHeight == value)
                    return;

                _leftHeight = value;
                InvalidateVisual();
            }
        }

        public double RightHeight
        {
            get
            {
                return _rightHeight;
            }
            set
            {
                if (_rightHeight == value)
                    return;

                _rightHeight = value;
                InvalidateVisual();
            }
        }

        public bool DrawBackground
        {
            get
            {
                return _drawBackground;
            }
            set
            {
                if (_drawBackground == value)
                    return;

                _drawBackground = value;
                InvalidateVisual();
            }
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);

            var w = this.ActualWidth;
            var h = this.ActualHeight;

            drawingContext.DrawRectangle(DrawBackground ? _grayBrush : _backgroundBrush, null, new Rect(0, 0, w, h));

            drawingContext.PushClip(new RectangleGeometry(new Rect(0, h * (1 - LeftHeight), w / 2, h * LeftHeight)));
            drawingContext.DrawRectangle(_gradientBrush, null, new Rect(0, 0, w / 2, h));
            drawingContext.Pop();

            drawingContext.PushClip(new RectangleGeometry(new Rect(w / 2, h * (1 - RightHeight), w / 2, h * RightHeight)));
            drawingContext.DrawRectangle(_gradientBrush, null, new Rect(w / 2, 0, w / 2, h));
            drawingContext.Pop();
        }

        private LinearGradientBrush _gradientBrush;
        private SolidColorBrush _backgroundBrush;
        private SolidColorBrush _grayBrush;
        private double _leftHeight = 0.4;
        private double _rightHeight = 0.6;
        private bool _drawBackground = true;
    }
}
