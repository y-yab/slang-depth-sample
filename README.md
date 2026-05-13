# slang-depth-sample

Slang + slang-rhi (D3D12 バックエンド) を使った深度バッファの学習用サンプルプロジェクトです。

## 描画モード

| モード | 概要 |
|---|---|
| Direct (`is_texture_composition_ = false`) | Box 2 つを直接 BackBuffer に描画 |
| Composition (`is_texture_composition_ = true`) | Box を個別の中間テクスチャに描画してから Depth 情報を保持しつつ BackBuffer に合成 |

## ビルド

```powershell
msbuild slang_depth_sample.sln /p:Configuration=Debug /p:Platform=x64 /t:build
```

vcpkg の依存ライブラリは `vcpkg.json` で管理されており、bootstrap.ps1 で初期セットアップできます。

## ドキュメント

| ファイル | 内容 |
|---|---|
| [docs/texture-composition-pipeline-ja.md](docs/texture-composition-pipeline-ja.md) | 中間テクスチャ合成パイプラインの実装と落とし穴 |
| [docs/box-winding-and-culling.md](docs/box-winding-and-culling.md) | Box およびスクリーン空間クアッドの頂点巻き順と Back-face Culling |
| [docs/left-handed-face-orientation-ja.md](docs/left-handed-face-orientation-ja.md) | 左手座標系での面向き早見表 |
| [docs/shader-matrix-conventions-ja.md](docs/shader-matrix-conventions-ja.md) | シェーダーの行列演算順序 (DirectX / Slang) |