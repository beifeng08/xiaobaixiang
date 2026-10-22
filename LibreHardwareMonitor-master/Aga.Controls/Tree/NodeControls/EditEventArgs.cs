// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

﻿using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Aga.Controls.Tree.NodeControls
{
	public class EditEventArgs : NodeEventArgs
	{
		private Control _control;
		public Control Control
		{
			get { return _control; }
		}

		public EditEventArgs(TreeNodeAdv node, Control control)
			: base(node)
		{
			_control = control;
		}
	}
}
