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

## 現在の実運用設定

### config/ZaruBall.conf
```kconfig
CONFIG_ZMK_IDLE_TIMEOUT=30000
CONFIG_ZMK_SLEEP=n
```

### boards/shields/ZaruBall/ZaruBall_left.overlay
```dts
// 左手側は trackball を使わない
&spi0 {
    status = "disabled";
};

&trackball {
    status = "disabled";
};

// ダイヤルは再度有効化
&left_encoder {
    status = "okay";
};
```

## テスト履歴

### 2026-01-30: 初回テスト

1. **Step 1**: スリープ無効化 → 問題解消（スリープ復帰経路が原因と確定）
2. **Step 3**: interrupt-gpios極性変更 → 左側が完全に動作しなくなった → 元に戻した
3. **その後**: 普段使い優先で `CONFIG_ZMK_SLEEP=n` に戻し、左側 trackball のみ無効化

## 次のステップ

1. sleep を再度有効化したい場合は、現行 pinned 依存のまま別ブランチで検証する
2. 左手側の不安定さが再発したら、まず encoder を切って再比較する
3. それでも再発するなら、電源回路か split BLE 周りを疑う

## 参考: 関連ファイル

- `config/ZaruBall.conf` - グローバル設定
- `boards/shields/ZaruBall/ZaruBall_left.overlay` - 左側固有設定
- `boards/shields/ZaruBall/ZaruBall.dtsi` - 共通デバイスツリー

## 備考

- このメモは「sleep 復帰問題」の切り分け記録であって、現行設定の完全な仕様書ではない
- 現行の依存 pin は `docs/dependency-pinning.md` を参照
