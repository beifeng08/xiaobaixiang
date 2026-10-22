// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls.Tree
{
	public struct DropPosition
	{
		private TreeNodeAdv _node;
		public TreeNodeAdv Node
		{
			get { return _node; }
			set { _node = value; }
		}

		private NodePosition _position;
		public NodePosition Position
		{
			get { return _position; }
			set { _position = value; }
		}
	}
}
