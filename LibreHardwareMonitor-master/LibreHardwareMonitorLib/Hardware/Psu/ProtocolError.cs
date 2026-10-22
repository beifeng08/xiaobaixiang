// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System;
using HidSharp;

namespace LibreHardwareMonitor.Hardware.Psu;

public class ProtocolError(HidDevice device, string message) : ApplicationException($"Error communicating with the PSU controller at {device.DevicePath}: {message}");
