# 左手側スリープ復帰問題 調査記録

## 問題の概要

- **症状**: 長時間操作なし→スリープ→復帰時、左手側が起動しない
- **詳細**: LEDすら光らない
- **暫定対処**: 電源オフ→5-10秒待機→電源オンで復旧

## 調査結果

### 確定事項

| 検証内容 | 結果 |
|----------|------|
| スリープ無効化 (`CONFIG_ZMK_SLEEP=n`) | 問題消える → **スリープ復帰経路が原因** |
| interrupt-gpios極性変更 (`ACTIVE_LOW + PULL_UP`) | 左側が完全に動作しなくなる → **これは原因ではない** |

### 原因候補（当初の想定）

| 原因 | 確度 | 検証結果 |
|------|------|----------|
| 割り込みピン（D10）の設定問題 | 70% | 極性変更はNG、元の設定が正しい |
| バッテリ保護IC/電源回路 | 50% | 未検証 |
| SPI/trackball有効化 | 30% | 左側で無効化テスト中 |
| ピン競合（エンコーダ/kscan） | 25% | エンコーダ無効化テスト中 |
| BLE再接続失敗 | 15% | 未検証 |

## 現在のテスト設定

### config/ZaruBall.conf
```kconfig
CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=3600000  # 1時間
```

### boards/shields/ZaruBall/ZaruBall_left.overlay
```dts
// SPI/trackball無効化（左側では不要）
&spi0 {
    status = "disabled";
};

&trackball {
    status = "disabled";
};

// エンコーダ無効化（ピン競合確認）
&left_encoder {
    status = "disabled";
};
```

## テスト履歴

### 2026-01-30: 初回テスト

1. **Step 1**: スリープ無効化 → 問題解消（スリープ復帰経路が原因と確定）
2. **Step 3**: interrupt-gpios極性変更 → 左側が完全に動作しなくなった → 元に戻した
3. **現在**: スリープ1時間 + SPI/エンコーダ無効化でテスト中

## 次のステップ

1. 1時間スリープ設定でテスト
   - 問題再発しない → SPI/エンコーダ無効化が有効
   - 問題再発する → 他の原因を調査

2. 問題再発しない場合の追加検証
   - エンコーダだけ有効に戻してテスト
   - SPI/trackballだけ有効に戻してテスト
   - どちらが原因か特定

## 参考: 関連ファイル

- `config/ZaruBall.conf` - グローバル設定
- `boards/shields/ZaruBall/ZaruBall_left.overlay` - 左側固有設定
- `boards/shields/ZaruBall/ZaruBall.dtsi` - 共通デバイスツリー

## 関連コミット

- `715072a` fix: remove BLE params causing left side sleep wake failure
- このブランチ: `debug/left-side-sleep-wake`
