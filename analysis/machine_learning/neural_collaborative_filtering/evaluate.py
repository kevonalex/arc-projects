"""
Evaluate NCF on training and test data; save metrics to results/.

  From project root:
    python -m analysis.machine_learning.neural_collaborative_filtering.evaluate

  From this directory:
    python evaluate.py
"""

import argparse
import json
import sys
from datetime import datetime
from pathlib import Path

try:
    from . import DEFAULT_TRAINING_DATA_DIR as _DEFAULT_TRAINING_DATA_DIR, DEFAULT_TEST_SETS_DIR as _DEFAULT_TEST_SETS_DIR
    from .config import CHECKPOINT_DIR
    from .data import (
        build_indices_and_pairs,
        load_interaction_tables,
        load_test_interactions,
    )
    from .model import NCF
except ImportError:
    _ncf_dir = Path(__file__).resolve().parent
    _root = _ncf_dir.parents[2]
    sys.path.insert(0, str(_root))
    from analysis.machine_learning.neural_collaborative_filtering import (
        DEFAULT_TRAINING_DATA_DIR as _DEFAULT_TRAINING_DATA_DIR,
        DEFAULT_TEST_SETS_DIR as _DEFAULT_TEST_SETS_DIR,
    )
    from analysis.machine_learning.neural_collaborative_filtering.config import CHECKPOINT_DIR
    from analysis.machine_learning.neural_collaborative_filtering.data import (
        build_indices_and_pairs,
        load_interaction_tables,
        load_test_interactions,
    )
    from analysis.machine_learning.neural_collaborative_filtering.model import NCF

import torch


def get_device() -> torch.device:
    return torch.device("cuda" if torch.cuda.is_available() else "cpu")


def load_model_from_checkpoint(
    ckpt_path: Path,
    device: torch.device,
) -> tuple[NCF, dict]:
    """Load NCF and checkpoint metadata. Returns (model, ckpt_info)."""
    ckpt = torch.load(ckpt_path, map_location=device, weights_only=False)
    n_users = ckpt["n_users"]
    n_items = ckpt["n_items"]
    embedding_dim = ckpt["embedding_dim"]
    mlp_dims = tuple(ckpt["mlp_dims"])
    model = NCF(
        n_users=n_users,
        n_items=n_items,
        embedding_dim=embedding_dim,
        mlp_dims=list(mlp_dims),
    )
    model.load_state_dict(ckpt["model_state_dict"])
    model.to(device)
    model.eval()
    info = {k: v for k, v in ckpt.items() if k != "model_state_dict" and k != "optimizer_state_dict"}
    return model, info


def test_pairs_from_test_set(
    test_df,
    user2idx: dict,
    item2idx: dict,
) -> list[tuple[int, int, float]]:
    """Convert test interactions to (u_idx, i_idx, weight), skipping users/items not in training."""
    pairs = []
    for _, row in test_df.iterrows():
        uid, iid = int(row["user_id"]), int(row["item_id"])
        if uid not in user2idx or iid not in item2idx:
            continue
        w = float(row["weight"])
        pairs.append((user2idx[uid], item2idx[iid], w))
    return pairs


def compute_loss_accuracy(model: NCF, pairs: list[tuple[int, int, float]], device: torch.device, batch_size: int = 512) -> tuple[float, float]:
    """BCE loss (with label 1 for all, weight from pair) and accuracy (logit > 0)."""
    model.eval()
    total_loss = 0.0
    total_acc = 0.0
    n = 0
    criterion = torch.nn.BCEWithLogitsLoss(reduction="none")
    for start in range(0, len(pairs), batch_size):
        batch = pairs[start : start + batch_size]
        u = torch.tensor([p[0] for p in batch], dtype=torch.long, device=device)
        i = torch.tensor([p[1] for p in batch], dtype=torch.long, device=device)
        w = torch.tensor([p[2] for p in batch], dtype=torch.float32, device=device)
        with torch.no_grad():
            logits = model(u, i)
        labels = torch.ones_like(logits, device=device)
        loss_per = criterion(logits, labels)
        loss = (loss_per * w).sum() / w.sum().clamp(min=1e-8)
        acc = ((logits > 0).float() * w).sum() / w.sum().clamp(min=1e-8)
        total_loss += loss.item() * len(batch)
        total_acc += acc.item() * len(batch)
        n += len(batch)
    return total_loss / max(n, 1), total_acc / max(n, 1)


def compute_hit_at_k(
    model: NCF,
    test_pairs: list[tuple[int, int, float]],
    n_items: int,
    device: torch.device,
    k_values: tuple[int, ...] = (5, 10),
    n_negatives: int = 99,
    seed: int = 42,
) -> dict[str, float]:
    """For each test (u,i), score positive + n_negatives random negatives; compute Hit@K and MRR."""
    import random
    rng = random.Random(seed)
    model.eval()
    hit_counts = {k: 0 for k in k_values}
    mrr_sum = 0.0
    n = 0
    for u_idx, i_pos, _ in test_pairs:
        # Sample negatives (excluding the positive item)
        neg_candidates = [j for j in range(n_items) if j != i_pos]
        if len(neg_candidates) < n_negatives:
            negs = neg_candidates
        else:
            negs = rng.sample(neg_candidates, n_negatives)
        all_items = [i_pos] + negs  # positive first
        u_t = torch.tensor([u_idx] * (1 + len(negs)), dtype=torch.long, device=device)
        i_t = torch.tensor(all_items, dtype=torch.long, device=device)
        with torch.no_grad():
            scores = model(u_t, i_t).cpu().numpy()
        # Rank: higher score = better; positive is index 0
        rank = 1 + sum(1 for j in range(1, len(scores)) if scores[j] >= scores[0])
        for k in k_values:
            if rank <= k:
                hit_counts[k] += 1
        mrr_sum += 1.0 / rank
        n += 1
    result = {f"hit_at_{k}": hit_counts[k] / max(n, 1) for k in k_values}
    result["mrr"] = mrr_sum / max(n, 1)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description="Evaluate NCF on training and test data")
    parser.add_argument("--training-data-dir", type=Path, default=_DEFAULT_TRAINING_DATA_DIR)
    parser.add_argument("--test-sets-dir", type=Path, default=_DEFAULT_TEST_SETS_DIR)
    parser.add_argument("--checkpoint", type=Path, default=CHECKPOINT_DIR / "ncf_best.pt")
    parser.add_argument("--output-dir", type=Path, default=None, help="Defaults to neural_collaborative_filtering/results/")
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    device = get_device()
    output_dir = args.output_dir or (Path(__file__).resolve().parent / "results")
    output_dir.mkdir(parents=True, exist_ok=True)

    if not args.checkpoint.exists():
        print(f"Checkpoint not found: {args.checkpoint}")
        print("Train the model first: python -m analysis.machine_learning.neural_collaborative_filtering.train")
        sys.exit(1)

    print(f"Loading checkpoint: {args.checkpoint}")
    model, ckpt_info = load_model_from_checkpoint(args.checkpoint, device)

    print("Loading training and test data...")
    users, items, train_interactions = load_interaction_tables(args.training_data_dir)
    user2idx, item2idx, train_pairs = build_indices_and_pairs(users, items, train_interactions)

    test_df = load_test_interactions(args.test_sets_dir)
    test_pairs = test_pairs_from_test_set(test_df, user2idx, item2idx)
    if not test_pairs:
        print("No test pairs (all test user/item IDs missing from training).")
    else:
        print(f"Test pairs (in training vocab): {len(test_pairs)}")

    n_users, n_items = len(user2idx), len(item2idx)
    print(f"Evaluating on {len(train_pairs)} training pairs, {len(test_pairs)} test pairs...")

    train_loss, train_acc = compute_loss_accuracy(model, train_pairs, device)
    results = {
        "checkpoint": str(args.checkpoint),
        "epoch": ckpt_info.get("epoch"),
        "train_loss": round(train_loss, 6),
        "train_accuracy": round(train_acc, 6),
        "n_train_pairs": len(train_pairs),
        "n_test_pairs": len(test_pairs),
    }

    if test_pairs:
        test_loss, test_acc = compute_loss_accuracy(model, test_pairs, device)
        results["test_loss"] = round(test_loss, 6)
        results["test_accuracy"] = round(test_acc, 6)
        rank_metrics = compute_hit_at_k(
            model, test_pairs, n_items, device,
            k_values=(5, 10), n_negatives=99, seed=args.seed,
        )
        for k, v in rank_metrics.items():
            results[k] = round(v, 6)
    else:
        results["test_loss"] = None
        results["test_accuracy"] = None

    results["evaluated_at"] = datetime.utcnow().isoformat() + "Z"

    out_file = output_dir / f"evaluation_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}.json"
    with open(out_file, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2)
    print(f"Results saved to {out_file}")

    latest = output_dir / "evaluation_latest.json"
    with open(latest, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2)
    print(f"Latest copy: {latest}")

    print("\nMetrics:")
    for k, v in results.items():
        if k not in ("checkpoint", "evaluated_at"):
            print(f"  {k}: {v}")


if __name__ == "__main__":
    main()
