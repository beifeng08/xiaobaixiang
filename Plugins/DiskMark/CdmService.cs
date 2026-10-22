// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.Json;

namespace YanJi.Plugin.DiskMark;

public class CdmService
{
    
    public record CdmResult(double SeqRead, double SeqWrite, double Rnd4KRead, double Rnd4KWrite, string JsonPath);

    public class RunProgress
    {
        public string Phase { get; set; } = "";
        public int RunsDone { get; set; }
    }

    public const int TotalRuns = 24;

    public const int RunsPerItem = TotalRuns / 4;

    public event Action<string>? Logged;

    public static string PluginDir =>
        Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)!;

    public static string HostExe => Path.Combine(PluginDir, "CdmHost", "CdmHost.exe");

    public static bool HostAvailable => File.Exists(HostExe);

    public static string ResultDir => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "YanJi", "disk_result");

    public static string JsonPath => Path.Combine(ResultDir, "硬盘基准.json");

    public static string ScoresCsvPath => Path.Combine(ResultDir, "硬盘成绩记录.csv");

    static Process? _current;

    public static string NormalizeDrive(string drive)
    {
        string d = drive.Trim().TrimEnd(':', '\\', '/').Trim().ToUpperInvariant();
        if (d.Length != 1 || d[0] < 'A' || d[0] > 'Z')
            throw new ArgumentException("无效盘符：" + drive);
        return d;
    }

    public async Task<CdmResult> RunAsync(string drive, IProgress<RunProgress>? progress, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        string exe = HostExe;
        if (!File.Exists(exe))
            throw new FileNotFoundException("未找到 CdmHost 负载（应随插件一同分发）：" + exe);
        drive = NormalizeDrive(drive);

        var psi = new ProcessStartInfo
        {
            FileName = exe,
            Arguments = drive,
            WorkingDirectory = Path.GetDirectoryName(exe)!, 
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
            StandardOutputEncoding = Encoding.UTF8,
            StandardErrorEncoding = Encoding.UTF8,
        };
        using var proc = new Process { StartInfo = psi, EnableRaisingEvents = true };
        var sb = new StringBuilder();
        string? jsonLine = null;
        int runs = 0;          
        int itemRuns = 0;      
        string io = "";        
        string op = "";        
        string phaseKey = "";  
        bool sawTotalIo = false;
        void Report(string phase, int runsDone) =>
            progress?.Report(new RunProgress { Phase = phase, RunsDone = runsDone });
        proc.OutputDataReceived += (_, e) =>
        {
            if (e.Data == null) return;
            sb.AppendLine(e.Data);
            var line = e.Data.Trim();
            if (line.StartsWith("{") && line.EndsWith("}"))
            {
                jsonLine = line;
            }
            else if (line.StartsWith("Command Line:", StringComparison.OrdinalIgnoreCase))
            {
                
                runs++;
                sawTotalIo = false;
                string ioNew = line.Contains("-b4K", StringComparison.OrdinalIgnoreCase) ? "4K 随机" : "顺序";
                string opNew = line.Contains("-w100", StringComparison.Ordinal) ? "写入" : "读取";
                string key = ioNew + opNew;
                if (key != phaseKey) { phaseKey = key; io = ioNew; op = opNew; itemRuns = 1; }
                else itemRuns++;
                Report($"{io}{op} 第 {itemRuns}/{RunsPerItem} 轮", runs - 1);
            }
            else if (line.StartsWith("Total IO", StringComparison.Ordinal)) sawTotalIo = true;
            else if (sawTotalIo && line.StartsWith("total:", StringComparison.OrdinalIgnoreCase))
            {
                sawTotalIo = false;
                
                var parts = line.Split('|');
                string mib = parts.Length >= 3 ? parts[2].Trim() : "?";
                Logged?.Invoke($"{io}{op} 第 {itemRuns}/{RunsPerItem} 轮完成：{mib} MB/s");
                Report($"{io}{op} 第 {itemRuns}/{RunsPerItem} 轮", runs);
            }
            else if (line.EndsWith("..."))
            {
                
                Logged?.Invoke(line.TrimEnd('.'));
                Report(line, runs);
            }
        };
        proc.ErrorDataReceived += (_, e) => { if (e.Data != null) sb.AppendLine(e.Data); };

        ct.ThrowIfCancellationRequested();
        proc.Start();
        _current = proc;
        try
        {
            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();
            using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(ct);
            timeoutCts.CancelAfter(TimeSpan.FromMinutes(15)); 
            try
            {
                await using (timeoutCts.Token.Register(() => { try { proc.Kill(true); } catch {  } }))
                {
                    await proc.WaitForExitAsync(timeoutCts.Token).ConfigureAwait(false);
                }
            }
            catch (OperationCanceledException) when (!ct.IsCancellationRequested)
            {
                throw new TimeoutException("硬盘基准超时（15 分钟未完成）");
            }
        }
        finally
        {
            _current = null;
        }

        ct.ThrowIfCancellationRequested();
        if (proc.ExitCode != 0)
            throw new InvalidOperationException($"CdmHost 运行失败（退出码 {proc.ExitCode}）：\n{Tail(sb.ToString(), 1200)}");
        if (jsonLine == null)
            throw new InvalidOperationException($"CdmHost 未输出成绩（退出码 {proc.ExitCode}）：\n{Tail(sb.ToString(), 1200)}");

        var result = ParseScores(jsonLine, JsonPath);
        Directory.CreateDirectory(ResultDir);
        File.WriteAllText(JsonPath, jsonLine + "\n", new UTF8Encoding(false));
        AppendScore(drive + ":", result);
        return result;
    }

    public static CdmResult ParseScores(string json, string jsonPath)
    {
        using var doc = JsonDocument.Parse(json);
        var r = doc.RootElement;
        var result = new CdmResult(
            r.GetProperty("seq_read").GetDouble(),
            r.GetProperty("seq_write").GetDouble(),
            r.GetProperty("rnd4k_read").GetDouble(),
            r.GetProperty("rnd4k_write").GetDouble(),
            jsonPath);
        if (!double.IsFinite(result.SeqRead) || result.SeqRead < 0 ||
            !double.IsFinite(result.SeqWrite) || result.SeqWrite < 0 ||
            !double.IsFinite(result.Rnd4KRead) || result.Rnd4KRead < 0 ||
            !double.IsFinite(result.Rnd4KWrite) || result.Rnd4KWrite < 0)
            throw new FormatException("CdmHost 成绩必须为有限非负数");
        return result;
    }

    public static void AppendScore(string drive, CdmResult res)
    {
        try
        {
            bool newFile = !File.Exists(ScoresCsvPath);
            using var w = new StreamWriter(ScoresCsvPath, append: true,
                new UTF8Encoding(encoderShouldEmitUTF8Identifier: true));
            if (newFile) w.WriteLine("时间,盘符,顺序读取(MB/s),顺序写入(MB/s),随机读取4K(MB/s),随机写入4K(MB/s)");
            w.WriteLine(string.Join(',',
                DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
                drive,
                res.SeqRead.ToString("0.0", System.Globalization.CultureInfo.InvariantCulture),
                res.SeqWrite.ToString("0.0", System.Globalization.CultureInfo.InvariantCulture),
                res.Rnd4KRead.ToString("0.0", System.Globalization.CultureInfo.InvariantCulture),
                res.Rnd4KWrite.ToString("0.0", System.Globalization.CultureInfo.InvariantCulture)));
        }
        catch {  }
    }

    static string Tail(string s, int n) => s.Length <= n ? s : s[^n..];

    public record ScoreRow(string Time, string Drive, double SeqRead, double SeqWrite, double Rnd4KRead, double Rnd4KWrite);

    public static List<ScoreRow> ReadScores(int max)
    {
        var rows = new List<ScoreRow>();
        try
        {
            if (!File.Exists(ScoresCsvPath)) return rows;
            var inv = System.Globalization.CultureInfo.InvariantCulture;
            foreach (string line in File.ReadLines(ScoresCsvPath).Skip(1))
            {
                string[] c = line.Split(',');
                if (c.Length != 6) continue;
                if (double.TryParse(c[2], inv, out double sr) && double.TryParse(c[3], inv, out double sw)
                    && double.TryParse(c[4], inv, out double rr) && double.TryParse(c[5], inv, out double rw))
                    rows.Add(new ScoreRow(c[0], c[1], sr, sw, rr, rw));
            }
        }
        catch {  }
        rows.Reverse();
        return rows.Count > max ? rows[..max] : rows;
    }

    public static void KillCurrent()
    {
        try { _current?.Kill(entireProcessTree: true); } catch {  }
    }
}
