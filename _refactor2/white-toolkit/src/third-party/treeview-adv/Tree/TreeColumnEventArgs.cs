// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls.Tree
{
	public class TreeColumnEventArgs: EventArgs
	{
		private TreeColumn _column;
		public TreeColumn Column
		{
			get { return _column; }
		}

		public TreeColumnEventArgs(TreeColumn column)
		{
			_column = column;
		}
	}
}
