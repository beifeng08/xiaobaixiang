// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using NAudio.CoreAudioApi;
using NAudio.Wave;

namespace YanJi.Plugin.AvConference;

public class AudioService : IDisposable
{
    WasapiCapture? _capture;
    WaveFileWriter? _writer;
    WasapiOut? _output;
    MediaFoundationReader? _reader;
    readonly object _recordGate = new();
    readonly object _playGate = new();
    MMDevice? _inputDevice, _outputDevice;
    float _peak;
    bool _recording;

    public float Peak => _peak;

    public bool IsPlaying => _output != null;

    public static List<(string Id, string Name)> GetInputDevices() => GetDevices(DataFlow.Capture);
    public static List<(string Id, string Name)> GetOutputDevices() => GetDevices(DataFlow.Render);

    static List<(string Id, string Name)> GetDevices(DataFlow flow)
    {
        try
        {
            using var en = new MMDeviceEnumerator();
            var result = new List<(string Id, string Name)>();
            foreach (var device in en.EnumerateAudioEndPoints(flow, DeviceState.Active))
                using (device) result.Add((device.ID, device.FriendlyName));
            return result;
        }
        catch { return []; }
    }

    public void StartInput(string deviceId)
    {
        StopInput();
        using var en = new MMDeviceEnumerator();
        var device = en.GetDevice(deviceId);
        WasapiCapture? cap = null;
        try
        {
            cap = new WasapiCapture(device);
            cap.DataAvailable += OnData;
            lock (_recordGate) { _capture = cap; _inputDevice = device; }
            cap.StartRecording();
        }
        catch
        {
            lock (_recordGate) { _capture = null; _inputDevice = null; }
            if (cap != null) { cap.DataAvailable -= OnData; cap.Dispose(); }
            device.Dispose();
            throw;
        }
    }

    void OnData(object? sender, WaveInEventArgs e)
    {
        lock (_recordGate)
        {
            if (_capture == null || !ReferenceEquals(sender, _capture) || e.BytesRecorded <= 0) return;
            _peak = ComputePeak(e.Buffer, e.BytesRecorded, _capture.WaveFormat);
            if (_recording)
            {
                try { _writer?.Write(e.Buffer, 0, e.BytesRecorded); }
                catch (Exception ex) { System.Diagnostics.Trace.WriteLine("录音写入失败：" + ex.Message); }
            }
        }
    }

    static float ComputePeak(byte[] buf, int len, WaveFormat wf)
    {
        float peak = 0;
        if (wf.Encoding == WaveFormatEncoding.IeeeFloat)
        {
            for (int i = 0; i + 4 <= len; i += 4)
                peak = Math.Max(peak, Math.Abs(BitConverter.ToSingle(buf, i)));
        }
        else 
        {
            for (int i = 0; i + 2 <= len; i += 2)
                peak = Math.Max(peak, Math.Abs(BitConverter.ToInt16(buf, i) / 32768f));
        }
        return Math.Min(1, peak);
    }

    public void StartRecord(string wavPath)
    {
        lock (_recordGate)
        {
            if (_capture == null) throw new InvalidOperationException("麦克风监听未启动");
            _recording = false;
            var previous = _writer; _writer = null;
            previous?.Dispose();
            _writer = new WaveFileWriter(wavPath, _capture.WaveFormat);
            _recording = true;
        }
    }

    public void StopRecord()
    {
        lock (_recordGate)
        {
            _recording = false;
            var writer = _writer; _writer = null;
            writer?.Dispose();
        }
    }

    public void StopInput()
    {
        WasapiCapture? cap;
        MMDevice? device;
        WaveFileWriter? writer;
        lock (_recordGate)
        {
            _recording = false;
            cap = _capture; _capture = null;
            device = _inputDevice; _inputDevice = null;
            writer = _writer; _writer = null;
            _peak = 0;
        }
        try { writer?.Dispose(); }
        finally
        {
            try
            {
                if (cap != null)
                {
                    cap.DataAvailable -= OnData;
                    try { cap.StopRecording(); } catch {  }
                    finally { cap.Dispose(); }
                }
            }
            finally { device?.Dispose(); }
        }
    }

    public void Play(string filePath, string? deviceId, Action onStopped)
    {
        StopPlay();
        MediaFoundationReader? reader = null;
        WasapiOut? output = null;
        MMDevice? device = null;
        bool published = false;
        try
        {
            reader = new MediaFoundationReader(filePath);
            if (deviceId != null)
            {
                using var en = new MMDeviceEnumerator();
                device = en.GetDevice(deviceId);
            }
            output = device != null
                ? new WasapiOut(device, AudioClientShareMode.Shared, false, 150) : new WasapiOut();
            var owner = output;
            output.PlaybackStopped += (_, _) =>
            {
                
                if (StopPlayback(owner)) onStopped();
            };
            output.Init(reader);
            lock (_playGate) { _reader = reader; _output = output; _outputDevice = device; }
            published = true;
            output.Play();
        }
        catch
        {
            if (published) StopPlayback(output);
            else ReleasePlayback(output, reader, device);
            throw;
        }
    }

    public void StopPlay() => StopPlayback(null);

    bool StopPlayback(WasapiOut? expected)
    {
        WasapiOut? output;
        MediaFoundationReader? reader;
        MMDevice? device;
        lock (_playGate)
        {
            if (expected != null && !ReferenceEquals(expected, _output)) return false;
            output = _output; _output = null;
            reader = _reader; _reader = null;
            device = _outputDevice; _outputDevice = null;
        }
        
        ReleasePlayback(output, reader, device);
        return output != null;
    }

    static void ReleasePlayback(WasapiOut? output, MediaFoundationReader? reader, MMDevice? device)
    {
        try
        {
            if (output != null)
            {
                try { output.Stop(); } catch {  }
                finally { output.Dispose(); }
            }
        }
        finally
        {
            try { reader?.Dispose(); }
            finally { device?.Dispose(); }
        }
    }

    public void Dispose()
    {
        StopInput();
        StopPlay();
    }
}
