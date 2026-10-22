// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

namespace YanJi.Services;

internal sealed class AutoReportState
{
    bool _hasExported;
    int _currentGeneration;
    int? _pendingGeneration;

    public int? Request(bool allDone)
    {
        if (!allDone)
        {
            _hasExported = false;
            _pendingGeneration = null;
            return null;
        }
        if (_hasExported || _pendingGeneration.HasValue) return null;
        _pendingGeneration = ++_currentGeneration;
        return _pendingGeneration;
    }

    public bool IsPending(int generation) => _pendingGeneration == generation;

    public void Finish(int generation, bool succeeded)
    {
        if (!IsPending(generation)) return;
        _pendingGeneration = null;
        _hasExported = succeeded;
    }
}
