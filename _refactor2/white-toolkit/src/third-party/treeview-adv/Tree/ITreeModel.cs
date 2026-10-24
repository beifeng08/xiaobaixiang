// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;
using System.Collections;

namespace Aga.Controls.Tree
{
	public interface ITreeModel
	{
		IEnumerable GetChildren(TreePath treePath);
		bool IsLeaf(TreePath treePath);

		event EventHandler<TreeModelEventArgs> NodesChanged; 
		event EventHandler<TreeModelEventArgs> NodesInserted;
		event EventHandler<TreeModelEventArgs> NodesRemoved; 
		event EventHandler<TreePathEventArgs> StructureChanged;
	}
}
