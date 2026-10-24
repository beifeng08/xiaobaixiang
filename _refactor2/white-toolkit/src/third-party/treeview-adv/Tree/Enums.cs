// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;

namespace Aga.Controls.Tree
{
	public enum DrawSelectionMode
	{
		None, Active, Inactive, FullRowSelect
	}

	public enum TreeSelectionMode
	{
		Single, Multi, MultiSameParent
	}

	public enum NodePosition
	{
		Inside, Before, After
	}

	public enum VerticalAlignment
	{
		Top, Bottom, Center
	}

	public enum IncrementalSearchMode
	{
		None, Standard, Continuous
	}

	[Flags]
    public enum GridLineStyle
    {
		None = 0, 
		Horizontal = 1, 
		Vertical = 2, 
		HorizontalAndVertical = 3
    }

	public enum ImageScaleMode
	{
		
		Clip,
		
		Fit,
		
		ScaleDown,
		
		ScaleUp,
		
		AlwaysScale,

	}
}
