// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls.Tree
{
	public class TreePathEventArgs : EventArgs
	{
		private TreePath _path;
		public TreePath Path
		{
			get { return _path; }
		}

		public TreePathEventArgs()
		{
			_path = new TreePath();
		}

		public TreePathEventArgs(TreePath path)
		{
			if (path == null)
				throw new ArgumentNullException();

			_path = path;
		}
	}
}
