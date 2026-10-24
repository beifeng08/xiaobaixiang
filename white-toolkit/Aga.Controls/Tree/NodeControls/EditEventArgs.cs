// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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
