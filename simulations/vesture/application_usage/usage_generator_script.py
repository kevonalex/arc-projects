# Script for simulating customer usage data for training the ML NCF recommendation model.

import csv
import random
import logging
from datetime import datetime, timedelta
from pathlib import Path

try:
    from .config.action_set import ACTIONS, ACTION_WEIGHTS
    from .config.aesthetic_set import AESTHETICS
except ImportError:
    from config.action_set import ACTIONS, ACTION_WEIGHTS
    from config.aesthetic_set import AESTHETICS

# SET LOGGING CONFIG
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# --- CONFIG (override for reproducibility / scale) ---
RANDOM_SEED = 42
CUSTOMER_COUNT_RANGE = (100, 200)
PRODUCT_COUNT_RANGE = (50, 100)
INTERACTIONS_PER_USER_RANGE = (5, 50)
USER_AESTHETICS_RANGE = (2, 5)
ITEM_AESTHETICS_RANGE = (1, 3)
# Bias: with this probability we pick from weighted (aesthetic overlap); else uniform.
BIASED_SAMPLE_PROB = 0.85
# Output directories: users/items -> data/, interactions -> results/
OUTPUT_DIR = Path(__file__).resolve().parent
DATA_DIR = OUTPUT_DIR / "data"
RESULTS_DIR = OUTPUT_DIR / "results"
TIMESTAMP_DAYS_AGO = 90


def _aesthetic_overlap(user_aesthetics: set, item_aesthetics: set) -> int:
    return len(user_aesthetics & item_aesthetics)


def _item_weights_for_user(user_aesthetics: set, items: list, item_aesthetics: dict, base_weight: float = 1.0) -> list[float]:
    """Weights for sampling an item for this user: higher overlap => higher weight."""
    weights = []
    for item_id in items:
        overlap = _aesthetic_overlap(user_aesthetics, item_aesthetics[item_id])
        w = base_weight + overlap
        weights.append(w)
    return weights


def _action_weights_for_overlap(overlap: int) -> list[float]:
    """Positive actions more likely when overlap high; reported/ignored more likely when overlap 0."""
    action_list = list(ACTIONS)
    # Positive actions (indices 0,1,2), negative (3,4), engagement (5,6,7)
    positive_idx = [0, 1, 2]
    negative_idx = [3, 4]
    engagement_idx = [5, 6, 7]
    weights = [1.0] * len(action_list)
    if overlap >= 1:
        for i in positive_idx:
            weights[i] = 3.0
        for i in negative_idx:
            weights[i] = 0.3
    else:
        for i in positive_idx:
            weights[i] = 0.3
        for i in negative_idx:
            weights[i] = 2.0
    return weights


def main():
    random.seed(RANDOM_SEED)

    aesthetics_list = list(AESTHETICS)
    customer_count = random.randint(*CUSTOMER_COUNT_RANGE)
    product_count = random.randint(*PRODUCT_COUNT_RANGE)

    logger.info(f"Customer count: {customer_count}")
    logger.info(f"Product count: {product_count}")

    # --- Users: each has preferred aesthetics ---
    users = []
    user_aesthetics = {}
    for uid in range(1, customer_count + 1):
        n = random.randint(*USER_AESTHETICS_RANGE)
        prefs = set(random.sample(aesthetics_list, min(n, len(aesthetics_list))))
        user_aesthetics[uid] = prefs
        users.append({"user_id": uid, "aesthetics": "|".join(sorted(prefs))})

    # --- Items: each has aesthetics ---
    items = []
    item_aesthetics = {}
    for iid in range(1, product_count + 1):
        n = random.randint(*ITEM_AESTHETICS_RANGE)
        aest = set(random.sample(aesthetics_list, min(n, len(aesthetics_list))))
        item_aesthetics[iid] = aest
        items.append({"item_id": iid, "aesthetics": "|".join(sorted(aest))})

    item_ids = list(item_aesthetics.keys())
    action_list = list(ACTIONS)

    # --- Interactions: biased by aesthetic overlap ---
    interactions = []
    end_time = datetime.now()
    start_time = end_time - timedelta(days=TIMESTAMP_DAYS_AGO)
    ts_range = (start_time, end_time)

    for uid in range(1, customer_count + 1):
        n_interactions = random.randint(*INTERACTIONS_PER_USER_RANGE)
        u_aesthetics = user_aesthetics[uid]

        for _ in range(n_interactions):
            # Biased vs uniform item choice
            if random.random() < BIASED_SAMPLE_PROB:
                weights = _item_weights_for_user(u_aesthetics, item_ids, item_aesthetics)
                iid = random.choices(item_ids, weights=weights, k=1)[0]
            else:
                iid = random.choice(item_ids)

            overlap = _aesthetic_overlap(u_aesthetics, item_aesthetics[iid])
            action_weights = _action_weights_for_overlap(overlap)
            action = random.choices(action_list, weights=action_weights, k=1)[0]
            weight = ACTION_WEIGHTS[action]

            # Random timestamp in the last N days
            delta = random.random() * (ts_range[1] - ts_range[0])
            ts = (ts_range[0] + delta).strftime("%Y-%m-%d %H:%M:%S")

            interactions.append({
                "user_id": uid,
                "item_id": iid,
                "action": action,
                "weight": weight,
                "timestamp": ts,
            })

    # --- Write CSVs: users/items -> data/, interactions -> results/ ---
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    users_path = DATA_DIR / "users.csv"
    with open(users_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["user_id", "aesthetics"])
        w.writeheader()
        w.writerows(users)
    logger.info(f"Wrote {users_path} ({len(users)} rows)")

    items_path = DATA_DIR / "items.csv"
    with open(items_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["item_id", "aesthetics"])
        w.writeheader()
        w.writerows(items)
    logger.info(f"Wrote {items_path} ({len(items)} rows)")

    interactions_path = RESULTS_DIR / "interactions.csv"
    with open(interactions_path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["user_id", "item_id", "action", "weight", "timestamp"])
        w.writeheader()
        w.writerows(interactions)
    logger.info(f"Wrote {interactions_path} ({len(interactions)} rows)")

    return users_path, items_path, interactions_path


if __name__ == "__main__":
    main()
