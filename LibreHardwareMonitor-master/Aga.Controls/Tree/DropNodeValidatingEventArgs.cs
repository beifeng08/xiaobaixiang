// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Drawing;

namespace Aga.Controls.Tree
{
	public class DropNodeValidatingEventArgs: EventArgs
	{
		Point _point;
		TreeNodeAdv _node;

		public DropNodeValidatingEventArgs(Point point, TreeNodeAdv node)
		{
			_point = point;
			_node = node;
		}

		public Point Point
		{
			get { return _point; }
		}

		public TreeNodeAdv Node
		{
			get { return _node; }
			set { _node = value; }
		}
	}
}
