# Neural Collaborative Filtering (NCF) pipeline for recommendation.

from pathlib import Path

# Default paths: run from project root; override via train.py CLI.
PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_TRAINING_DATA_DIR = PROJECT_ROOT / "simulations" / "vesture" / "application_usage" / "training_data"
DEFAULT_TEST_SETS_DIR = PROJECT_ROOT / "simulations" / "vesture" / "application_usage" / "test_sets"

__all__ = [
    "DEFAULT_TRAINING_DATA_DIR",
    "DEFAULT_TEST_SETS_DIR",
]
