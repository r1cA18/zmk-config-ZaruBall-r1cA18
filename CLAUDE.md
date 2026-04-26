# CLAUDE.md

このリポは自作キーボード **ZaruBall** 用の ZMK ファームウェア設定。daily-driver で使うので、再現性と安定性を優先する。

## 最初に読むべき docs

- `README.md` — レイヤー構成、ハードウェア仕様、ビルド方法
- `docs/dependency-pinning.md` — 依存 pin ポリシー（ZMK 本体・モジュール・workflow を全部 pin）
- `docs/left-side-sleep-wake-investigation.md` — 左側スリープ復帰問題の切り分け記録
- `docs/zephyr-4.1-migration-guide.md` — Zephyr 4.1 移行時の注意
- `docs/logs/` — 日付別の作業ログ（直近の調査・変更の経緯）

## ハードウェア構成（要点）

- **ボード**: Seeeduino XIAO BLE (`xiao_ble`, nRF52840)
- **構成**: 左右分離 BLE。右側に USB（central）、左側は USB 無し（peripheral）
- **キースキャン**: charlieplex (`zmk,kscan-gpio-charlieplex`)、16 col × 18 row
- **トラックボール**: PMW3610（**右側のみ**）、SPI、CPI 600、`force-awake` 有効
- **エンコーダ**: 左側のみ、ALPS EC11、30 ステップ、15 trigger/rotation
- **LED**: caksoylar/zmk-rgbled-widget でレイヤー/バッテリ表示
- 左側 overlay は `&spi0` / `&trackball` を `disabled`、`&left_encoder` を `okay`

## ビルド構成

`build.yaml`:

- `xiao_ble` + shield `ZaruBall_left;rgbled_adapter`
- `xiao_ble` + shield `ZaruBall_right;rgbled_adapter` + snippet `studio-rpc-usb-uart`（ZMK Studio用）
- `xiao_ble` + shield `settings_reset`

通常はリポを GitHub に push → GitHub Actions でビルド → Artifacts から uf2 を落として焼く。ローカルビルドは west でも可。

## レイヤー & 色対応

| Layer | 名前 | 用途 | LED 色（RGBLED_WIDGET） |
|------:|------|------|:----------:|
| 0 | Main | 通常タイピング | 0 = OFF |
| 1 | Lower | 数字 / F | 1 = RED |
| 2 | Raise | 記号 / カーソル | 4 = BLUE |
| 3 | Mouse | マウス（AML自動切替） | **5 = MAGENTA**（紫） |
| 4 | Scroll | トラックボール→スクロール | 6 = CYAN |
| 5 | Control | BT / メディア / マクロ | 2 = GREEN |

`CONFIG_RGBLED_WIDGET_LAYER_*_COLOR` で定義。**紫はレイヤー3 (Mouse) の正常表示**。フリーズと混同しないこと。

## ファイル構造

```
config/
  ZaruBall.keymap   # キーマップ・behaviors・macros
  ZaruBall.conf     # グローバル Kconfig（idle, RGBLED, etc.）
  west.yml          # 依存 module（ZMK / naginata / rgbled-widget を pin）
boards/shields/ZaruBall/
  ZaruBall.dtsi          # 共通: physical layout / matrix transform / kscan / SPI / trackball
  ZaruBall_left.overlay  # 左: kscan ピン、encoder 有効、SPI/trackball 無効
  ZaruBall_right.overlay # 右: kscan ピン、trackball_listener 有効、xiao_serial 無効
  ZaruBall_left.conf     # 左: USB 無効
  ZaruBall_right.conf    # 右: ZMK Studio 有効
src/
  input_processor_scroll_stepper.c  # 自前のスクロール段階化プロセッサ
  module_anchor.c
dts/
  input/processors/scroll_stepper.dtsi
  bindings/input_processors/zmk,input-processor-stepper.yaml
init_firmware/
  *.uf2  # 初期化用 uf2（settings_reset / rgbled_adapter）
docs/
  *.md   # 調査ログ・移行ガイド
```

## 重要な設計判断（やる前に確認）

- **依存は必ず pin**。`west.yml` の revision は **タグが無いものは commit hash**、ZMK は手動 pin。更新は1モジュールずつ、両側ビルドして実機検証してからコミット
- **CONFIG_ZMK_SLEEP=n** のまま運用（左側スリープ復帰問題のため）。`CONFIG_ZMK_IDLE_TIMEOUT=30000` で IDLE は使う
- **左側で trackball / SPI を `disabled`**、`&left_encoder` を `okay` にしている。左用 overlay を触るときに上書きしないこと
- **右の overlay は `xiao_serial` を `disabled` に**。これは ZMK Studio の studio-rpc-usb-uart スニペットと共存させるため
- **ホームロウモッドのチューニング**: `tapping-term-ms = 300` / `require-prior-idle-ms = 200` / `hold-trigger-on-release`。誤発動防ぎ目的なので不用意に下げない
- **キースキャン debounce**: `kscan0` に `debounce-press-ms = 5` / `debounce-release-ms = 30` を設定（チャタリング対策）。release 側を上げているのは「押下中の一瞬の release バウンス」を吸収するため。**Kconfig の `CONFIG_ZMK_KSCAN_DEBOUNCE_*` は使わない**（DT 値を全 kscan に上書きするため）

## ZMK 固有のハマりどころ

- **DT property vs Kconfig**: ZMK の `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS` / `..._RELEASE_MS` のデフォルトは `-1`（DT を尊重）。`-1` 以外を入れると全 kscan の DT を上書きする。基本は DT property を使う
- **charlieplex bindings**: `debounce-press-ms` / `debounce-release-ms` / `debounce-scan-period-ms` を正式サポート（公式 docs の対応表は古いことがある、bindings yaml を一次情報とする）
- **input processor の自作**: `dts/bindings/input_processors/` に yaml、`src/` に C 実装、`dts/input/processors/*.dtsi` で node 定義。`module_anchor.c` がモジュールアンカー
- **ZMK Studio**: 右側 only。`config/ZaruBall_right.conf` に `CONFIG_ZMK_STUDIO=y` / `CONFIG_ZMK_STUDIO_LOCKING=n`

## 開発ワークフロー

- ブランチ: `feature/xxx`, `fix/xxx`, `docs/xxx`。`main` への直 push は小さい修正のみ
- コミット: Conventional Commits 英語（`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`）
- キーマップ・ハード設定の変更は **両側ビルド + 実機検証** してからマージ
- 調査ログは `docs/<topic>-investigation.md` に残す（既存の sleep 調査ファイルがテンプレ）
- 絵文字は使わない（コード・コメント・コミット・Markdown 全てで）

## 作業ログ運用

**コード変更・調査・方針決定をしたら、その日のうちに `docs/logs/YYYY-MM-DD.md` に追記する**。同じ日に複数作業した場合は同じファイルに追記（新規作成しない）。

書く内容:

- **症状 / きっかけ**（何を解決したかったか）
- **分析 / 確定情報**（一次情報のリンク必須）
- **対応 / 変更内容**（差分の要約、意図、代替案を却下した理由）
- **次のステップ**（残タスク、未実施の切り分け案）
- **失敗・誤解**（調査エージェントが外した点、自分の誤読）

目的: AI エージェントが過去の経緯を読んで「なぜこの設定なのか」を辿れるようにする。`docs/<topic>-investigation.md` は**長期に追跡する個別トピック**用、`docs/logs/` は**時系列の作業記録**用と使い分ける。

## 検証チェックリスト（ハード設定変更時）

1. GitHub Actions で左右両方ビルドが通る
2. 左右ペアリングが復活する（central 側 = 右 → reset 後ペアリングし直しが必要なケースあり）
3. 左側エンコーダが動く
4. 右側トラックボールが動く（X/Y 軸の向き、AML、Scroll レイヤー）
5. ZMK Studio が右側で接続できる
6. バッテリ表示 LED が出る
7. レイヤー切替時の LED 色が想定通り

## 既知の問題

- **左側スリープ復帰失敗**: `CONFIG_ZMK_SLEEP=n` で回避中。再有効化検討時は `docs/left-side-sleep-wake-investigation.md` を読む
- 他は調査次第で本ファイルの「既知の問題」セクションに追記
