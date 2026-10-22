// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using YanJi.PluginSdk;

namespace YanJi.Plugin.DiskMark;

public partial class DiskMarkPage : UserControl, IModulePage, IReportSection
{
    public event Action<bool>? PassChanged;

    readonly CdmService _cdm = new();
    CdmService.CdmResult? _result;
    string? _resultDrive;
    CancellationTokenSource? _cts;   
    bool _passed;

    public DiskMarkPage()
    {
        InitializeComponent();
        
        _cdm.Logged += line => Dispatcher.BeginInvoke(() => Log(line));
        
        string systemRoot = Path.GetPathRoot(Environment.SystemDirectory) ?? "C:\\";
        foreach (var d in DriveInfo.GetDrives())
        {
            if (d.DriveType != DriveType.Fixed || !d.IsReady) continue;
            DriveBox.Items.Add(new ComboBoxItem { Content = d.Name.TrimEnd('\\'), Tag = d.Name });
        }
        for (int i = 0; i < DriveBox.Items.Count; i++)
            if (((ComboBoxItem)DriveBox.Items[i]).Tag is string tag &&
                tag.Equals(systemRoot, StringComparison.OrdinalIgnoreCase))
            {
                DriveBox.SelectedIndex = i;
                break;
            }
        if (DriveBox.SelectedIndex < 0 && DriveBox.Items.Count > 0) DriveBox.SelectedIndex = 0;
        if (!CdmService.HostAvailable)
            Log("错误：未找到 CdmHost 负载（应随插件一同分发）：" + CdmService.HostExe);
        
        Unloaded += (_, _) => Cleanup();
        UpdatePassHint();
    }

    public static void CleanupStatic() => CdmService.KillCurrent();

    void Cleanup()
    {
        try { _cts?.Cancel(); } catch { }
        CdmService.KillCurrent();
    }

    async void RunButton_Click(object sender, RoutedEventArgs e)
    {
        if (_cts != null) return; 
        if (!CdmService.HostAvailable)
        {
            Log("错误：未找到 CdmHost 负载（应随插件一同分发）：" + CdmService.HostExe);
            return;
        }
        string drive = (DriveBox.SelectedItem as ComboBoxItem)?.Tag as string ?? "C:\\";

        _result = null;
        ScoreSeqRead.Text = "—";
        ScoreSeqWrite.Text = "—";
        Score4KRead.Text = "—";
        Score4KWrite.Text = "—";
        DetailText.Text = "";
        TestProgress.Maximum = CdmService.TotalRuns;
        TestProgress.Value = 0;

        _cts = new CancellationTokenSource();
        RunButton.IsEnabled = false;
        CancelButton.IsEnabled = true;
        SetRunning();
        try
        {
            Log($"开始硬盘基准（{drive.TrimEnd('\\')} 盘，SEQ1M Q8T1 + RND4K Q1T1，共 {CdmService.TotalRuns} 轮）");
            var progress = new Progress<CdmService.RunProgress>(p =>
                Dispatcher.BeginInvoke(() =>
                {
                    PhaseText.Text = p.Phase;
                    TestProgress.Value = p.RunsDone;
                }));
            var res = await _cdm.RunAsync(drive, progress, _cts.Token);
            TestProgress.Value = CdmService.TotalRuns;
            _result = res;
            _resultDrive = drive.TrimEnd('\\', ':');
            ScoreSeqRead.Text = res.SeqRead.ToString("0.0");
            ScoreSeqWrite.Text = res.SeqWrite.ToString("0.0");
            Score4KRead.Text = res.Rnd4KRead.ToString("0.0");
            Score4KWrite.Text = res.Rnd4KWrite.ToString("0.0");
            DetailText.Text = $"顺序 {res.SeqRead:0.0} / {res.SeqWrite:0.0} MB/s，4K 随机 {res.Rnd4KRead:0.0} / {res.Rnd4KWrite:0.0} MB/s"
                + $"（读 / 写）；成绩已保存：{res.JsonPath}";
            SetIdle("测试完成");
            Log($"完成：顺序 {res.SeqRead:0.0} / {res.SeqWrite:0.0}，4K 随机 {res.Rnd4KRead:0.0} / {res.Rnd4KWrite:0.0} MB/s（读/写）");
        }
        catch (OperationCanceledException)
        {
            SetIdle("已取消");
            Log("已取消");
        }
        catch (Exception ex)
        {
            SetIdle("出错");
            Log("错误：" + ex.Message.Split('\n')[0]);
        }
        finally
        {
            _cts.Dispose();
            _cts = null;
            RunButton.IsEnabled = true;
            CancelButton.IsEnabled = false;
            PhaseText.Text = "";
            UpdatePassHint();
        }
    }

    void CancelButton_Click(object sender, RoutedEventArgs e) => _cts?.Cancel();

    void OpenResultDir_Click(object sender, RoutedEventArgs e)
    {
        Directory.CreateDirectory(CdmService.ResultDir);
        System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
        {
            FileName = "explorer.exe",
            Arguments = $"\"{CdmService.ResultDir}\"",
            UseShellExecute = true,
        });
    }

    void SetRunning()
    {
        _passed = false;
        PassButton.IsEnabled = true;
        StatusText.Text = "跑分中…";
        StatusText.SetResourceReference(TextBlock.ForegroundProperty, "TextSecondaryBrush");
        UpdatePassHint();
        PassChanged?.Invoke(false);
    }

    void SetIdle(string text)
    {
        if (_passed) return;
        StatusText.Text = text;
        StatusText.SetResourceReference(TextBlock.ForegroundProperty, "TextSecondaryBrush");
    }

    void PassButton_Click(object sender, RoutedEventArgs e)
    {
        if (_cts != null) _cts.Cancel(); 
        _passed = true;
        StatusText.Text = "已通过";
        StatusText.Foreground = Brushes.ForestGreen;
        PassButton.IsEnabled = false;
        PassChanged?.Invoke(true);
    }

    void UpdatePassHint()
    {
        PassHintText.Text = _cts != null ? "正在跑分"
            : _result == null ? "尚未跑过分"
            : "已有成绩，可人工判断后通过";
    }

    public object? GetReportSection()
    {
        var history = CdmService.ReadScores(20);
        if (_result == null && history.Count == 0) return null;
        return new
        {
            drive = _resultDrive,
            latest = _result is not { } r ? null : new
            {
                seq_read = r.SeqRead, seq_write = r.SeqWrite, rnd4k_read = r.Rnd4KRead, rnd4k_write = r.Rnd4KWrite,
            },
            history = history.Select(h => new
            {
                time = h.Time, drive = h.Drive.TrimEnd(':'),
                seqRead = h.SeqRead, seqWrite = h.SeqWrite, rnd4kRead = h.Rnd4KRead, rnd4kWrite = h.Rnd4KWrite,
            }),
        };
    }

    void Log(string message)
    {
        LogBox.AppendText($"{DateTime.Now:HH:mm:ss}  {message}\n");
        if (LogBox.LineCount > 500)
        {
            int cut = LogBox.Text.IndexOf('\n');
            if (cut > 0) LogBox.Text = LogBox.Text[(cut + 1)..];
        }
        LogBox.ScrollToEnd();
    }
}
