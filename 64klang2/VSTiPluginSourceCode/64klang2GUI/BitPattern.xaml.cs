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
	/// Interaction logic for BitPattern.xaml
	/// </summary>
	public partial class BitPattern : UserControl
	{
		int index;
		int pindex;
		bool syncedEdit;

		public BitPattern()
		{
			InitializeComponent();

			pindex = 0;
			index = 0;
			syncedEdit = false;
		}

		public void Init(int paramindex, int patternindex, uint model, uint moder)
		{
			index = paramindex;
			pindex = patternindex;

			syncedEdit = true;

			model = model >> patternindex * 8;
			if ((model & 1) != 0) this.L0.IsChecked = true;
			if ((model & 2) != 0) this.L1.IsChecked = true;
			if ((model & 4) != 0) this.L2.IsChecked = true;
			if ((model & 8) != 0) this.L3.IsChecked = true;
			if ((model & 16) != 0) this.L4.IsChecked = true;
			if ((model & 32) != 0) this.L5.IsChecked = true;
			if ((model & 64) != 0) this.L6.IsChecked = true;
			if ((model & 128) != 0) this.L7.IsChecked = true;

			moder = moder >> patternindex * 8;
			if ((moder & 1) != 0) this.R0.IsChecked = true;
			if ((moder & 2) != 0) this.R1.IsChecked = true;
			if ((moder & 4) != 0) this.R2.IsChecked = true;
			if ((moder & 8) != 0) this.R3.IsChecked = true;
			if ((moder & 16) != 0) this.R4.IsChecked = true;
			if ((moder & 32) != 0) this.R5.IsChecked = true;
			if ((moder & 64) != 0) this.R6.IsChecked = true;
			if ((moder & 128) != 0) this.R7.IsChecked = true;

			if (model != moder)
				this.ValueSync.IsChecked = false;

			syncedEdit = false;
		}
				
		private void L_Checked(object sender, RoutedEventArgs e)
		{
			CheckUncheckL();
		}

		private void L_Unchecked(object sender, RoutedEventArgs e)
		{
			CheckUncheckL();
		}

		private void R_Checked(object sender, RoutedEventArgs e)
		{
			CheckUncheckR();
		}

		private void R_Unchecked(object sender, RoutedEventArgs e)
		{
			CheckUncheckR();
		}

		private void CheckUncheckL()
		{
			if (!this.IsInitialized || syncedEdit == true)
				return;
			if (this.ValueSync.IsChecked == true)
			{
				syncedEdit = true;
				this.R0.IsChecked = this.L0.IsChecked;
				this.R1.IsChecked = this.L1.IsChecked;
				this.R2.IsChecked = this.L2.IsChecked;
				this.R3.IsChecked = this.L3.IsChecked;
				this.R4.IsChecked = this.L4.IsChecked;
				this.R5.IsChecked = this.L5.IsChecked;
				this.R6.IsChecked = this.L6.IsChecked;
				this.R7.IsChecked = this.L7.IsChecked;
				syncedEdit = false;
			}
			UpdateValues();
		}

		private void CheckUncheckR()
		{
			if (!this.IsInitialized || syncedEdit == true)
				return;
			if (this.ValueSync.IsChecked == true)
			{
				syncedEdit = true;
				this.L0.IsChecked = this.R0.IsChecked;
				this.L1.IsChecked = this.R1.IsChecked;
				this.L2.IsChecked = this.R2.IsChecked;
				this.L3.IsChecked = this.R3.IsChecked;
				this.L4.IsChecked = this.R4.IsChecked;
				this.L5.IsChecked = this.R5.IsChecked;
				this.L6.IsChecked = this.R6.IsChecked;
				this.L7.IsChecked = this.R7.IsChecked;
				syncedEdit = false;
			}
			UpdateValues();
		}

		public delegate void ValueChangedHandler(int index, int patternindex, uint model, uint moder);
		public ValueChangedHandler valueChangedHandler;
		private void UpdateValues()
		{
			uint valueL = 0;
			if (this.L0.IsChecked == true) valueL |= 1;
			if (this.L1.IsChecked == true) valueL |= 2;
			if (this.L2.IsChecked == true) valueL |= 4;
			if (this.L3.IsChecked == true) valueL |= 8;
			if (this.L4.IsChecked == true) valueL |= 16;
			if (this.L5.IsChecked == true) valueL |= 32;
			if (this.L6.IsChecked == true) valueL |= 64;
			if (this.L7.IsChecked == true) valueL |= 128;
			uint valueR = 0;
			if (this.R0.IsChecked == true) valueR |= 1;
			if (this.R1.IsChecked == true) valueR |= 2;
			if (this.R2.IsChecked == true) valueR |= 4;
			if (this.R3.IsChecked == true) valueR |= 8;
			if (this.R4.IsChecked == true) valueR |= 16;
			if (this.R5.IsChecked == true) valueR |= 32;
			if (this.R6.IsChecked == true) valueR |= 64;
			if (this.R7.IsChecked == true) valueR |= 128;

			if (valueChangedHandler != null)
				valueChangedHandler(index, pindex, valueL, valueR);
		}
	}
}
