"""
Neural Collaborative Filtering (NCF) model: GMF + MLP branches (He et al.).
"""

import torch
import torch.nn as nn


class NCF(nn.Module):
    """
    NCF with GMF (element-wise product) and MLP (concat + layers) branches.
    Final layer combines both branches and outputs a single score (logit).
    """

    def __init__(
        self,
        n_users: int,
        n_items: int,
        embedding_dim: int = 32,
        mlp_dims: list[int] = (64, 32, 16),
    ):
        super().__init__()
        self.n_users = n_users
        self.n_items = n_items
        self.embedding_dim = embedding_dim

        # Shared embeddings for both branches
        self.user_embedding = nn.Embedding(n_users, embedding_dim)
        self.item_embedding = nn.Embedding(n_items, embedding_dim)

        # GMF: element-wise product -> linear to 1
        self.gmf_fc = nn.Linear(embedding_dim, 1)

        # MLP: concat (2 * embedding_dim) -> layers
        mlp_layers = []
        in_dim = 2 * embedding_dim
        for dim in mlp_dims:
            mlp_layers.append(nn.Linear(in_dim, dim))
            mlp_layers.append(nn.ReLU())
            mlp_layers.append(nn.Dropout(0.2))
            in_dim = dim
        self.mlp = nn.Sequential(*mlp_layers)
        self.mlp_fc = nn.Linear(mlp_dims[-1], 1)

        # Combine GMF and MLP outputs
        self.final = nn.Linear(2, 1)

        self._init_weights()

    def _init_weights(self):
        nn.init.xavier_uniform_(self.user_embedding.weight)
        nn.init.xavier_uniform_(self.item_embedding.weight)
        for m in (self.gmf_fc, self.mlp_fc, self.final):
            if isinstance(m, nn.Linear):
                nn.init.xavier_uniform_(m.weight)
                if m.bias is not None:
                    m.bias.data.zero_()
        for m in self.mlp:
            if isinstance(m, nn.Linear):
                nn.init.xavier_uniform_(m.weight)
                if m.bias is not None:
                    m.bias.data.zero_()

    def forward(self, user_idx: torch.Tensor, item_idx: torch.Tensor) -> torch.Tensor:
        """
        user_idx: (batch,) long
        item_idx: (batch,) long
        Returns: (batch,) logits
        """
        u_emb = self.user_embedding(user_idx)   # (B, E)
        i_emb = self.item_embedding(item_idx)  # (B, E)

        # GMF branch
        gmf_out = u_emb * i_emb                 # (B, E)
        gmf_out = self.gmf_fc(gmf_out)          # (B, 1)

        # MLP branch
        mlp_in = torch.cat([u_emb, i_emb], dim=1)  # (B, 2*E)
        mlp_out = self.mlp(mlp_in)                 # (B, mlp_dims[-1])
        mlp_out = self.mlp_fc(mlp_out)             # (B, 1)

        # Combine
        combined = torch.cat([gmf_out, mlp_out], dim=1)  # (B, 2)
        logits = self.final(combined).squeeze(1)        # (B,)
        return logits
