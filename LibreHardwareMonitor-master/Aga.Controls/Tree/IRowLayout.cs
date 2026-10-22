// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace Aga.Controls.Tree
{
	internal interface IRowLayout
	{
		int PreferredRowHeight
		{
			get;
			set;
		}

		int PageRowCount
		{
			get;
		}

		int CurrentPageSize
		{
			get;
		}

		Rectangle GetRowBounds(int rowNo);

		int GetRowAt(Point point);

		int GetFirstRow(int lastPageRow);

		void ClearCache();
	}
}
