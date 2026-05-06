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
using System.Reflection;
using Microsoft.Win32;

namespace _64klang2GUI
{
	public class NodeConnection
	{
		public Path PathRef { get; set; }
		public int From { get; set; }
		public int To { get; set; }
		public int Index { get; set; }

		public NodeConnection(int from, int to, int index)
		{
			PathRef = null;
			From = from;
			To = to;
			Index = index;
		}		
	}

	/// <summary>
	/// Interaktionslogik für SynthCanvas.xaml
	/// </summary>
	public partial class SynthCanvas : Canvas
	{
		SynthWindow mainWindow;
		public SynthWindow MainWindow { get { return mainWindow; } set { mainWindow = value; } }
		Point _mousePos;
		Point _nodeClickPos;
		GUISynthNode _dragNode;
		double _zoom;
		public double Zoom() { return _zoom; }
		ContextMenu _popup;		

		Dictionary<Path, NodeConnection>	pathConnectionMap;
		List<GUISynthNode> selectedNodes;
		List<Path> selectedPaths;
		Path contextPath;
		Rectangle selectionRect;
		Point selectionStartPos;
		NodeConnection pendingConnection;

		public SynthCanvas()
		{
			InitializeComponent();

			// scale transform
			ScaleTransform st = new ScaleTransform();
			this.LayoutTransform = st;

			// context menu
			_popup = this.ContextMenu;
			// disable paste and save selection menu items
			MenuItem mi = _popup.Items[0] as MenuItem;
			mi.IsEnabled = false;
			mi = _popup.Items[1] as MenuItem;
			mi.IsEnabled = false;

			try
			{
				string xmlName = System.IO.Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "/64klang2Config.xml";
				XmlDocument doc = new XmlDocument();
				doc.XmlResolver = null;
				doc.Load(xmlName);
				// menu hierarchy
				foreach (XmlNode n in doc.DocumentElement.ChildNodes[1].ChildNodes)
				{
					XmlElement e = n as XmlElement;
					// create submenu
					MenuItem menu = new MenuItem();
					menu.Header = e.GetAttribute("Header");
					_popup.Items.Add(menu);
					// insert items
					foreach (XmlNode n2 in n.ChildNodes)
					{
						e = n2 as XmlElement;
						if (e != null)
						{
							MenuItem item = new MenuItem();
							int id = Convert.ToInt32(e.GetAttribute("id"));
							item.Header = NodeInfo.Info(id).Name;
							item.Click += new RoutedEventHandler(NodeMenuItem_Click);
							menu.Items.Add(item);
						}
					}
				}
			}
			catch
			{

			}
			this.ContextMenu = null;

			pendingConnection = null;

			Reset();				
		}

		public void Reset(bool resetViewer = true)
		{
			if (resetViewer)
			{
				_zoom = 1.0;
			}
			_mousePos = new Point(0, 0);
			_nodeClickPos = new Point(0, 0);
			_dragNode = null;			
			pathConnectionMap = new Dictionary<Path, NodeConnection>();
			selectedNodes = new List<GUISynthNode>();
			selectedPaths = new List<Path>();
			contextPath = null;
			selectionRect = null;
			selectionStartPos = new Point();
			pendingConnection = null;
			KillNodeTimers();
			GUISynthNode.IDNodeMap.Clear();
			Children.Clear();
			if (MainWindow != null)
				MainWindow.EditStackPanel.Children.Clear();
		}

		public void KillNodeTimers()
		{
			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				node.KillTimer();
				if (node.IsEditOpen > 0)
					node.EditWindow.KillTimer();
			}
		}

		public void SetZoomOut(double factor)
		{
			_zoom = factor;
			// update layout transform with current zoom
			ScaleTransform st = this.LayoutTransform as ScaleTransform;
			st.ScaleX = _zoom;
			st.ScaleY = _zoom;
			this.LayoutTransform = st;
		}

		public void SetZoom(MouseWheelEventArgs e, bool absolute)
		{
			ScrollViewer sw = this.Parent as ScrollViewer;
			if (sw == null)
				return;

			double parentwidth = sw.ActualWidth - SystemParameters.VerticalScrollBarWidth;
			double parentheight = sw.ActualHeight - SystemParameters.HorizontalScrollBarHeight;

			Point pc = e.GetPosition(this);
			Point ps = e.GetPosition(sw);

			double zoom = e.Delta;
			double oldzoom = _zoom;
			if (absolute)
			{
				_zoom = zoom;
			}
			else
			{
				if (zoom > 0)
					zoom = 1.0 / (1.0 - zoom / 1200.0);
				else
					zoom = 1.0 + zoom / 1200.0;
				_zoom *= zoom;
			}

			if (_zoom > 5)
				_zoom = oldzoom;
			if (32768 * _zoom < parentwidth)
				_zoom = oldzoom;
			if (32768 * _zoom < parentheight)
				_zoom = oldzoom;

			// update layout transform with current zoom
			ScaleTransform st = this.LayoutTransform as ScaleTransform;
			st.ScaleX = _zoom;
			st.ScaleY = _zoom;
			this.LayoutTransform = st;

			// move scrollviewer so it stays focussed at mouse position if possible
			sw.ScrollToHorizontalOffset(pc.X * _zoom - ps.X);
			sw.ScrollToVerticalOffset(pc.Y * _zoom - ps.Y);
		}

		private void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			if ((Keyboard.Modifiers & ModifierKeys.Control) == 0)
				ClearSelection();

			_dragNode = null;

			// readd the selectinrect
			if (selectionRect != null)
				this.Children.Remove(selectionRect);
			selectionRect = new Rectangle();
			selectionRect.Width = 0;
			selectionRect.Height = 0;
			selectionRect.Fill = Brushes.White;
			selectionRect.Opacity = 0.3;
			this.Children.Add(selectionRect);
			selectionStartPos = e.GetPosition(this);

			// clear path selection
			ClearContextPath();			

			// clear pending connection
			ClearPendingConnection();

			this.CaptureMouse();
		}

		private void Canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
		{
			_mousePos = e.GetPosition(this);
			this.ContextMenu = _popup;
		}

		private void Canvas_MouseMove(object sender, MouseEventArgs e)
		{
            try
            {
                // no context menu when moving
                this.ContextMenu = null;

                Point currentPos = e.GetPosition(this);

                // dragging nodes?
                if (_dragNode != null && _dragNode.IsSelected)
                {
                    // move nodes
                    Point delta = new Point((currentPos.X - _nodeClickPos.X) - Canvas.GetLeft(_dragNode),
                                            (currentPos.Y - _nodeClickPos.Y) - Canvas.GetTop(_dragNode));
                    // update node positions
                    List<Path> movePaths = new List<Path>();
                    foreach (GUISynthNode node in selectedNodes)
                    {
                        node.UpdateCanvasPosition(delta);

                        foreach (NodeConnection con in pathConnectionMap.Values)
                        {
                            if (con.From == node.ID || con.To == node.ID)
                                movePaths.Add(con.PathRef);
                        }
                    }
                    // update affected connections
                    foreach (Path path in movePaths)
                    {
                        UpdateConnectionPath(path, currentPos);
                    }
                }
                // draw selection rect
                else if (selectionRect != null)
                {
                    double x1 = selectionStartPos.X;
                    double y1 = selectionStartPos.Y;
                    double x2 = e.GetPosition(this).X;
                    double y2 = e.GetPosition(this).Y;

                    double bx1 = Math.Min(x1, x2);
                    double bx2 = Math.Max(x1, x2);
                    double by1 = Math.Min(y1, y2);
                    double by2 = Math.Max(y1, y2);

                    Canvas.SetLeft(selectionRect, bx1);
                    selectionRect.Width = bx2 - bx1;
                    Canvas.SetTop(selectionRect, by1);
                    selectionRect.Height = by2 - by1;

                    // gather
                    foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
                    {
                        if ((Canvas.GetLeft(node) > bx1) && ((Canvas.GetLeft(node) + node.ActualWidth) < bx2) &&
                            (Canvas.GetTop(node) > by1) && ((Canvas.GetTop(node) + node.ActualHeight) < by2))
                        {
                            if (!node.IsSelected)
                            {
                                AddSelection(node);
                            }
                        }
                        else
                        {
                            RemoveSelection(node);
                        }
                    }
                }
                // draw pending connection
                if (pendingConnection != null)
                {
                    UpdateConnectionPath(pendingConnection.PathRef, currentPos);
                }
            }
            catch (Exception ex)
            {
            }
		}

		private void Canvas_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			this.ReleaseMouseCapture();
			this.Children.Remove(selectionRect);
			selectionRect = null;
		}

		public void SelectChannel(int channel, bool bringToView = true)
		{
			int searchtype = 1;
			if (channel == 16)
				searchtype = 0;

			GUISynthNode foundnode = null;
			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				if (node.Info.TypeID == searchtype)
				{
					if (searchtype == 0)
					{
						foundnode = node;
						break;
					}
					else if (node.Channel == channel)
					{
						foundnode = node;
						break;
					}
				}
			}

			if (foundnode != null)
			{
				ClearSelection();
				RecursiveAddSelection(foundnode, true);
				if (bringToView == true)
					foundnode.BringIntoView();
			}
		}

		public void UpdateChannelName(int channel)
		{
			GUISynthNode foundnode = null;
			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				if ((node.Info.TypeID == 1) && (node.Channel == channel))
				{
					foundnode = node;
					break;
				}
			}
			string newname = "";
			if (foundnode != null)
			{
				newname = GetNodeName(foundnode.ID);
				TextBox nameBox = foundnode.NodeAdditionalVerticalStackPanel.Children[2] as TextBox;
				nameBox.Text = newname;
			}
		}

		public string GetChannelName(int channel, bool displayname)
		{
			GUISynthNode foundnode = null;
			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				if ((node.Info.TypeID == 1) && (node.Channel == channel))
				{
					foundnode = node;
					break;
				}
			}

			if (foundnode != null)
			{
				if (displayname)
				{
					return foundnode.NodeName.Content.ToString();
				}
				else
				{
					TextBox nameBox = foundnode.NodeAdditionalVerticalStackPanel.Children[2] as TextBox;
					return nameBox.Text;
				}
			}
			else
				return "Channel " + channel.ToString();
		}

        public delegate void SetNodeProcessingFlagsHandler(int id, int flags);
        public SetNodeProcessingFlagsHandler setNodeProcessingFlagsHandler;
        public void SetNodeProcessingFlags(int id, int flags)
        {
            if (setNodeProcessingFlagsHandler != null)
                setNodeProcessingFlagsHandler(id, flags);
        }

		public delegate void SetNodeNameHandler(int id, string name);
		public SetNodeNameHandler setNodeNameHandler;
		public void SetNodeName(int id, string name)
		{
			if (setNodeNameHandler != null)
				setNodeNameHandler(id, name);
		}

		public delegate string GetNodeNameHandler(int id);
		public GetNodeNameHandler getNodeNameHandler;
		public string GetNodeName(int id)
		{
			if (getNodeNameHandler != null)
				return getNodeNameHandler(id);
			else
				return "";
		}

		public delegate int CopyNodeHandler(int type, int x, int y, int copyFlag);
		public CopyNodeHandler copyNodeHandler;
		private void PasteSelection_Click(object sender, RoutedEventArgs e)
		{
            try
            {
                if (copyNodeHandler != null)
                {
                    // get min gui xy
                    double mx = 1000000000.0;
                    double my = 1000000000.0;
                    foreach (GUISynthNode node in selectedNodes)
                    {
                        mx = Math.Min(mx, Canvas.GetLeft(node));
                        my = Math.Min(my, Canvas.GetTop(node));
                    }

                    // special paste? 0 = normal, 1 = as global, 2 = as local
                    int specialPaste = 0;
                    if (((Keyboard.Modifiers & ModifierKeys.Control) != 0) && ((Keyboard.Modifiers & ModifierKeys.Shift) == 0))
                        specialPaste = 1;
                    if (((Keyboard.Modifiers & ModifierKeys.Control) != 0) && ((Keyboard.Modifiers & ModifierKeys.Shift) != 0))
                        specialPaste = 2;

                    // create copies of allowed nodes and create a mapping
                    Dictionary<GUISynthNode, int> copyMap = new Dictionary<GUISynthNode, int>();
                    foreach (GUISynthNode node in selectedNodes)
                    {
                        // dont ever copy SynthRoot, ChannelRoot, NoteController
                        if (node.Info.TypeID < 3)
                            continue;
                        // paste as global nodes?
                        if (specialPaste == 1)
                        {
                            // remove voice root or voice nodes as they cannot be pasted as global nodes
                            if ((node.Info.TypeID == 4) || (node.Info.TypeID > 64))
                                continue;
                        }
                        // paste as local nodes?
                        if (specialPaste == 2)
                        {
                            // remove voicemanager nodes as they cannot be pasted as local nodes
                            if (node.Info.TypeID == 3)
                                continue;
                        }

                        int newnodeid = copyNodeHandler(node.ID, (int)(_mousePos.X + Canvas.GetLeft(node) - mx), (int)(_mousePos.Y + Canvas.GetTop(node) - my), specialPaste);
                        copyMap.Add(node, newnodeid);
                    }
                    // establish internal connections for the copied nodes
                    foreach (GUISynthNode oldnode in copyMap.Keys)
                    {
                        int newnodeid = copyMap[oldnode];
                        int numinputs = oldnode.Info.NumInputs;
                        if (oldnode.Info.VariableInput)
                            numinputs = oldnode.NumVariableInputs;
                        // search old nodes inputs for connections within the selection
                        for (int i = 0; i < numinputs; i++)
                        {
                            int oldinputid = oldnode.GetInput(i);
                            // not a default zero connection?
                            if (oldinputid != -1 && GUISynthNode.IDNodeMap.ContainsKey(oldinputid))
                            {
                                GUISynthNode oldinput = GUISynthNode.IDNodeMap[oldinputid];
                                if (copyMap.ContainsKey(oldinput))
                                {
                                    int newinputid = copyMap[oldinput];
                                    // add new connection
                                    AddConnection(newinputid, newnodeid, i);
                                    // connect in core
                                    if (connectionInputHandler != null)
                                        connectionInputHandler(newinputid, newnodeid, i, true);
                                }
                            }
                        }
                    }

                    // select the copied nodes
                    ClearSelection();
                    foreach (int newnodeid in copyMap.Values)
                    {
                        if (GUISynthNode.IDNodeMap.ContainsKey(newnodeid))
                        {
                            GUISynthNode newnode = GUISynthNode.IDNodeMap[newnodeid];
                            AddSelection(newnode);
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
		}

		public static string LastSelectionName = null;

		public delegate void ClearSelectionHandler();
		public ClearSelectionHandler clearSelectionHandler;

		public delegate void SetSelectedHandler(int id, int selected);
		public SetSelectedHandler setSelectedHandler;

		public delegate void SaveSelectionHandler(string filename);
		public SaveSelectionHandler saveSelectionHandler;
		private void SaveSelection_Click(object sender, RoutedEventArgs e)
		{
			if (clearSelectionHandler != null)
				clearSelectionHandler();		

			int saveCount = 0;
			foreach (GUISynthNode node in selectedNodes)
			{
				// dont ever save SynthRoot, ChannelRoot, NoteController
				if (node.Info.TypeID < 3)
					continue;
				saveCount++;

				setSelectedHandler(node.ID, 1);
			}
			if (saveCount == 0)
				return;

			SaveFileDialog sfd = new SaveFileDialog();
			if (LastSelectionName != null)
				sfd.FileName = LastSelectionName;
			sfd.DefaultExt = ".64k2Selection";
			sfd.Filter = "64klang2 Selection|*.64k2Selection";
			Nullable<bool> result = sfd.ShowDialog();
			if (result == true)
			{
				LastSelectionName = sfd.FileName;

				System.IO.FileInfo fi = new System.IO.FileInfo(sfd.FileName);
				mainWindow.CurrentDirectory = fi.DirectoryName;

				if (saveSelectionHandler != null)
					saveSelectionHandler(sfd.FileName);
			}
		}		

		public delegate void LoadSelectionHandler(string filename, int x, int y);
		public LoadSelectionHandler loadSelectionHandler;
		private void LoadSelection_Click(object sender, RoutedEventArgs e)
		{
			OpenFileDialog ofd = new OpenFileDialog();
			if (LastSelectionName != null)
			{
				System.IO.FileInfo fi = new System.IO.FileInfo(LastSelectionName);
				ofd.InitialDirectory = fi.DirectoryName;
			}
			else
			{
				ofd.InitialDirectory = mainWindow.CurrentDirectory;
			}
			ofd.DefaultExt = ".64k2Selection";
			ofd.Filter = "64klang2 Selection|*.64k2Selection";
			Nullable<bool> result = ofd.ShowDialog();
			if (result == true)
			{
				ClearSelection();

				System.IO.FileInfo fi = new System.IO.FileInfo(ofd.FileName);
				ofd.InitialDirectory = mainWindow.CurrentDirectory;
				mainWindow.CurrentDirectory = fi.DirectoryName;

				LastSelectionName = ofd.FileName;
				if (loadSelectionHandler != null)
					loadSelectionHandler(ofd.FileName, (int)(_mousePos.X), (int)(_mousePos.Y));
			}
		}

		public delegate void MenuItemHandler(string message);
		public MenuItemHandler menuItemHandler;
		private void MenuItem_Click(object sender, RoutedEventArgs e)
		{
			MenuItem item = (MenuItem)sender;
			if (menuItemHandler != null)
			{
				menuItemHandler(item.Header.ToString());
			}
		}

		public delegate void NodeMenuItemHandler(int type, int channel, bool isGlobal, int x, int y);
		public NodeMenuItemHandler nodeMenuItemHandler;
		private void NodeMenuItem_Click(object sender, RoutedEventArgs e)
		{
            try
            {
                MenuItem item = (MenuItem)sender;
                if (nodeMenuItemHandler != null)
                {
                    string selected = item.Header.ToString();
                    int id = 0;
                    for (id = 0; id < NodeInfo.MaxTypeID(); id++)
                    {
                        if (selected == NodeInfo.Info(id).Name)
                            break;
                    }

                    // global or local path selected?
                    bool isGlobal = false;
                    if ((contextPath != null) && pathConnectionMap.ContainsKey(contextPath))
                    {
                        NodeConnection con = pathConnectionMap[contextPath];
                        GUISynthNode fromNode = GUISynthNode.IDNodeMap[con.From];
                        GUISynthNode toNode = GUISynthNode.IDNodeMap[con.To];
                        isGlobal = fromNode.IsGlobal;
                        // dont allow signal insertion for voiceroot to voicemanager and 
                        // from  voicemanager to notecontroller
                        if ((fromNode.Info.TypeID == 4 && toNode.Info.TypeID == 3) &&
                            (fromNode.Info.TypeID == 3 && toNode.Info.TypeID == 2))
                        {
                            ClearContextPath();
                        }
                    }
                    else if ((Keyboard.Modifiers & ModifierKeys.Control) > 0)
                    {
                        isGlobal = true;
                    }

                    // voice constants never global
                    if (id > NodeInfo.ConstantTypeID())
                        isGlobal = false;

                    nodeMenuItemHandler(id, -2, isGlobal, (int)_mousePos.X, (int)_mousePos.Y);
                    // clear path selection
                    ClearContextPath();
                }
            }
            catch (Exception ex)
            {

            }
		}

		public void AddNode(int id, int type, int channel, bool isGlobal, double x, double y, string name)
		{
            try
            {
                GUISynthNode node = new GUISynthNode(this, id, type, channel, isGlobal, name);
                node.MouseLeftButtonDown += new MouseButtonEventHandler(node_MouseLeftButtonDown);
                node.MouseLeftButtonUp += new MouseButtonEventHandler(node_MouseLeftButtonUp);
                node.MouseMove += new MouseEventHandler(Canvas_MouseMove);
                this.Children.Add(node);
                Canvas.SetZIndex(node, 1);

                Canvas.SetLeft(node, x);
                Canvas.SetTop(node, y);

                // insert to a selected path -> establish connection if possible
                if ((contextPath != null) && pathConnectionMap.ContainsKey(contextPath) && (node.Info.AllowSignalInsertion))
                {
                    NodeConnection con = pathConnectionMap[contextPath];
                    GUISynthNode fromNodeOld = GUISynthNode.IDNodeMap[con.From];
                    GUISynthNode toNodeOld = GUISynthNode.IDNodeMap[con.To];
                    int oldIndex = con.Index;

                    // remove old connection
                    InputClicked(toNodeOld, oldIndex);

                    if (toNodeOld.Info.VariableInput)
                        oldIndex = -1;

                    // reconnect with the new node in the middle
                    ClearPendingConnection();
                    NewPendingConnection(fromNodeOld);
                    InputClicked(node, 0);
                    ClearPendingConnection();
                    NewPendingConnection(node);
                    InputClicked(toNodeOld, oldIndex);
                    ClearPendingConnection();
                }
                else
                {
                    node.UpdateInputStates();
                }
            }
            catch (Exception ex)
            {

            }
		}

		public double GetNodeX(int id)
		{
			if (GUISynthNode.IDNodeMap.ContainsKey(id))
			{
				GUISynthNode node = GUISynthNode.IDNodeMap[id];
				return Canvas.GetLeft(node);
			}
			return -1.0;
		}

		public double GetNodeY(int id)
		{
			if (GUISynthNode.IDNodeMap.ContainsKey(id))
			{
				GUISynthNode node = GUISynthNode.IDNodeMap[id];
				return Canvas.GetTop(node);
			}
			return -1.0;
		}

		void node_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
		{
			_dragNode = null;
		}

		public void ClearSelection()
		{
			// clear selection
			foreach (GUISynthNode node in selectedNodes)
			{
				node.IsSelected = false;
				Canvas.SetZIndex(node, 1);
			}
			selectedNodes.Clear();
			// restore highlighted paths
			foreach (Path path in selectedPaths)
			{
				if (!pathConnectionMap.ContainsKey(path))
					continue;
				NodeConnection con = pathConnectionMap[path];
                if (!GUISynthNode.IDNodeMap.ContainsKey(con.From))
                    continue;
                if (GUISynthNode.IDNodeMap[con.From].IsGlobal == false)
					path.Stroke = Brushes.LightSkyBlue;
				else
					path.Stroke = Brushes.DeepPink;
				Canvas.SetZIndex(path, 0);
			}
			selectedPaths.Clear();
			// disable paste and save selection menu items
			MenuItem mi = _popup.Items[0] as MenuItem;
			mi.IsEnabled = false;
			mi = _popup.Items[1] as MenuItem;
			mi.IsEnabled = false;
		}

        public void AddToSelection(int id)
        {
            if (GUISynthNode.IDNodeMap.ContainsKey(id))
            { 
                GUISynthNode node = GUISynthNode.IDNodeMap[id];
                AddSelection(node);
            }
		}

		public void AddSelection(GUISynthNode node, bool selectPaths = true)
		{
			if (selectedNodes.Contains(node))
				return;
			selectedNodes.Add(node);
			node.IsSelected = true;
			Canvas.SetZIndex(node, 1100);
			// highlight incoming connections
			if (selectPaths == true)
			{
				foreach (NodeConnection con in pathConnectionMap.Values)
				{
					if (con.To == node.ID)
					{
						selectedPaths.Add(con.PathRef);
						BrushConverter bc = new BrushConverter();
						con.PathRef.Stroke = (Brush)bc.ConvertFrom("#FFFFCD50");
						Canvas.SetZIndex(con.PathRef, 1000);
					}
				}
			}
			// enable paste and save selection menu items
			MenuItem mi = _popup.Items[0] as MenuItem;
			mi.IsEnabled = true;
			mi = _popup.Items[1] as MenuItem;
			mi.IsEnabled = true;
		}

		void RemoveSelection(GUISynthNode node)
		{
			selectedNodes.Remove(node);
			node.IsSelected = false;
			Canvas.SetZIndex(node, 1);
			// restore highlighted paths
			List<NodeConnection> restore = new List<NodeConnection>();
			foreach (Path path in selectedPaths)
			{
				if (pathConnectionMap.ContainsKey(path))
				{
					NodeConnection con = pathConnectionMap[path];
					if (con.To == node.ID)
						restore.Add(con);
				}
			}
			foreach (NodeConnection con in restore)
			{
                if (!GUISynthNode.IDNodeMap.ContainsKey(con.From))
                    continue;

                if (GUISynthNode.IDNodeMap[con.From].IsGlobal == false)
					con.PathRef.Stroke = Brushes.LightSkyBlue;
				else
					con.PathRef.Stroke = Brushes.DeepPink;
				Canvas.SetZIndex(con.PathRef, 0);
				selectedPaths.Remove(con.PathRef);
			}
			// enable/disable paste and save selection menu item
			MenuItem mi = _popup.Items[0] as MenuItem;
			mi.IsEnabled = selectedNodes.Count > 0;
			mi = _popup.Items[1] as MenuItem;
			mi.IsEnabled = selectedNodes.Count > 0;
		}

		Dictionary<GUISynthNode, bool> _recursiveVisitFlagMap;
		void RecursiveAddSelection(GUISynthNode node, bool initRecursion = false)
		{
			if (initRecursion == true)
			{
				_recursiveVisitFlagMap = new Dictionary<GUISynthNode, bool>();
			}

			// stop recursion when encountering an already processed node
			if (_recursiveVisitFlagMap.Keys.Contains(node))
				return;

			AddSelection(node);
			_recursiveVisitFlagMap[node] = true;

			int numinputs = node.Info.NumInputs;
			if (node.Info.VariableInput)
				numinputs = node.NumVariableInputs;
			// traverse all input recursively
			for (int i = 0; i < numinputs; i++)
			{
				int inputid = node.GetInput(i);
				// not a default zero connection?
				if (inputid != -1)
				{
                    if (!GUISynthNode.IDNodeMap.ContainsKey(inputid))
                        continue;
                    GUISynthNode input = GUISynthNode.IDNodeMap[inputid];
					RecursiveAddSelection(input);
				}
			}
		}

		void node_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			_dragNode = sender as GUISynthNode;
			_nodeClickPos = e.GetPosition(_dragNode);
			_mousePos = e.GetPosition(this);

			// recursive selection
			if ((Keyboard.Modifiers & ModifierKeys.Shift) != 0)
			{
				// new selection?
				if ((Keyboard.Modifiers & ModifierKeys.Control) == 0)
					ClearSelection();

				RecursiveAddSelection(_dragNode, true);
			}
			else
			{
				if (!_dragNode.IsSelected)
				{
					// new selection?
					if ((Keyboard.Modifiers & ModifierKeys.Control) == 0)
						ClearSelection();

					AddSelection(_dragNode);
				}
				// remove from selection
				else if ((Keyboard.Modifiers & ModifierKeys.Control) != 0)
				{
					RemoveSelection(_dragNode);
				}
			}
			e.Handled = true;
		}

		public void SetInitialValue(int id, int index, double v1, double v2, int mode)
		{
			if (!GUISynthNode.IDNodeMap.Keys.Contains(id))
				return;
			GUISynthNode node = GUISynthNode.IDNodeMap[id];
			if (index == -1) // for constants
			{
				index = 0;
				node.NodeEditButton.Content = Math.Round(v1, 2).ToString() + " / " + Math.Round(v2, 2).ToString();
			}
			node.InitialValues[index].LeftValue = v1;
			node.InitialValues[index].RightValue = v2;
			node.InitialValues[index].ModeValue = mode;
		}

		public void UpdateConfigText(int id)
		{
			if (!GUISynthNode.IDNodeMap.Keys.Contains(id))
				return;
			GUISynthNode node = GUISynthNode.IDNodeMap[id];
			node.UpdateModeText();
		}

		public void AddConnection(int from, int to, int index, bool visible = true)
		{
			if (!GUISynthNode.IDNodeMap.Keys.Contains(from) || !GUISynthNode.IDNodeMap.Keys.Contains(to))
				return;

			GUISynthNode fromNode = GUISynthNode.IDNodeMap[from];
			GUISynthNode toNode = GUISynthNode.IDNodeMap[to];

			toNode.SetInput(from, index);

			if (!visible)
				return;

			double fromX = Canvas.GetLeft(fromNode) + fromNode.ActualWidth;
			double fromY = Canvas.GetTop(fromNode) + 14;

			double toX = Canvas.GetLeft(toNode);
			double toY = Canvas.GetTop(toNode) + 37 + index * 20;

			//double dx = 20;
			//if ((toX > fromX) && ((toX - fromX) < 40))
			//    dx = (toX - fromX) / 2;

			NodeConnection con = new NodeConnection(from, to, index);

			Path path = CreateConnectionPath();	
			if (fromNode.IsGlobal == false)
				path.Stroke = Brushes.LightSkyBlue;
			else
				path.Stroke = Brushes.DeepPink;

			con.PathRef = path;
			pathConnectionMap.Add(path, con);

			Point dummy = new Point();
			UpdateConnectionPath(path, dummy);

			path.MouseLeftButtonDown += new MouseButtonEventHandler(path_MouseLeftButtonDown);
			path.MouseRightButtonDown += new MouseButtonEventHandler(path_MouseRightButtonDown);
			this.Children.Add(path);
			Canvas.SetZIndex(path, 0);
		}

		Path CreateConnectionPath()
		{
			Path path = new Path();
			PathGeometry pathGeometry = new PathGeometry();

			PathFigure pathFigure = new PathFigure();
			pathFigure.StartPoint = new Point(0, 0);

			LineSegment lineSegment = new LineSegment();
			lineSegment.Point = new Point(0, 0);
			pathFigure.Segments.Add(lineSegment);

			lineSegment = new LineSegment();
			lineSegment.Point = new Point(0, 0);
			pathFigure.Segments.Add(lineSegment);

			lineSegment = new LineSegment();
			lineSegment.Point = new Point(0, 0);
			pathFigure.Segments.Add(lineSegment);

			pathGeometry.Figures.Add(pathFigure);

			path.StrokeThickness = 4.5;
			//path.StrokeStartLineCap = PenLineCap.Round;
			//path.StrokeEndLineCap = PenLineCap.Round;
			path.Data = pathGeometry;
			path.Cursor = Cursors.Arrow;

			return path;
		}
		
		void UpdateConnectionPath(Path path, Point currentPos)
		{
			if (!pathConnectionMap.ContainsKey(path))
				return;
			NodeConnection con = pathConnectionMap[path];
            if (!GUISynthNode.IDNodeMap.ContainsKey(con.From))
                return;           
            GUISynthNode fromNode = GUISynthNode.IDNodeMap[con.From];
			double fromX = Canvas.GetLeft(fromNode) + fromNode.ActualWidth;
			double fromY = Canvas.GetTop(fromNode) + 14;
			double toX, toY;
			// existing connection?
			if (con.To != -1)
			{
				if (!GUISynthNode.IDNodeMap.ContainsKey(con.To))
					return;
				GUISynthNode toNode = GUISynthNode.IDNodeMap[con.To];
				toX = Canvas.GetLeft(toNode);
				toY = Canvas.GetTop(toNode) + 37 + con.Index * 20;			
			}
			// pending connection
			else
			{
				toX = currentPos.X-10;
				toY = currentPos.Y;
			}

			double dx = 20;
			//if ((toX > fromX) && ((toX - fromX) < 40))
			//    dx = (toX - fromX) / 2;

			PathGeometry pathGeometry = path.Data as PathGeometry;
			PathFigure pathFigure = pathGeometry.Figures[0];
			LineSegment lineSegment;
			pathFigure.StartPoint = new Point(fromX, fromY);
			lineSegment = pathFigure.Segments[0] as LineSegment;
			lineSegment.Point = new Point(fromX + dx, fromY);
			lineSegment = pathFigure.Segments[1] as LineSegment;
			lineSegment.Point = new Point(toX - dx, toY);
			lineSegment = pathFigure.Segments[2] as LineSegment;
			lineSegment.Point = new Point(toX, toY);
		}

		// update node connection paths
		public void UpdateNodeConnectionPaths(GUISynthNode node)
		{
			List<Path> movePaths = new List<Path>();
			foreach (NodeConnection con in pathConnectionMap.Values)
			{
				if (con.From == node.ID || con.To == node.ID)
					movePaths.Add(con.PathRef);
			}
			// update affected connections
			Point dummy = new Point();
			foreach (Path path in movePaths)
			{
				UpdateConnectionPath(path, dummy);
			}
		}

		void ClearContextPath()
		{
			if ((contextPath != null) && pathConnectionMap.ContainsKey(contextPath))
			{
				NodeConnection con = pathConnectionMap[contextPath];
                if (!GUISynthNode.IDNodeMap.ContainsKey(con.From))
                    return;
                if (GUISynthNode.IDNodeMap[con.From].IsGlobal == false)
					contextPath.Stroke = Brushes.LightSkyBlue;
				else
					contextPath.Stroke = Brushes.DeepPink;
				Canvas.SetZIndex(contextPath, 0);				
			}
			contextPath = null;
		}

		void path_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
		{
			ClearContextPath();
			contextPath = sender as Path;
			contextPath.Stroke = Brushes.SpringGreen;
			Canvas.SetZIndex(contextPath, 1000);
			e.Handled = true;
		}

		void path_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
		{
			ClearContextPath();
			contextPath = sender as Path;
			contextPath.Stroke = Brushes.SpringGreen;
			Canvas.SetZIndex(contextPath, 1000);
		}

		public delegate void RemoveNodeHandler(int id);
		public RemoveNodeHandler removeNodeHandler;
		public void RemoveNode(GUISynthNode rnode)
		{
			// a non selected item is to be removed so clear the seleciton list so no others are removed
			if (!selectedNodes.Contains(rnode))
			{
				ClearSelection();
				AddSelection(rnode);
			}
			// clear pending connections
			ClearPendingConnection();

			// wait for core mutex
			mainWindow.QueryCoreProcessingMutex(true);

            try
            {
                // stop all voices to ensure deleted core nodes wont lead to crash for running voices
                mainWindow.KillVoices();

                List<NodeConnection> removeCons = null;
                GUISynthNode removeNode = null;
                GUISynthNode removeNodeInput0 = null;

                // if selection active, remove all selected nodes
                foreach (GUISynthNode node in selectedNodes)
                {
                    // dont ever remove SynthRoot, ChannelRoot, NoteController
                    if (node.Info.TypeID < 3)
                        continue;

                    removeNode = node;
                    // collect the relevant paths and connection objects
                    removeCons = new List<NodeConnection>();
                    foreach (NodeConnection con in pathConnectionMap.Values)
                    {
                        if (con.From == node.ID || con.To == node.ID)
                            removeCons.Add(con);
                        if (con.To == node.ID && con.Index == 0 && GUISynthNode.IDNodeMap.ContainsKey(con.From))
                            removeNodeInput0 = GUISynthNode.IDNodeMap[con.From];
                    }
                    // remove paths and connection objects
                    foreach (NodeConnection con in removeCons)
                    {
                        Path path = con.PathRef;
                        if (!pathConnectionMap.ContainsKey(path))
                            continue;
                        // special case for variable input, need to adjust a paths
                        if (GUISynthNode.IDNodeMap.ContainsKey(con.To))
                        {    
	                        GUISynthNode toNode = GUISynthNode.IDNodeMap[con.To];
	                        if (toNode.Info.VariableInput)
	                        {
	                            if (con.Index < toNode.NumVariableInputs - 1)
	                            {
	                                NodeConnection foundFlip = null;
	                                // find path for last input
	                                foreach (NodeConnection nc in pathConnectionMap.Values)
	                                {
	                                    // found the connection?
	                                    if ((nc.To == toNode.ID) && (nc.Index == toNode.NumVariableInputs - 1))
	                                    {
	                                        foundFlip = nc;
	                                        break;
	                                    }
	                                }
	                                if (foundFlip != null)
	                                {
	                                    Point dummy = new Point();
	                                    foundFlip.Index = con.Index;
	                                    pathConnectionMap.Remove(foundFlip.PathRef);
	                                    pathConnectionMap.Add(foundFlip.PathRef, foundFlip);
	                                    UpdateConnectionPath(foundFlip.PathRef, dummy);
	                                }
	                            }
	                        }
						}

                        // remove the path
                        this.Children.Remove(path);
                        // update the connected gui nodes status
                        if (pathConnectionMap.ContainsKey(path))
                        {
                            NodeConnection c = pathConnectionMap[path];
                            if (GUISynthNode.IDNodeMap.ContainsKey(c.To))
                                GUISynthNode.IDNodeMap[c.To].SetInput(-1, c.Index);
                        }
                        // remove the path connection object
                        pathConnectionMap.Remove(path);
                    }

                    if (node.IsEditOpen > 0)
                    {
                        node.EditWindow.KillTimer();
                        this.Children.Remove(node.EditWindow);
                        MainWindow.EditStackPanel.Children.Remove(node.EditWindow);
                    }
                    node.KillTimer();
                    this.Children.Remove(node);
                    GUISynthNode.IDNodeMap.Remove(node.ID);

                    // remove in core
                    if (removeNodeHandler != null)
                        removeNodeHandler(node.ID);

                    // special case if only one node is removed. 
                    // check if the signals to/from the deleted node can be reassigned automatically
                    if (selectedNodes.Count == 1 && removeNode.Info.AllowSignalInsertion == true && removeNodeInput0 != null)
                    {
                        foreach (NodeConnection con in removeCons)
                        {
                            if (con.From == removeNode.ID && GUISynthNode.IDNodeMap.ContainsKey(con.To))
                            {
                                GUISynthNode toNode = GUISynthNode.IDNodeMap[con.To];
                                ClearPendingConnection();
                                NewPendingConnection(removeNodeInput0);
                                int index = con.Index;
                                if (toNode.Info.VariableInput)
                                    index = -1;
                                InputClicked(toNode, index);
                            }
                        }
                        ClearPendingConnection();
                    }
                }

                ClearSelection();
            }
            catch (Exception ex)
            {

            }

			// release core mutex
			mainWindow.QueryCoreProcessingMutex(false);
		}

		void ClearPendingConnection()
		{
			if (pendingConnection != null)
			{
				this.Children.Remove(pendingConnection.PathRef);
				pathConnectionMap.Remove(pendingConnection.PathRef);
				pendingConnection = null;
			}
		}

		public bool HasPendingConnection()
		{
			return pendingConnection != null;
		}

		public void NewPendingConnection(GUISynthNode fromNode)
		{
			// always clear pending connection
			ClearPendingConnection();
					
			pendingConnection = new NodeConnection(fromNode.ID, -1, 0);
			Path path = CreateConnectionPath();
			path.IsHitTestVisible = false;
			path.Stroke = Brushes.Red;

			pendingConnection.PathRef = path;
			pathConnectionMap.Add(path, pendingConnection);

			this.Children.Add(path);
			Canvas.SetZIndex(path, 2000);
		}

		public void RaiseEditWindow(GUISynthNode toRaise)
		{
			if ((toRaise.IsEditOpen == 1) && (Canvas.GetZIndex(toRaise.EditWindow) > 1050))
				return;

			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				if (node.IsEditOpen == 1)
				{
					int zindex = Canvas.GetZIndex(node.EditWindow);
					if (zindex > 1050)
						Canvas.SetZIndex(node.EditWindow, 1050);
				}
			}
			if (toRaise.IsEditOpen == 1)
				Canvas.SetZIndex(toRaise.EditWindow, 1080);
		}

		public delegate void ConnectionInputHandler(int from, int to, int index, bool connect);
		public ConnectionInputHandler connectionInputHandler;
		public void InputClicked(GUISynthNode toNode, int index)
		{
            try
            {
                // remove on direct click on input (index >= 0)
                if (index >= 0)
                {
                    // always remove input if available
                    int inputNode = toNode.GetInput(index);
                    if (inputNode != 1)
                    {
                        NodeConnection found = null;
                        foreach (NodeConnection nc in pathConnectionMap.Values)
                        {
                            // found the connection?
                            if ((nc.To == toNode.ID) && (nc.Index == index))
                            {
                                found = nc;
                                break;
                            }
                        }
                        if (found != null)
                        {
                            // special case for variable input, need to adjust a path
                            if (toNode.Info.VariableInput)
                            {
                                if (index < toNode.NumVariableInputs - 1)
                                {
                                    NodeConnection foundFlip = null;
                                    // find path for last input
                                    foreach (NodeConnection nc in pathConnectionMap.Values)
                                    {
                                        // found the connection?
                                        if ((nc.To == toNode.ID) && (nc.Index == toNode.NumVariableInputs - 1))
                                        {
                                            foundFlip = nc;
                                            break;
                                        }
                                    }
                                    if (foundFlip != null)
                                    {
                                        Point dummy = new Point();
                                        foundFlip.Index = found.Index;
                                        pathConnectionMap.Remove(foundFlip.PathRef);
                                        pathConnectionMap.Add(foundFlip.PathRef, foundFlip);
                                        UpdateConnectionPath(foundFlip.PathRef, dummy);
                                    }
                                }
                            }

                            // remove path and connection in gui
                            this.Children.Remove(found.PathRef);
                            pathConnectionMap.Remove(found.PathRef);
                            toNode.SetInput(-1, index);

                            // disconnect in core
                            if (connectionInputHandler != null)
                                connectionInputHandler(inputNode, toNode.ID, index, false);
                        }
                    }
                }

                // active pending connection?
                if (pendingConnection != null)
                {
                    // special handling for variable inputs
                    if (toNode.Info.VariableInput)
                    {
                        if (toNode.NumVariableInputs < 16)
                        {
                            index = toNode.NumVariableInputs;
                        }
                        else
                        {
                            MessageBox.Show("No more than 16 inputs are allowed!", "64klang2");
                            return;
                        }
                    }

                    // no voice to global allowed, except voiceroot to voicemanager
                    GUISynthNode fromNode = GUISynthNode.IDNodeMap[pendingConnection.From];
                    if ((!fromNode.IsGlobal && toNode.IsGlobal) && !((fromNode.Info.TypeID == 4) && (toNode.Info.TypeID == 3)))
                    {
                        MessageBox.Show("A voice node output cannot be connected to a global node input!", "64klang2");
                        return;
                    }

                    // voiceroot can only be connected to voicemanager index 0
                    if ((fromNode.Info.TypeID == 4) && (toNode.Info.TypeID == 3) && (index != 0))
                    {
                        MessageBox.Show("Voice Root can only be attached to Voice Manager's Voice Root input!", "64klang2");
                        return;
                    }

                    // notecontroller must only have voicemanagers as input
                    if ((toNode.Info.TypeID == 2) && (fromNode.Info.TypeID != 3))
                    {
                        MessageBox.Show("Only Voice Manager can be input for Note Controller!", "64klang2");
                        return;
                    }

                    // recorder may only be iput for sample player or wtf oscillator or eventsignal
                    if (fromNode.Info.TypeID == 41)
                    {
                        if (!(((toNode.Info.TypeID == 42) && (index == 0)) || ((toNode.Info.TypeID == 56) && (index == 0)) || (toNode.Info.TypeID == 27)))
                        {
                            MessageBox.Show("Sample Recorder can only be input for Sample Player or WTF Oscillator In input!", "64klang2");
                            return;
                        }
                    }

                    // texttospeech may only be input for sample player or wtf oscillator
                    if (fromNode.Info.TypeID == 50)
                    {
                        if (!(((toNode.Info.TypeID == 42) && (index == 0)) || ((toNode.Info.TypeID == 56) && (index == 0))))
                        {
                            MessageBox.Show("TextToSpeech can only be input for Sample Player or WTF Oscillator In input!", "64klang2");
                            return;
                        }
                    }

                    // gm.dls may only be input for sampleplayer
                    if (fromNode.Info.TypeID == 52)
                    {
                        if (!(((toNode.Info.TypeID == 42) && (index == 0)) || ((toNode.Info.TypeID == 56) && (index == 0))))
                        {
                            MessageBox.Show("TextToSpeech can only be input for Sample Player or WTF Oscillator In input!", "64klang2");
                            return;
                        }
                    }

                    // sampleplayer only allows input at first slot from recorder or texttospeech or gml.dls
                    if ((toNode.Info.TypeID == 42) && (index == 0) && !((fromNode.Info.TypeID == 41) || (fromNode.Info.TypeID == 50) || (fromNode.Info.TypeID == 52)))
                    {
                        MessageBox.Show("Sample Player's In can only be Sample Recorder, TextToSpeech or GM.DLS!", "64klang2");
                        return;
                    }

                    // wtf oscillator only allows input at first slot from recorder or texttospeech or gm.dls
                    if ((toNode.Info.TypeID == 56) && (index == 0) && !((fromNode.Info.TypeID == 41) || (fromNode.Info.TypeID == 50) || (fromNode.Info.TypeID == 52)))
                    {
                        MessageBox.Show("WTF Oscillator's In can only be Sample Recorder, TextToSpeech or GM.DLS!", "64klang2");
                        return;
                    }

                    // voiceroot can only be connected to voicemanager index 0
                    if ((fromNode.Info.TypeID != 6) && (fromNode.Info.TypeID != 7) && (toNode.Info.TypeID == 59) && (index == 1))
                    {
                        MessageBox.Show("Only Oscillator or LFO can be input for SyncOsc!", "64klang2");
                        return;
                    }

                    // add new connection
                    AddConnection(pendingConnection.From, toNode.ID, index);

                    // connect in core
                    if (connectionInputHandler != null)
                        connectionInputHandler(pendingConnection.From, toNode.ID, index, true);
                }
            }
            catch (Exception ex)
            {

            }		
		}

		public delegate void UpdateNodeValuesHandler(int id);
		public UpdateNodeValuesHandler updateNodeValuesHandler;
		public void UpdateNodeValues(int id)
		{
			if (updateNodeValuesHandler != null)
				updateNodeValuesHandler(id);
		}

		public delegate void ValueChangedHandler(int id, int index, double value1, double value2);
		public ValueChangedHandler valueChangedHandler;
		public void NodeValueChanged(int id, int index, double value1, double value2)
		{
			if (valueChangedHandler != null && GUISynthNode.IDNodeMap.ContainsKey(id))
			{
				valueChangedHandler(id, index, value1, value2);
				GUISynthNode node = GUISynthNode.IDNodeMap[id];
				// update initialvalues
				if (index == -1) 
					index = 0; // for constants
				node.InitialValues[index].LeftValue = value1;
				node.InitialValues[index].RightValue = value2;
				node.UpdateModeText();
			}
		}

		public delegate void ModeChangedHandler(int id, int index, int mode, int modemask);
		public ModeChangedHandler modeChangedHandler;
		public void NodeModeChanged(int id, int index, int mode, int modemask = -1)
		{
			if (modeChangedHandler != null && GUISynthNode.IDNodeMap.ContainsKey(id))
			{
				modeChangedHandler(id, index, mode, modemask);
				GUISynthNode node = GUISynthNode.IDNodeMap[id];
				// update initialvalues
				int oldmode = node.InitialValues[index].ModeValue & ~modemask;
				node.InitialValues[index].ModeValue = (mode & modemask) | oldmode;
				node.UpdateModeText();
			}
		}

		public delegate void NodeResetEventSignaldHandler(int id);
		public NodeResetEventSignaldHandler nodeResetEventSignalHandler;
		public void NodeResetEventSignal(int id)
		{
			if (nodeResetEventSignalHandler != null)
				nodeResetEventSignalHandler(id);
		}

		public delegate int GetArpStepDataHandler(int id, int step);
		public GetArpStepDataHandler getArpStepDataHandler;
		public int GetArpStepData(int id, int step)
		{
			if (getArpStepDataHandler != null)
				return getArpStepDataHandler(id, step);
			else
				return 0;
		}

		public delegate int GetArpPlayPosHandler(int id);
		public GetArpPlayPosHandler getArpPlayPosHandler;
		public int GetArpPlayPos(int id)
		{
			if (getArpPlayPosHandler != null)
				return getArpPlayPosHandler(id);
			else
				return 0;
		}

		public delegate void SetArpStepDataHandler(int id, int step, int value);
		public SetArpStepDataHandler setArpStepDataHandler;
		public void SetArpStepData(int id, int step, int value)
		{
			if (setArpStepDataHandler != null)
				setArpStepDataHandler(id, step, value);
		}

		public delegate double GetNodeSignalHandler(int id, int left, int input);
		public GetNodeSignalHandler getNodeSignalHandler;
		public double GetNodeSignal(int id, int left, int input)
		{
			if (getNodeSignalHandler != null)
				return getNodeSignalHandler(id, left, input);
			else
				return 0;
		}

		public delegate double GetNodeValueHandler(int id, int index, int channel);
		public GetNodeValueHandler getNodeValueHandler;
		public double GetNodeValue(int id, int index, int channel)
		{
			if (getNodeValueHandler != null)
				return getNodeValueHandler(id, index, channel);
			else
				return 0;
		}

		public delegate void SetSAPITextHandler(int id, string text);
		public SetSAPITextHandler setSAPITextHandler;
		public void SetSAPIText(int id, string text)
		{
            MainWindow.QueryCoreProcessingMutex(true);
			if (setSAPITextHandler != null)
				setSAPITextHandler(id, text);
            MainWindow.QueryCoreProcessingMutex(false);
		}

		public delegate string GetSAPITextHandler(int id);
		public GetSAPITextHandler getSAPITextHandler;
		public string GetSAPIText(int id)
		{
			if (getSAPITextHandler != null)
				return getSAPITextHandler(id);
			else
				return "";
		}

        public delegate void SetFormulaTextHandler(int id, string text, string rpn);
        public SetFormulaTextHandler setFormulaTextHandler;
        public void SetFormulaText(int id, string text, string rpn)
        {
            MainWindow.QueryCoreProcessingMutex(true);
            if (setFormulaTextHandler != null)
                setFormulaTextHandler(id, text, rpn);
            MainWindow.QueryCoreProcessingMutex(false);
        }

        public delegate string GetFormulaTextHandler(int id);
        public GetFormulaTextHandler getFormulaTextHandler;
        public string GetFormulaText(int id)
        {
            if (getFormulaTextHandler != null)
                return getFormulaTextHandler(id);
            else
                return "";
        }

		public delegate int GetNumActiveVoicesHandler(int id);
		public GetNumActiveVoicesHandler getNumActiveVoicesHandler;
		public int GetNumActiveVoices(int id)
		{
			if (getNumActiveVoicesHandler != null)
				return getNumActiveVoicesHandler(id);
			else
				return 0;
		}

		public void HighlightSearchNodes(string text)
		{
			//if (text.Length > 2)
			ClearSelection();
			foreach (GUISynthNode node in GUISynthNode.IDNodeMap.Values)
			{
				if (node.NodeName.Content.ToString().ToLower().Contains(text.ToLower()))
				{
					AddSelection(node, false);
				}
			}
		}
	}
}
