// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls.Tree
{
	public class TreeViewAdvCancelEventArgs : TreeViewAdvEventArgs
	{
		private bool _cancel;

		public bool Cancel
		{
			get { return _cancel; }
			set { _cancel = value; }
		}

		public TreeViewAdvCancelEventArgs(TreeNodeAdv node)
			: base(node)
		{
		}

	}
}
