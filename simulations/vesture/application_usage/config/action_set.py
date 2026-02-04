ACTIONS = (
    "liked",
    "favourites",
    "saved",
    "reported",
    "ignored",
    "tapped",
    "tapped_and_clicked",
    "tapped_and_zoomed",
)

# Weights for NCF: positive (3), ignored (0.5), reported (0.2 = stronger negative), engagement (1–3).
ACTION_WEIGHTS = {
    "liked": 3.0,
    "favourites": 3.0,
    "saved": 3.0,
    "reported": 0.2,
    "ignored": 0.5,
    "tapped": 1.0,
    "tapped_and_clicked": 4.0,
    "tapped_and_zoomed": 3.0,
}