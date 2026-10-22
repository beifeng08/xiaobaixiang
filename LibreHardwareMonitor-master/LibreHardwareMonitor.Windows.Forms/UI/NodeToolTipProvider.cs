// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace LibreHardwareMonitor.Windows.Forms.UI;

internal class NodeToolTipProvider : IToolTipProvider
{
    public string GetToolTip(TreeNodeAdv node, NodeControl nodeControl) => (node.Tag as Node)?.ToolTip;
}