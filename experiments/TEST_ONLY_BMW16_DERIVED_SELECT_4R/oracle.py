"""Frozen TEST_ONLY stable order-statistic oracle for BMW16 audit work.

The input values are signed Q20.12 integers.  The fractional scale does not
change their order, so this helper accepts the signed integer represented by
each original input position.  It is intentionally independent of the BMW16
candidate interpreter: no algorithm code imports this module.
"""

from __future__ import annotations

from collections.abc import Sequence

INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1


def _validate(scores: Sequence[int], k: int) -> None:
    if not scores:
        raise ValueError("input must be non-empty")
    if not 1 <= k <= len(scores):
        raise ValueError("k must satisfy 1 <= k <= n")
    for score in scores:
        if not INT32_MIN <= score <= INT32_MAX:
            raise ValueError("score must be a signed int32 Q20.12 integer")


def stable_order(scores: Sequence[int]) -> list[int]:
    """Return original indices by score descending, then index ascending."""
    if not scores:
        raise ValueError("input must be non-empty")
    for score in scores:
        if not INT32_MIN <= score <= INT32_MAX:
            raise ValueError("score must be a signed int32 Q20.12 integer")
    return sorted(range(len(scores)), key=lambda i: (-scores[i], i))


def select_index(scores: Sequence[int], k: int) -> int:
    """Return the 1-based k-th highest element under the frozen stable order."""
    _validate(scores, k)
    return stable_order(scores)[k - 1]


def top_k_mask(scores: Sequence[int], k: int) -> list[int]:
    """Return an original-order 0/1 mask with exactly k selected entries."""
    _validate(scores, k)
    mask = [0] * len(scores)
    for index in stable_order(scores)[:k]:
        mask[index] = 1
    return mask
