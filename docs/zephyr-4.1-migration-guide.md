# ZMK/Zephyr 4.1 移行ガイド

このドキュメントは、ZMK の Zephyr 4.1 追従により発生したビルドエラーとその解決方法をまとめたものです。

## 発生した問題の概要

2025年12月、GitHub Actions でのビルドが突然失敗するようになりました。リポジトリのコードを変更していないにもかかわらず、ビルドが通らなくなった原因は **ZMK が Zephyr 4.1 系へ追従した** ためです。

ZMK の "Zephyr 4.1 Update" は **2025-12-09 に公開**されました。

参考: [Zephyr 4.1 Update | ZMK Firmware](https://zmk.dev/blog/2025/12/09/zephyr-4-1)

---

## 原因1: ボード名の変更

### エラーメッセージ
```
CMake Error ... Invalid BOARD
-DBOARD=seeeduino_xiao_ble
```

### 原因
ZMK が Zephyr 4.1 に追従する過程（board revision system 移行を含む）で、Seeed XIAO BLE のボード名が変更されました。

| 旧名 | 新名 |
|------|------|
| `seeeduino_xiao_ble` | `xiao_ble` |

**注意**: これは「Zephyr 4.1.0 単体での変更」というより、**ZMK が Zephyr 4.1 に追従する過程で ZMK 側の取り扱いとして変わった**ものです。

参考: [Zephyr 4.1 Update | ZMK Firmware](https://zmk.dev/blog/2025/12/09/zephyr-4-1)

### 修正方法
**ファイル**: `build.yaml`

```yaml
# Before
include:
  - board: seeeduino_xiao_ble
    shield: ZaruBall_left rgbled_adapter

# After
include:
  - board: xiao_ble
    shield: "ZaruBall_left;rgbled_adapter"
```

### shield の複数指定について

`SHIELD` は CMake の「リスト」として扱われるため、複数指定はセミコロン（`;`）区切りが適切です。スペース区切りだと問題になる場合があります。

これは「仕様が変わった」というより、**CMake/Zephyr の期待する表現に寄せた**修正です。

参考: [cmake: resolved list issue when parsing shield list #25716](https://github.com/zephyrproject-rtos/zephyr/issues/25716)

---

## 原因2: PMW3610 ドライバのバインディング変更

### エラーメッセージ
```
devicetree error: 'motion-gpios' is marked as required in 'properties:'
in /tmp/zmk-config/zephyr/dts/bindings/input/pixart,pmw3610.yaml

devicetree error: 'zephyr,axis-x' is marked as required in 'properties:'
in /tmp/zmk-config/zephyr/dts/bindings/input/pixart,pmw3610.yaml
```

### 原因
Zephyr 4.1 で PMW3610 ドライバが Zephyr 本体に統合され、デバイスツリーのプロパティ形式が変更されました。

### 修正方法
**ファイル**: `boards/shields/ZaruBall/ZaruBall.dtsi`

```dts
// Before (旧形式)
trackball: trackball@0 {
    status = "okay";
    compatible = "pixart,pmw3610";
    reg = <0>;
    spi-max-frequency = <2000000>;
    irq-gpios = <&gpio1 12 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    cpi = <600>;
    evt-type = <INPUT_EV_REL>;
    x-input-code = <INPUT_REL_X>;
    y-input-code = <INPUT_REL_Y>;
    force-awake;
};

// After (Zephyr 4.1 形式)
trackball: trackball@0 {
    status = "okay";
    compatible = "pixart,pmw3610";
    reg = <0>;
    spi-max-frequency = <2000000>;
    motion-gpios = <&gpio1 12 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    zephyr,axis-x = <INPUT_REL_Y>;
    zephyr,axis-y = <INPUT_REL_X>;
    res-cpi = <600>;
    force-awake;
};
```

### プロパティの変更点

| 旧プロパティ | 新プロパティ | 備考 |
|-------------|-------------|------|
| `irq-gpios` | `motion-gpios` | **必須**プロパティに変更 |
| `evt-type` | (削除) | 不要に |
| `x-input-code` | `zephyr,axis-x` | **必須**プロパティ |
| `y-input-code` | `zephyr,axis-y` | **必須**プロパティ |
| `cpi` | `res-cpi` | プロパティ名変更 |

参考: [pixart,pmw3610 — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/build/dts/api/bindings/input/pixart%2Cpmw3610.html)

### 軸の向き調整

**重要**: `zephyr,axis-x` と `zephyr,axis-y` を指定することは**必須**ですが、どのコードを割り当てるかは**センサーの物理的な取り付け向き次第**です。

```dts
// 標準的な設定
zephyr,axis-x = <INPUT_REL_X>;
zephyr,axis-y = <INPUT_REL_Y>;

// 90度回転補正が必要な場合（軸を入れ替え）
zephyr,axis-x = <INPUT_REL_Y>;
zephyr,axis-y = <INPUT_REL_X>;

// 反転が必要な場合は以下を追加
invert-x;  // X軸を反転
invert-y;  // Y軸を反転
```

取り付け向きに応じて、軸の入れ替えや反転の組み合わせを調整してください。

---

## 原因3: Kconfig シンボルの変更

### エラーメッセージ
```
warning: attempt to assign the value 'y' to the undefined symbol PMW3610
error: Aborting due to Kconfig warnings
```

### 原因
Zephyr 4.1 で PMW3610 の Kconfig シンボルが変更されました。

### 修正方法
**ファイル**: `config/ZaruBall.conf`

```conf
# Before (旧シンボル)
CONFIG_PMW3610=y
# CONFIG_PMW3610_INVERT_X=y
# CONFIG_PMW3610_INVERT_Y=n
# CONFIG_PMW3610_ORIENTATION_90=y

# After (Zephyr 4.1)
CONFIG_INPUT_PMW3610=y
```

**注意**: 旧ドライバの `CONFIG_PMW3610_INVERT_X` などのオプションは、新ドライバではデバイスツリーの `invert-x`, `invert-y` プロパティで設定します。

参考: [drivers/input/Kconfig.pmw3610 - Zephyr](https://pigweed.googlesource.com/third_party/github/zephyrproject-rtos/zephyr/+/refs/tags/v4.0.0/drivers/input/Kconfig.pmw3610)

---

## 再発防止策: upstream バージョンの固定

### 問題
`.github/workflows/build.yml` で `@main` を参照していると、ZMK/Zephyr の更新により突然ビルドが壊れる可能性があります。

```yaml
# 危険な設定（常に最新を追従）
uses: zmkfirmware/zmk/.github/workflows/build-user-config.yml@main
```

### 修正方法
**ファイル**: `.github/workflows/build.yml`

```yaml
# 安全な設定（特定のコミットに固定）
uses: zmkfirmware/zmk/.github/workflows/build-user-config.yml@118359c83efa0758144489fd5e12cc6f5bdbad4c
```

### バージョン固定の選択肢

| 方法 | メリット | デメリット |
|------|----------|------------|
| コミットSHA固定 | 完全に固定される | 読みにくい、更新時に探しにくい |
| リリースタグ固定（例: `@v0.3`） | 読みやすく管理しやすい | タグがない場合は使えない |

運用上は**リリースタグ固定の方が管理しやすい**場合もあります。プロジェクトの状況に応じて選択してください。

### コミットSHAの取得方法
```bash
curl -s "https://api.github.com/repos/zmkfirmware/zmk/commits/main" | jq -r '.sha'
```

---

## 修正ファイル一覧

| ファイル | 修正内容 |
|----------|----------|
| `build.yaml` | ボード名を `xiao_ble` に変更、shield 区切りを `;` に |
| `.github/workflows/build.yml` | `@main` を特定コミットSHAに固定 |
| `boards/shields/ZaruBall/ZaruBall.dtsi` | PMW3610 のプロパティを新形式に更新 |
| `config/ZaruBall.conf` | `CONFIG_PMW3610` を `CONFIG_INPUT_PMW3610` に変更 |

---

## 参考リンク

- [Zephyr 4.1 Update | ZMK Firmware](https://zmk.dev/blog/2025/12/09/zephyr-4-1)
- [pixart,pmw3610 — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/build/dts/api/bindings/input/pixart%2Cpmw3610.html)
- [ZMK 公式ドキュメント](https://zmk.dev/docs)
- [cmake: resolved list issue when parsing shield list #25716](https://github.com/zephyrproject-rtos/zephyr/issues/25716)

---

## 更新履歴

- 2025-12-18: 初版作成（ZMK Zephyr 4.1 追従対応）
