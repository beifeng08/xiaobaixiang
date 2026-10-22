// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// Copyright (C) LibreHardwareMonitor and Contributors.
// All Rights Reserved.

namespace LibreHardwareMonitor.Hardware;

internal interface IHardwareChanged
{
    event HardwareEventHandler HardwareAdded;
    event HardwareEventHandler HardwareRemoved;
}