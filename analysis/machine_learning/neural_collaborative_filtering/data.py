"""Load vesture CSVs and build NCF dataset with negative sampling."""

import random
from pathlib import Path
from typing import Optional

import pandas as pd
import torch
from torch.utils.data import Dataset


def load_interaction_tables(training_data_dir: Path) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    """Load users.csv, items.csv, interactions.csv from training_data_dir. Returns (users, items, interactions)."""
    users_path = training_data_dir / "users.csv"
    items_path = training_data_dir / "items.csv"
    interactions_path = training_data_dir / "interactions.csv"

    for p in (users_path, items_path, interactions_path):
        if not p.exists():
            raise FileNotFoundError(f"Missing data file: {p}")

    users = pd.read_csv(users_path)
    items = pd.read_csv(items_path)
    interactions = pd.read_csv(interactions_path)
    return users, items, interactions


def load_test_interactions(test_sets_dir: Path) -> pd.DataFrame:
    """Load test_sets_dir/interactions.csv. Returns test interactions DataFrame."""
    path = test_sets_dir / "interactions.csv"
    if not path.exists():
        raise FileNotFoundError(f"Missing test set file: {path}")
    return pd.read_csv(path)


def build_indices_and_pairs(
    users: pd.DataFrame,
    items: pd.DataFrame,
    interactions: pd.DataFrame,
) -> tuple[dict[int, int], dict[int, int], list[tuple[int, int, float]]]:
    """
    Map raw user_id/item_id to 0-based indices. Aggregate interactions to (u_idx, i_idx, weight).
    weight = max weight for (user, item) if multiple interactions.
    Returns (user2idx, item2idx, list of (u_idx, i_idx, weight)).
    """
    user_ids = sorted(users["user_id"].unique())
    item_ids = sorted(items["item_id"].unique())
    user2idx = {uid: i for i, uid in enumerate(user_ids)}
    item2idx = {iid: i for i, iid in enumerate(item_ids)}

    # Aggregate: keep max weight per (user, item)
    agg = (
        interactions.groupby(["user_id", "item_id"], as_index=False)["weight"]
        .max()
    )
    pairs = []
    for _, row in agg.iterrows():
        u = user2idx[int(row["user_id"])]
        i = item2idx[int(row["item_id"])]
        w = float(row["weight"])
        pairs.append((u, i, w))

    return user2idx, item2idx, pairs


class NCFDataset(Dataset):
    """
    Dataset for NCF: positive (user, item) pairs with label 1 and optional weight;
    negative (user, random item) pairs with label 0. Negative sampling is done at __getitem__.
    """

    def __init__(
        self,
        pairs: list[tuple[int, int, float]],
        n_users: int,
        n_items: int,
        n_neg_per_pos: int = 4,
        seed: Optional[int] = None,
    ):
        self.pairs = pairs  # (u_idx, i_idx, weight)
        self.n_users = n_users
        self.n_items = n_items
        self.n_neg_per_pos = n_neg_per_pos
        self.seed = seed
        # Set of (u, i) for fast negative sampling
        self.positive_set = {(u, i) for u, i, _ in pairs}
        # Per-user positive items (for sampling negatives not in this set)
        self.user_positives: dict[int, set[int]] = {}
        for u, i, _ in pairs:
            self.user_positives.setdefault(u, set()).add(i)

    def __len__(self) -> int:
        return len(self.pairs) * (1 + self.n_neg_per_pos)

    def _get_negative_item(self, u: int, rng: random.Random) -> int:
        for _ in range(100):
            i = rng.randint(0, self.n_items - 1)
            if (u, i) not in self.positive_set:
                return i
        return rng.randint(0, self.n_items - 1)

    def __getitem__(self, idx: int) -> tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        # Map idx to (pos_idx, is_negative_offset)
        n_pos = len(self.pairs)
        block = 1 + self.n_neg_per_pos
        pos_idx = idx // block
        offset = idx % block
        rng = random.Random(self.seed + idx if self.seed is not None else None)

        if offset == 0:
            u, i, w = self.pairs[pos_idx]
            label = 1.0
            weight = w
        else:
            u, i, _ = self.pairs[pos_idx]
            i_neg = self._get_negative_item(u, rng)
            u, i, label, weight = u, i_neg, 0.0, 1.0

        return (
            torch.tensor(u, dtype=torch.long),
            torch.tensor(i, dtype=torch.long),
            torch.tensor(label, dtype=torch.float32),
            torch.tensor(weight, dtype=torch.float32),
        )


def train_val_split(
    pairs: list[tuple[int, int, float]],
    train_ratio: float = 0.85,
    seed: int = 42,
) -> tuple[list[tuple[int, int, float]], list[tuple[int, int, float]]]:
    """Random split of pairs into train and val."""
    rng = random.Random(seed)
    shuffled = list(pairs)
    rng.shuffle(shuffled)
    n = len(shuffled)
    n_train = int(n * train_ratio)
    return shuffled[:n_train], shuffled[n_train:]
