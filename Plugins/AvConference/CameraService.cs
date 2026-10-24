// Copyright (c) 2026 OmniSensor

using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Enumeration;
using Windows.Graphics.Imaging;
using Windows.Media.Capture;
using Windows.Media.Capture.Frames;
using Windows.Media.MediaProperties;
using Windows.Storage.Streams;

namespace YanJi.Plugin.AvConference;

public class CameraService : IDisposable
{
    readonly SemaphoreSlim _lifecycleGate = new(1, 1);
    readonly SemaphoreSlim _frameGate = new(1, 1);
    MediaCapture? _capture;
    MediaFrameReader? _reader;
    MediaCaptureFailedEventHandler? _captureFailedHandler;
    CancellationToken _frameCancellationToken;
    int _generation;
    int _activeGeneration;
    TaskCompletionSource<bool>? _firstFrame;
    string? _frameDiagnostic;

    public event Action<int, int, byte[]>? FrameReady;
    public event Action<string>? Failed;

    public static async Task<List<(string Id, string Name)>> GetDevicesAsync()
    {
        var infos = await DeviceInformation.FindAllAsync(DeviceClass.VideoCapture);
        return infos.Select(i => (i.Id, i.Name)).ToList();
    }

    public Task StartAsync(string deviceId) => StartAsync(deviceId, CancellationToken.None);

    public async Task StartAsync(string deviceId, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        int generation = Interlocked.Increment(ref _generation);
        await _lifecycleGate.WaitAsync(cancellationToken);
        MediaCapture? capture = null;
        MediaFrameReader? reader = null;
        string? captureError = null;
        MediaCaptureFailedEventHandler failedHandler = (sender, args) =>
        {
            Volatile.Write(ref captureError, $"{args.Message}（错误码 0x{args.Code:X8}）");
            OnCaptureFailed(sender, args);
        };
        try
        {
            CheckRequest(generation, cancellationToken);
            await StopCoreAsync();
            CheckRequest(generation, cancellationToken);
            capture = new MediaCapture();
            capture.Failed += failedHandler;
            await capture.InitializeAsync(new MediaCaptureInitializationSettings
            {
                VideoDeviceId = deviceId,
                StreamingCaptureMode = StreamingCaptureMode.Video,
                
                MemoryPreference = MediaCaptureMemoryPreference.Cpu,
            });
            CheckRequest(generation, cancellationToken);
            if (Volatile.Read(ref captureError) is { } initializeError)
                throw new InvalidOperationException("摄像头采集失败：" + initializeError);
            var source = capture.FrameSources.Values.FirstOrDefault(s =>
                s.Info.MediaStreamType == MediaStreamType.VideoPreview)
                ?? capture.FrameSources.Values.FirstOrDefault();
            if (source == null)
                throw new InvalidOperationException("该摄像头没有可用的预览帧源");
            reader = await capture.CreateFrameReaderAsync(source, MediaEncodingSubtypes.Bgra8);
            CheckRequest(generation, cancellationToken);
            reader.FrameArrived += OnFrame;
            var status = await reader.StartAsync();
            CheckRequest(generation, cancellationToken);
            if (status == MediaFrameReaderStartStatus.OutputFormatNotSupported)
            {
                
                var unsupported = reader;
                reader = null;
                await ReleaseAsync(unsupported, null, null);
                CheckRequest(generation, cancellationToken);
                reader = await capture.CreateFrameReaderAsync(source);
                CheckRequest(generation, cancellationToken);
                reader.FrameArrived += OnFrame;
                status = await reader.StartAsync();
                CheckRequest(generation, cancellationToken);
            }
            if (status != MediaFrameReaderStartStatus.Success)
                throw new InvalidOperationException("摄像头预览启动失败：" + status);
            var firstFrame = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            Volatile.Write(ref _frameDiagnostic, null);
            _firstFrame = firstFrame;
            _capture = capture;
            _reader = reader;
            _captureFailedHandler = failedHandler;
            _frameCancellationToken = cancellationToken;
            Volatile.Write(ref _activeGeneration, generation);
            capture = null;
            reader = null;
            try
            {
                if (Volatile.Read(ref captureError) is { } startError)
                    throw new InvalidOperationException("摄像头采集失败：" + startError);
                
                try { await firstFrame.Task.WaitAsync(TimeSpan.FromSeconds(6), cancellationToken); }
                catch (TimeoutException)
                {
                    throw new InvalidOperationException("摄像头启动后6秒未收到可用画面：" +
                        (Volatile.Read(ref _frameDiagnostic) ?? "设备未交付视频帧，请检查设备占用、隐私权限或驱动"));
                }
                CheckRequest(generation, cancellationToken);
            }
            catch
            {
                await StopCoreAsync();
                throw;
            }
        }
        finally
        {
            try { await ReleaseAsync(reader, capture, failedHandler); }
            finally { _lifecycleGate.Release(); }
        }
    }

    void CheckRequest(int generation, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (generation != Volatile.Read(ref _generation))
            throw new OperationCanceledException("摄像头请求已失效");
    }

    void OnCaptureFailed(MediaCapture sender, MediaCaptureFailedEventArgs args)
    {
        var failed = Failed;
        if (ReferenceEquals(sender, _capture) &&
            Volatile.Read(ref _activeGeneration) == Volatile.Read(ref _generation))
        {
            string error = $"{args.Message}（错误码 0x{args.Code:X8}）";
            _firstFrame?.TrySetException(new InvalidOperationException("摄像头采集失败：" + error));
            failed?.Invoke(error);
        }
    }

    bool IsCurrentFrame(MediaFrameReader reader, int generation) => generation != 0 &&
        generation == Volatile.Read(ref _activeGeneration) &&
        generation == Volatile.Read(ref _generation) && ReferenceEquals(reader, _reader) &&
        !_frameCancellationToken.IsCancellationRequested;

    async void OnFrame(MediaFrameReader sender, MediaFrameArrivedEventArgs args)
    {
        var frameReady = FrameReady;
        int generation = Volatile.Read(ref _activeGeneration);
        if (!IsCurrentFrame(sender, generation) || !_frameGate.Wait(0)) return;
        try
        {
            
            if (!IsCurrentFrame(sender, generation)) return;
            using var frame = sender.TryAcquireLatestFrame();
            var videoFrame = frame?.VideoMediaFrame;
            using var sb = videoFrame?.SoftwareBitmap;
            using var surface = sb == null ? videoFrame?.Direct3DSurface : null;
            using var readback = surface != null
                ? await SoftwareBitmap.CreateCopyFromSurfaceAsync(surface) : null;
            
            if (!IsCurrentFrame(sender, generation)) return;
            var bitmap = sb ?? readback;
            if (bitmap == null)
            {
                Volatile.Write(ref _frameDiagnostic, "视频帧不包含可读取的CPU位图或GPU表面");
                return;
            }
            using var converted = bitmap.BitmapPixelFormat != BitmapPixelFormat.Bgra8 ||
                bitmap.BitmapAlphaMode != BitmapAlphaMode.Premultiplied
                ? SoftwareBitmap.Convert(bitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied) : null;
            var bmp = converted ?? bitmap;
            int w = bmp.PixelWidth, h = bmp.PixelHeight;
            var buf = new Windows.Storage.Streams.Buffer((uint)(w * h * 4));
            bmp.CopyToBuffer(buf);
            if (IsCurrentFrame(sender, generation))
            {
                frameReady?.Invoke(w, h, buf.ToArray());
                _firstFrame?.TrySetResult(true);
            }
        }
        catch (Exception ex)
        {
            if (IsCurrentFrame(sender, generation))
                Volatile.Write(ref _frameDiagnostic, "视频帧转换失败：" + ex.Message);
        }
        finally { _frameGate.Release(); }
    }

    public async Task StopAsync()
    {
        int generation = Interlocked.Increment(ref _generation);
        await _lifecycleGate.WaitAsync();
        try
        {
            
            if (Volatile.Read(ref _activeGeneration) <= generation) await StopCoreAsync();
        }
        finally { _lifecycleGate.Release(); }
    }

    async Task StopCoreAsync()
    {
        Volatile.Write(ref _activeGeneration, 0);
        _firstFrame = null;
        var reader = _reader; _reader = null;
        var capture = _capture; _capture = null;
        var failedHandler = _captureFailedHandler; _captureFailedHandler = null;
        await ReleaseAsync(reader, capture, failedHandler);
    }

    async Task ReleaseAsync(MediaFrameReader? reader, MediaCapture? capture, MediaCaptureFailedEventHandler? failedHandler)
    {
        try
        {
            if (reader != null)
            {
                await _frameGate.WaitAsync();
                try
                {
                    try
                    {
                        reader.FrameArrived -= OnFrame;
                        await reader.StopAsync();
                    }
                    catch {  }
                    finally { reader.Dispose(); }
                }
                finally { _frameGate.Release(); }
            }
        }
        finally
        {
            if (capture != null)
            {
                try { if (failedHandler != null) capture.Failed -= failedHandler; }
                finally { capture.Dispose(); }
            }
        }
    }

    public void Dispose() => _ = StopAsync();
}
