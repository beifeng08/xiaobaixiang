namespace YanJi.Services;

internal sealed class AutoReportState
{
    bool _exported;
    int _generation;
    int? _pending;

    public int? Request(bool allDone)
    {
        if (!allDone)
        {
            _exported = false;
            _pending = null;
            return null;
        }
        if (_exported || _pending.HasValue) return null;
        _pending = ++_generation;
        return _pending;
    }

    public bool IsPending(int generation) => _pending == generation;

    public void Finish(int generation, bool succeeded)
    {
        if (!IsPending(generation)) return;
        _pending = null;
        _exported = succeeded;
    }
}
