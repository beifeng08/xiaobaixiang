// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace SysCore.Windows.Forms.UI;

internal class NodeToolTipProvider : IToolTipProvider
{
    public string GetToolTip(TreeNodeAdv node, NodeControl nodeControl) => (node.Tag as Node)?.ToolTip;
}