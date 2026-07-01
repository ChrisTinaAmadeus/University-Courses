from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
import requests


WENLAN_IMAGE_URL = "http://116.204.91.119:6175/image_query"


def extract_image_embedding(image_path: str | Path, timeout: int = 30) -> List[float]:
    """
    使用文澜 API 提取单张图片的特征向量。

    返回: 浮点列表（embedding）。
    失败时抛出异常（含 HTTP 状态或解析错误）。
    """
    image_path = Path(image_path)
    if not image_path.exists():
        raise FileNotFoundError(f"图片不存在: {image_path}")

    with image_path.open("rb") as f:
        files = {"image": (image_path.name, f, "image/jpeg")}
        resp = requests.post(WENLAN_IMAGE_URL, files=files, timeout=timeout)
    resp.raise_for_status()
    data = resp.json()
    # 期望 data 是 embedding 列表；若服务返回字典，尝试取常见键
    if isinstance(data, dict):
        for key in ("embedding", "vector", "data", "feat", "feature"):
            if key in data:
                data = data[key]
                break
    if not isinstance(data, (list, tuple)):
        raise ValueError(f"无法解析特征返回格式: {type(data)} -> {data}")
    return [float(x) for x in data]


def extract_embeddings_from_manifest(
    manifest_path: str | Path,
    out_npz: str | Path,
    skip_if_exists: bool = False,
    max_items: int | None = None,
    timeout: int = 30,
) -> Tuple[Path, int]:
    """
    从 data.py 生成的 manifest.json 读取图片路径与标签，调用文澜 API 批量提取并保存。

    保存为 npz 包含:
      - features: (N, D) float32
      - labels: (N,) int64
      - paths:  (N,) object (原始路径字符串)

    返回: (npz_path, 实际处理的样本数)
    """
    manifest_path = Path(manifest_path)
    out_npz = Path(out_npz)

    if skip_if_exists and out_npz.exists():
        return out_npz, -1

    items = json.loads(manifest_path.read_text(encoding="utf-8"))
    if max_items is not None:
        items = items[:max_items]

    feats: List[List[float]] = []
    labels: List[int] = []
    paths: List[str] = []

    for i, it in enumerate(items, 1):
        img_path = it["path"]
        label = int(it["label"]) if "label" in it else -1
        try:
            emb = extract_image_embedding(img_path, timeout=timeout)
        except Exception as e:
            # 出错时跳过该样本
            print(f"[WARN] 第{i}张失败: {img_path} -> {e}")
            continue
        feats.append(emb)
        labels.append(label)
        paths.append(img_path)

        if i % 20 == 0:
            print(f"已处理 {i}/{len(items)} …")

    if not feats:
        raise RuntimeError("没有成功提取到任何特征。")

    feats_np = np.asarray(feats, dtype=np.float32)
    labels_np = np.asarray(labels, dtype=np.int64)
    paths_np = np.asarray(paths, dtype=object)

    out_npz.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(out_npz, features=feats_np, labels=labels_np, paths=paths_np)
    print(f"已保存特征到: {out_npz} | 形状: {feats_np.shape}")
    return out_npz, feats_np.shape[0]


def main():
    base = Path(__file__).resolve().parent
    manifest = base / "processed" / "manifest_cats.json"
    out_npz = base / "processed" / "wenlan_embeddings_cats.npz"
    extract_embeddings_from_manifest(manifest, out_npz)

    manifest = base / "processed" / "manifest_dogs.json"
    out_npz = base / "processed" / "wenlan_embeddings_dogs.npz"
    extract_embeddings_from_manifest(manifest, out_npz)

    manifest = base / "processed" / "manifest.json"
    out_npz = base / "processed" / "wenlan_embeddings.npz"
    extract_embeddings_from_manifest(manifest, out_npz)
if __name__ == "__main__":
    main()
