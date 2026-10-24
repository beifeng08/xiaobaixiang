// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel.Design;

namespace Aga.Controls
{
	public class StringCollectionEditor : CollectionEditor
	{
		public StringCollectionEditor(Type type): base(type)
		{
		}

		protected override Type CreateCollectionItemType()
		{
			return typeof(string);
		}

		protected override object CreateInstance(Type itemType)
		{
			return "";
		}
	}
}
