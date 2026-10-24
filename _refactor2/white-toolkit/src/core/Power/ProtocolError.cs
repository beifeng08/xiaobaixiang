// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

using System;
using HidSharp;

using SysCore.Contracts;
using SysCore.Model;
using SysCore.Platform;
namespace SysCore.Power;

public class ProtocolError(HidDevice device, string message) : ApplicationException($"Error communicating with the PSU controller at {device.DevicePath}: {message}");
