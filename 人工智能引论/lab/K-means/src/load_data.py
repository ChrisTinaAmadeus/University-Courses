from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List


def collect_image_paths(base_dir: Path) -> Dict[str, List[str]]:
    """
    收集 Cats 和 Dogs 文件夹中所有 .jpeg 图片的绝对路径。

    输入:
      base_dir: 项目根目录 (包含 archive/ 目录)

    返回:
      {"Cats": [path, ...], "Dogs": [path, ...]}
    """
    dogs_cats_dir = base_dir / "archive" / "DogsCats"
    cats_dir = dogs_cats_dir / "Cats"
    dogs_dir = dogs_cats_dir / "Dogs"

    if not cats_dir.exists():
        raise FileNotFoundError(f"未找到目录: {cats_dir}")
    if not dogs_dir.exists():
        raise FileNotFoundError(f"未找到目录: {dogs_dir}")

    def list_jpeg(folder: Path) -> List[str]:
        # 只匹配 .jpeg（注意大小写）
        return [str(p.resolve()) for p in folder.glob("*.jpeg")]

    cats = list_jpeg(cats_dir)
    dogs = list_jpeg(dogs_dir)

    return {"Cats": cats, "Dogs": dogs}


def save_manifests(paths_map: Dict[str, List[str]], out_dir: Path) -> Path:
    """
    将每个类别的路径单独保存，并生成总清单 manifest.json

    输出结构:
      out_dir/
            cats_paths.json
            dogs_paths.json
            manifest.json  # [{"path": abs_path, "label": 0/1, "label_name": "Cats"|"Dogs"}, ...]
                        manifest_cats.json  # 仅 Cats 的清单（与 manifest.json 同结构）
                        manifest_dogs.json  # 仅 Dogs 的清单（与 manifest.json 同结构）

    返回 manifest.json 的路径
    """
    out_dir.mkdir(parents=True, exist_ok=True)

    cats_paths = paths_map.get("Cats", [])
    dogs_paths = paths_map.get("Dogs", [])

    (out_dir / "cats_paths.json").write_text(
        json.dumps(cats_paths, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (out_dir / "dogs_paths.json").write_text(
        json.dumps(dogs_paths, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    label_map = {"Cats": 0, "Dogs": 1}
    manifest = []
    for name, paths in paths_map.items():
        label = label_map[name]
        for p in paths:
            manifest.append({"path": p, "label": label, "label_name": name})

    manifest_path = out_dir / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    # 生成分别针对 Cats 与 Dogs 的清单
    cats_manifest = [m for m in manifest if m["label_name"] == "Cats"]
    dogs_manifest = [m for m in manifest if m["label_name"] == "Dogs"]
    (out_dir / "manifest_cats.json").write_text(
        json.dumps(cats_manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (out_dir / "manifest_dogs.json").write_text(
        json.dumps(dogs_manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    return manifest_path


def main() -> None:
    base = Path(__file__).resolve().parent
    out = base / "processed"
    print("开始收集 .jpeg 图片路径…")
    paths_map = collect_image_paths(base)
    print({k: len(v) for k, v in paths_map.items()})
    manifest_path = save_manifests(paths_map, out)
    print(f"已保存路径清单至: {manifest_path}")


if __name__ == "__main__":
    main()
