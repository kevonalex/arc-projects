"""
Train NCF on vesture interaction data. Run from project root or set paths.

  From project root:
    python -m analysis.machine_learning.neural_collaborative_filtering.train

  From this directory:
    python train.py
"""

import argparse
import sys
from pathlib import Path

# Support running as script or as "python -m train" (no parent package)
try:
    from . import DEFAULT_TRAINING_DATA_DIR as _DEFAULT_TRAINING_DATA_DIR
    from .config import (
        BATCH_SIZE,
        CHECKPOINT_DIR,
        EMBEDDING_DIM,
        EPOCHS,
        LEARNING_RATE,
        MLP_LAYERS,
        NEGATIVE_SAMPLES_PER_POSITIVE,
        RANDOM_SEED,
        TRAIN_RATIO,
        WEIGHT_DECAY,
    )
    from .data import (
        NCFDataset,
        build_indices_and_pairs,
        load_interaction_tables,
        train_val_split,
    )
    from .model import NCF
except ImportError:
    _ncf_dir = Path(__file__).resolve().parent
    _root = _ncf_dir.parents[2]  # -> project root
    sys.path.insert(0, str(_root))
    from analysis.machine_learning.neural_collaborative_filtering import (
        DEFAULT_TRAINING_DATA_DIR as _DEFAULT_TRAINING_DATA_DIR,
    )
    from analysis.machine_learning.neural_collaborative_filtering.config import (
        BATCH_SIZE,
        CHECKPOINT_DIR,
        EMBEDDING_DIM,
        EPOCHS,
        LEARNING_RATE,
        MLP_LAYERS,
        NEGATIVE_SAMPLES_PER_POSITIVE,
        RANDOM_SEED,
        TRAIN_RATIO,
        WEIGHT_DECAY,
    )
    from analysis.machine_learning.neural_collaborative_filtering.data import (
        NCFDataset,
        build_indices_and_pairs,
        load_interaction_tables,
        train_val_split,
    )
    from analysis.machine_learning.neural_collaborative_filtering.model import NCF

import torch
from torch.utils.data import DataLoader


def get_device() -> torch.device:
    return torch.device("cuda" if torch.cuda.is_available() else "cpu")


def main() -> None:
    parser = argparse.ArgumentParser(description="Train NCF on vesture interactions")
    parser.add_argument(
        "--training-data-dir",
        type=Path,
        default=_DEFAULT_TRAINING_DATA_DIR,
        help="Directory containing users.csv, items.csv, and interactions.csv",
    )
    parser.add_argument("--epochs", type=int, default=EPOCHS)
    parser.add_argument("--batch-size", type=int, default=BATCH_SIZE)
    parser.add_argument("--lr", type=float, default=LEARNING_RATE)
    parser.add_argument("--seed", type=int, default=RANDOM_SEED)
    parser.add_argument("--save", type=Path, default=CHECKPOINT_DIR, help="Checkpoint directory")
    args = parser.parse_args()

    torch.manual_seed(args.seed)
    device = get_device()
    print(f"Device: {device}")

    # Load data
    users, items, interactions = load_interaction_tables(args.training_data_dir)
    user2idx, item2idx, pairs = build_indices_and_pairs(users, items, interactions)
    n_users = len(user2idx)
    n_items = len(item2idx)
    print(f"Users: {n_users}, Items: {n_items}, Pairs: {len(pairs)}")

    train_pairs, val_pairs = train_val_split(pairs, train_ratio=TRAIN_RATIO, seed=args.seed)
    print(f"Train pairs: {len(train_pairs)}, Val pairs: {len(val_pairs)}")

    train_dataset = NCFDataset(
        train_pairs,
        n_users=n_users,
        n_items=n_items,
        n_neg_per_pos=NEGATIVE_SAMPLES_PER_POSITIVE,
        seed=args.seed,
    )
    val_dataset = NCFDataset(
        val_pairs,
        n_users=n_users,
        n_items=n_items,
        n_neg_per_pos=NEGATIVE_SAMPLES_PER_POSITIVE,
        seed=args.seed + 1,
    )

    train_loader = DataLoader(
        train_dataset,
        batch_size=args.batch_size,
        shuffle=True,
        num_workers=0,
        pin_memory=(device.type == "cuda"),
    )
    val_loader = DataLoader(
        val_dataset,
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=0,
    )

    model = NCF(
        n_users=n_users,
        n_items=n_items,
        embedding_dim=EMBEDDING_DIM,
        mlp_dims=MLP_LAYERS,
    ).to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr, weight_decay=WEIGHT_DECAY)
    criterion = torch.nn.BCEWithLogitsLoss(reduction="none")

    args.save.mkdir(parents=True, exist_ok=True)
    best_val_loss = float("inf")

    for epoch in range(1, args.epochs + 1):
        model.train()
        train_loss = 0.0
        n_train = 0
        for batch in train_loader:
            u, i, label, weight = (x.to(device) for x in batch)
            logits = model(u, i)
            loss_per = criterion(logits, label)
            loss = (loss_per * weight).sum() / weight.sum().clamp(min=1e-8)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            train_loss += loss.item() * u.size(0)
            n_train += u.size(0)

        train_loss /= max(n_train, 1)

        model.eval()
        val_loss = 0.0
        n_val = 0
        with torch.no_grad():
            for batch in val_loader:
                u, i, label, weight = (x.to(device) for x in batch)
                logits = model(u, i)
                loss_per = criterion(logits, label)
                loss = (loss_per * weight).sum() / weight.sum().clamp(min=1e-8)
                val_loss += loss.item() * u.size(0)
                n_val += u.size(0)
        val_loss /= max(n_val, 1)

        print(f"Epoch {epoch}/{args.epochs}  train_loss={train_loss:.4f}  val_loss={val_loss:.4f}")

        if val_loss < best_val_loss:
            best_val_loss = val_loss
            ckpt = args.save / "ncf_best.pt"
            torch.save({
                "epoch": epoch,
                "model_state_dict": model.state_dict(),
                "optimizer_state_dict": optimizer.state_dict(),
                "n_users": n_users,
                "n_items": n_items,
                "embedding_dim": EMBEDDING_DIM,
                "mlp_dims": MLP_LAYERS,
                "val_loss": val_loss,
            }, ckpt)
            print(f"  -> saved {ckpt}")

    print("Done.")


if __name__ == "__main__":
    main()
