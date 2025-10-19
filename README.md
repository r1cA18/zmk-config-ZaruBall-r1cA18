# ZaruBall-v3.x ファームウェア（40％カスタム & 装飾キー最適化）

自作キーボードZaruBall-v3.xのブランチをベースに、**40%キーボード的レイアウト & 独自装飾・マウス操作キー配置**へカスタムした自分用のファームウェアです。  
Macでの操作性、テンティング姿勢での最小限の移動を重視し、手に馴染むよう割り当てを調整しました。

***

## カスタム内容

- 40%配列を意識した最小限キー＋最小レイヤー
- 装飾キー（Ctrl, Shift, Alt, Command）のポジション移動
- 打鍵しやすい場所へ**マウスクリックやカーソルキー機能**を割り当て
- レイヤー切り替えやワンショットキーのカスタマイズ(Mac, Arcのショトカ)

***

## ビルド方法

1. GitHubでこのリポジトリをフォーク
2. 上部「Actions」タブを開く
3. 左メニュー `.github/workflows/build.yml` をクリック
4. 右側「Run Workflow」→ブランチを選択→実行
5. 数分でビルドが完了し、緑のチェックで成功がわかる
6. 「Artifacts」からビルド済みファームをダウンロード＆解凍
7. `left`, `right`, `reset` 3種ファイルが揃ってるか確認
8. 右手キーボードをリセット＆PC認識
9. エクスプローラーで `right` ファイルを書き込み
10. 左手キーボードも同様に `left` ファイル
11. 完了後、Bluetoothペアリング

***

## キーマップの編集方法

- `.keymap` ファイルを直接編集
    - 優先されるのは `config/ZaruBall.keymap`
    - 公式参考: [https://zmk.dev/docs/keymaps](https://zmk.dev/docs/keymaps)
- **keymap editor**
    - [https://nickcoutsos.github.io/keymap-editor/](https://nickcoutsos.github.io/keymap-editor/)
    - Forkしたリポジトリとブランチ指定して使える
    - 編集後Saveで自動ビルド
- **ZMK Studio**
    - &studio_unlock キー押下後編集可能
    - Saveでキーボード即書き込み

***

## オリジナル＆参考リポジトリ

- 元リポジトリ: https://github.com/ImSota/zmk-config-ZaruBall
- keymap editor: https://nickcoutsos.github.io/keymap-editor/
