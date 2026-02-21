"""Hyperparameters and paths for NCF training."""

from pathlib import Path

from . import DEFAULT_TRAINING_DATA_DIR, DEFAULT_TEST_SETS_DIR

# Data paths (override via CLI in train.py)
TRAINING_DATA_DIR = DEFAULT_TRAINING_DATA_DIR
TEST_SETS_DIR = DEFAULT_TEST_SETS_DIR

# Model: embedding and MLP sizes (He et al. NCF paper)
EMBEDDING_DIM = 32
MLP_LAYERS = [64, 32, 16]  # MLP branch hidden dims

# Training
BATCH_SIZE = 256
EPOCHS = 20
LEARNING_RATE = 1e-3
WEIGHT_DECAY = 0.0
NEGATIVE_SAMPLES_PER_POSITIVE = 4  # negative (user, item) per positive
TRAIN_RATIO = 0.85  # fraction of interactions for training (rest for val)
RANDOM_SEED = 42

# Device
DEVICE = "cpu"  # set to "cuda" if available on machine; train.py can override

# Outputs
CHECKPOINT_DIR = Path(__file__).resolve().parent / "checkpoints"
CHECKPOINT_DIR.mkdir(parents=True, exist_ok=True)
