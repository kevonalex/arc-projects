# Neural Collaborative Filtering (NCF)

PyTorch NCF model and training pipeline for the vesture interaction data (users, items, interactions from `simulations/vesture/application_usage`).

## Setup

From project root (or with a venv that has PyTorch and pandas):

```bash
pip install -r analysis/machine_learning/neural_collaborative_filtering/requirements.txt
```

## Data

Expects:

- **training_data/** (e.g. `simulations/vesture/application_usage/training_data/`): `users.csv`, `items.csv`, `interactions.csv` (training)
- **test_sets/** (e.g. `simulations/vesture/application_usage/test_sets/`): `interactions.csv` (held-out test)

Generate these by running the vesture usage generator:

```bash
python3 simulations/vesture/application_usage/usage_generator_script.py
```

## Train

From **project root**:

```bash
python -m analysis.machine_learning.neural_collaborative_filtering.train
```

Optional arguments:

- `--training-data-dir` – directory with `users.csv`, `items.csv`, and `interactions.csv` (default: vesture `training_data/`)
- `--epochs`, `--batch-size`, `--lr`, `--seed`, `--save`

Checkpoints are written to `analysis/machine_learning/neural_collaborative_filtering/checkpoints/ncf_best.pt`.

## Evaluate

Run the trained model on training and test data; metrics are written to `results/`:

```bash
python -m analysis.machine_learning.neural_collaborative_filtering.evaluate
```

Optional: `--training-data-dir`, `--test-sets-dir`, `--checkpoint`, `--output-dir`, `--seed`.

Outputs:

- **Training**: loss and accuracy on training interactions.
- **Test**: loss, accuracy, Hit@5, Hit@10, and MRR (positive item vs 99 random negatives per test pair).
- **Results**: `results/evaluation_<timestamp>.json` and `results/evaluation_latest.json`.

## Model

- **GMF**: user and item embeddings, element-wise product, linear → 1
- **MLP**: concat(user_emb, item_emb), hidden layers, linear → 1
- **Combine**: concat(GMF_out, MLP_out), linear → logit; train with BCE and negative sampling

Hyperparameters (in `config.py`): `EMBEDDING_DIM`, `MLP_LAYERS`, `NEGATIVE_SAMPLES_PER_POSITIVE`, etc.
