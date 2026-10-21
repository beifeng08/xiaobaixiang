using System.Windows;
using YanJi.Services;

namespace YanJi.Views;

/// <summary>验机前登记：录入已知配置（订单/卖家描述），供报告与采集值并排对照。不做任何自动判定。</summary>
public partial class IntakeWindow : Window
{
    public sealed class Row
    {
        public required string No { get; init; }
        public required string Key { get; init; }
        public required string Label { get; init; }
        public required string Hint { get; init; }
        public bool Multiline { get; init; }
        public string Value { get; set; } = "";
    }

    static readonly (string Key, string Label, string Hint)[] Fields =
    [
        ("device", "设备编号", "自定义编号即可，避免填写完整序列号"),
        ("model", "整机型号", "如 82WM、拯救者 Y9000P"),
        ("cpu", "处理器", "如 i7-13700H"),
        ("gpu", "显卡", "如 RTX 4060"),
        ("memory", "内存", "如 16G DDR5"),
        ("storage", "硬盘", "如 1TB 三星"),
        ("display", "屏幕", "如 2560x1600 165Hz"),
        ("battery", "电池", "如 80Wh"),
        ("notes", "其它备注", "卖家描述、成色、已知问题等"),
    ];

    readonly List<Row> _rows;

    public IReadOnlyList<ExpectedField> Result { get; private set; } = [];
    public string? DeviceLabel { get; private set; }

    public IntakeWindow(IReadOnlyList<ExpectedField> current, string? deviceLabel)
    {
        InitializeComponent();
        _rows = Fields.Select((f, i) => new Row
        {
            No = (i + 1).ToString("00"),
            Key = f.Key,
            Label = f.Label,
            Hint = f.Hint,
            Multiline = f.Key == "notes",
            Value = f.Key == "device" ? deviceLabel ?? "" : current.FirstOrDefault(x => x.Key == f.Key)?.Value ?? "",
        }).ToList();
        FieldList.ItemsSource = _rows;
        Loaded += (_, _) => ThemeManager.ApplyTitleBar(this);
    }

    void Start_Click(object sender, RoutedEventArgs e)
    {
        DeviceLabel = _rows.First(r => r.Key == "device").Value.Trim() is { Length: > 0 } d ? d : null;
        Result = _rows.Where(r => r.Key != "device" && r.Value.Trim().Length > 0)
            .Select(r => new ExpectedField(r.Key, r.Label, r.Value.Trim())).ToList();
        DialogResult = true;
    }

    void Skip_Click(object sender, RoutedEventArgs e) => DialogResult = false;
}
