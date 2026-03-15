# ZaruBall Firmware

> 自作キーボード **ZaruBall** 用のZMKファームウェア設定です

自分の好みにキーマップを編集したら、GitHub Actionsからビルドしてファームウェアを取得できます。
**keymap editor** / **ZMK Studio** どちらにも対応しています。

この repo は daily-driver 用なので、ZMK 本体・workflow・外部 module を pin して再現性を優先しています。
詳細は [docs/dependency-pinning.md](docs/dependency-pinning.md) を参照。

---

## このキーマップの特徴

### レイヤー構成

```
┌─────────────┬──────────────────────────────────────────────┐
│ Layer 0     │ Main - メインタイピング                      │
├─────────────┼──────────────────────────────────────────────┤
│ Layer 1     │ Lower - 数字 / Fキー                         │
├─────────────┼──────────────────────────────────────────────┤
│ Layer 2     │ Raise - 記号 / カーソル移動                  │
├─────────────┼──────────────────────────────────────────────┤
│ Layer 3     │ Mouse - マウス操作                           │
├─────────────┼──────────────────────────────────────────────┤
│ Layer 4     │ Scroll - スクロールモード                    │
├─────────────┼──────────────────────────────────────────────┤
│ Layer 5     │ Control - BT制御 / メディア / マクロ         │
└─────────────┴──────────────────────────────────────────────┘
```

### ホームロウモッド (HRM)

ホームポジションに修飾キーを配置。
高速タイピング時の誤発動を防ぐチューニング済み：

| 左手 | キー | 右手 | キー |
|:----:|:----:|:----:|:----:|
| Alt | D | - | - |
| Shift+GUI | F | - | - |
| Control | G | - | - |
| - | - | Hyper | ; |

```
tapping-term-ms = 300   // ホールド判定時間
require-prior-idle-ms = 200  // 連続タイプ中は発動しない
```

### トラックボール連携

- **AML (Auto Mouse Layer)**: トラックボール操作で自動的にMouseレイヤーに移行（500ms維持）
- **スクロールモード**: Layer 4 でトラックボールがスクロールホイールに変化
- 軸変換設定でトラックボールの向きに対応

### その他の便利機能

| 機能 | 説明 |
|:-----|:-----|
| SandS | Space長押しでShift |
| ZMK Studio | `&studio_unlock` で接続中にリアルタイム編集可能 |
| 入力マクロ | ユーザーID / メールアドレスをワンキーで入力 |
| ロータリーエンコーダ | Mainでボリューム / Raiseでカーソル移動 |
| BT接続切替 | Controlレイヤーで BT0-4 を切り替え可能 |

---

## ハードウェア設定

### キースキャン

- **方式**: Charlieplex マトリクス (`zmk,kscan-gpio-charlieplex`)
- **マトリクス**: 16 columns × 18 rows

### トラックボール

- **センサー**: PMW3610 (SPI接続)
- **CPI**: 600
- **ピン配置**:
  - SPI SCK: P1.15
  - SPI MOSI/MISO: P1.13
  - CS: P1.14
  - Motion: P1.12

### ロータリーエンコーダ

- **ステップ数**: 30
- **トリガー/回転**: 15
- **ピン配置**: P0.9 / P0.10

---

## ビルド方法

### GitHub Actions でビルド

1. リポジトリ上部の **Actions** タブを開く
2. 左サイドバーから `.github/workflows/build.yml` を選択
3. **Run workflow** → ブランチを選んで実行
4. ビルド完了後、ページ下部の **Artifacts** からダウンロード

### ファームウェアの書き込み

1. ダウンロードしたzipを解凍（`left` / `right` / `reset` の3ファイル）
2. **右手キーボード**をUSB接続
3. リセットボタンをダブルクリック → PCに `XIAO SENSE` として認識
4. `right` の `.uf2` ファイルをコピー
5. **左手キーボード**も同様に `left` ファイルを書き込み
6. Bluetooth接続して完了！

> `reset` ファイルは不具合発生時のリカバリ用です

---

## キーマップの編集

### 方法1: `.keymap` ファイルを直接編集

最も自由度が高い方法。ZMKの全機能を利用可能：

**編集ファイル**: [`config/ZaruBall.keymap`](config/ZaruBall.keymap)

> `boards/shields/` 配下にも同名ファイルがありますが、`config/` が優先されます

**参考ドキュメント**: [ZMK Keymaps](https://zmk.dev/docs/keymaps)

### 方法2: Keymap Editor（GUI）

ブラウザ上でビジュアル編集：

1. [keymap editor](https://nickcoutsos.github.io/keymap-editor/) を開く
2. GitHubアカウントでログイン
3. このリポジトリを選択
4. GUIで編集 → **Save** で自動ビルド開始

### 方法3: ZMK Studio（リアルタイム）

ビルド不要！接続中のキーボードを直接編集：

| 版 | 特徴 |
|:---|:-----|
| アプリ版 | USB / Bluetooth どちらでも使用可 |
| ブラウザ版 | USB接続時のみ |

1. [ZMK Studio](https://zmk.studio/) を起動
2. キーボードで `&studio_unlock` キーを押下
3. リアルタイムで編集 → 右上の保存ボタンで即時反映

---

## 設定ファイル

| ファイル | 説明 |
|:---------|:-----|
| `config/ZaruBall.keymap` | キーマップ定義 |
| `config/ZaruBall.conf` | Kconfig設定（ドライバ有効化など） |
| `boards/shields/ZaruBall/ZaruBall.dtsi` | ハードウェア定義（共通） |
| `boards/shields/ZaruBall/ZaruBall_left.overlay` | 左手側オーバーレイ |
| `boards/shields/ZaruBall/ZaruBall_right.overlay` | 右手側オーバーレイ |

---

## ブランチ

| ブランチ | 説明 |
|:---------|:-----|
| `main` | 現行版（Mac専用、シンプル構成） |
| `feature/multi-device-support` | Mac/Windows/iPad 3デバイス対応版 |

---

## 関連リンク

- [ZMK Firmware 公式ドキュメント](https://zmk.dev/docs)
- [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)
- [ZMK Studio](https://zmk.studio/)

---

## Credits

**Fork元リポジトリ**: [ImSota/zmk-config-ZaruBall](https://github.com/ImSota/zmk-config-ZaruBall)
**参考キーマップ**: [keymap/claw44.pdf](https://github.com/ryoppippi/dotfiles/blob/2b48d80e095f08307b3ff717f03baa595314f4b1/keymap/claw44.pdf)

このプロジェクトは上記リポジトリをフォークし、 **ryoppippi** さんに影響を受けてキーマップ設定を編集したものです。
ZaruBallキーボードの開発者である **zaruSaru** さん、ワクワクするキーマップを公開してくださっている **ryoppippi** さん、本当にありがとうございます。
